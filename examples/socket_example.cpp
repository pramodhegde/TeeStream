#include "TeeStream.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <thread>
#include <chrono>
#include <atomic>
#include <signal.h>

// Socket headers
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>

// Custom streambuf for TCP socket
class SocketStreambuf : public std::streambuf {
private:
    int socket_fd;
    char buffer[8192];  // Internal buffer
    bool connected;

public:
    explicit SocketStreambuf(int socket_fd) : socket_fd(socket_fd), connected(true) {
        // Set buffer for output operations
        setp(buffer, buffer + sizeof(buffer) - 1);
    }

    ~SocketStreambuf() {
        sync();  // Flush any remaining data
        if (socket_fd >= 0) {
            close(socket_fd);
        }
    }

    bool is_connected() const {
        return connected;
    }

protected:
    // Flush the buffer to the socket
    virtual int sync() override {
        if (!connected) return -1;

        int num = pptr() - pbase();
        if (num > 0) {
            int sent = send(socket_fd, pbase(), num, 0);
            if (sent <= 0) {
                connected = false;
                return -1;
            }
            pbump(-sent);  // Reset buffer position
        }
        return 0;
    }

    // Handle buffer overflow
    virtual int_type overflow(int_type c) override {
        if (!connected) return traits_type::eof();

        if (c != traits_type::eof()) {
            *pptr() = c;
            pbump(1);
        }
        
        if (sync() == -1) {
            return traits_type::eof();
        }
        
        return c;
    }

    // Write multiple characters
    virtual std::streamsize xsputn(const char* s, std::streamsize n) override {
        if (!connected) return 0;

        // If the data is larger than our buffer, send directly
        if (n >= sizeof(buffer)) {
            sync();  // Flush any buffered data first
            int sent = send(socket_fd, s, n, 0);
            if (sent <= 0) {
                connected = false;
                return 0;
            }
            return sent;
        }

        // Otherwise, use the buffer
        const char* end = s + n;
        while (s < end) {
            if (pptr() == epptr()) {
                if (sync() == -1) {
                    break;
                }
            }
            
            std::streamsize avail = epptr() - pptr();
            std::streamsize chunk = std::min(avail, end - s);
            std::memcpy(pptr(), s, chunk);
            pbump(chunk);
            s += chunk;
        }
        
        return s - (end - n);
    }
};

// Socket output stream
class SocketStream : public std::ostream {
private:
    SocketStreambuf buf;

public:
    explicit SocketStream(int socket_fd) : std::ostream(nullptr), buf(socket_fd) {
        rdbuf(&buf);
    }

    bool is_connected() const {
        return buf.is_connected();
    }
};

// Global flag for handling Ctrl+C
std::atomic<bool> running(true);

// Signal handler
void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nReceived Ctrl+C, shutting down...\n";
        running = false;
    }
}

// Function to create a TCP client socket
int create_tcp_client(const std::string& server_ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket\n";
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    // Convert IP address from string to binary form
    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address or address not supported\n";
        close(sock);
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed\n";
        close(sock);
        return -1;
    }

    return sock;
}

int main(int argc, char* argv[]) {
    // Register signal handler for Ctrl+C
    signal(SIGINT, signal_handler);

    // Default values
    std::string server_ip = "127.0.0.1";  // localhost
    int port = 12345;
    std::string log_file = "socket_log.txt";

    // Parse command line arguments
    for (int i = 1; i < argc; i += 2) {
        if (i + 1 >= argc) {
            std::cerr << "Missing value for parameter " << argv[i] << std::endl;
            return 1;
        }

        std::string param = argv[i];
        std::string value = argv[i + 1];

        if (param == "--ip") {
            server_ip = value;
        } else if (param == "--port") {
            port = std::stoi(value);
        } else if (param == "--log") {
            log_file = value;
        } else {
            std::cerr << "Unknown parameter: " << param << std::endl;
            return 1;
        }
    }

    std::cout << "Connecting to " << server_ip << ":" << port << std::endl;
    std::cout << "Logging to file: " << log_file << std::endl;
    std::cout << "Press Ctrl+C to exit\n\n";

    // Create TCP socket
    int sock_fd = create_tcp_client(server_ip, port);
    if (sock_fd < 0) {
        std::cerr << "Failed to connect to server. Is the server running?\n";
        std::cerr << "You can start a simple TCP server with: nc -l " << port << std::endl;
        return 1;
    }

    try {
        // Create socket stream
        SocketStream socket_stream(sock_fd);
        
        // Create file stream
        std::ofstream file_stream(log_file);
        if (!file_stream) {
            std::cerr << "Failed to open log file: " << log_file << std::endl;
            return 1;
        }
        
        // Create TeeStream to write to both socket and file
        TeeStream tee;
        tee.add_stream(socket_stream);
        tee.add_stream(file_stream);
        tee.add_stream(std::cout);  // Also write to console
        
        // Send data periodically
        int counter = 0;
        while (running && socket_stream.is_connected()) {
            // Create a message with timestamp
            auto now = std::chrono::system_clock::now();
            auto now_time_t = std::chrono::system_clock::to_time_t(now);
            
            tee << "[" << std::ctime(&now_time_t) << "] "
                << "Message #" << counter++ << ": "
                << "Data sent to both socket and file simultaneously!" << std::endl;
            
            // Wait a bit before sending the next message
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        // Final message
        tee << "Connection closed. Sent " << counter << " messages." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Program completed successfully.\n";
    return 0;
} 