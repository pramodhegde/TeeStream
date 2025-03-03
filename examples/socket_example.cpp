#include "TeeStream.h"

#include <atomic>
#include <chrono>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <signal.h>
#include <string>
#include <thread>

// Use standalone Asio (non-Boost version)
#define ASIO_STANDALONE
#include <asio.hpp>

// Custom streambuf for Asio TCP socket
class AsioSocketStreambuf : public std::streambuf {
private:
    std::shared_ptr<asio::ip::tcp::socket> socket;
    char buffer[8192];  // Internal buffer
    bool connected;
    asio::error_code last_error;

public:
    explicit AsioSocketStreambuf(std::shared_ptr<asio::ip::tcp::socket> socket)
        : socket(socket), connected(socket && socket->is_open()) {
        // Set buffer for output operations
        setp(buffer, buffer + sizeof(buffer) - 1);
    }

    ~AsioSocketStreambuf() {
        sync();  // Flush any remaining data
    }

    bool is_connected() const {
        return connected && socket && socket->is_open();
    }

    asio::error_code get_last_error() const {
        return last_error;
    }

protected:
    // Flush the buffer to the socket
    virtual int sync() override {
        if (!is_connected()) return -1;

        int num = pptr() - pbase();
        if (num > 0) {
            try {
                asio::error_code ec;
                size_t sent = socket->write_some(asio::buffer(pbase(), num), ec);
                
                if (ec) {
                    last_error = ec;
                    connected = false;
                    return -1;
                }
                
                pbump(-static_cast<int>(sent));  // Reset buffer position
            }
            catch (const std::exception& e) {
                connected = false;
                return -1;
            }
        }
        return 0;
    }

    // Handle buffer overflow
    virtual int_type overflow(int_type c) override {
        if (!is_connected()) return traits_type::eof();

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
        if (!is_connected()) return 0;

        // If the data is larger than our buffer, send directly
        if (n >= sizeof(buffer)) {
            sync();  // Flush any buffered data first
            
            try {
                asio::error_code ec;
                size_t sent = socket->write_some(asio::buffer(s, n), ec);
                
                if (ec) {
                    last_error = ec;
                    connected = false;
                    return 0;
                }
                
                return sent;
            }
            catch (const std::exception& e) {
                connected = false;
                return 0;
            }
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

// Socket output stream using Asio
class AsioSocketStream : public std::ostream {
private:
    std::shared_ptr<asio::ip::tcp::socket> socket;
    AsioSocketStreambuf buf;

public:
    explicit AsioSocketStream(std::shared_ptr<asio::ip::tcp::socket> socket)
        : std::ostream(nullptr), socket(socket), buf(socket) {
        rdbuf(&buf);
    }

    bool is_connected() const {
        return buf.is_connected();
    }

    asio::error_code get_last_error() const {
        return buf.get_last_error();
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

// Helper function to get current timestamp as string
std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_tm = std::localtime(&now_time_t);
    
    std::ostringstream oss;
    oss << std::put_time(now_tm, "[%Y-%m-%d %H:%M:%S] ");
    return oss.str();
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

    try {
        // Initialize Asio
        asio::io_context io_context;
        
        // Create a TCP socket
        auto socket = std::make_shared<asio::ip::tcp::socket>(io_context);
        
        // Resolve the server address
        asio::ip::tcp::resolver resolver(io_context);
        asio::ip::tcp::resolver::results_type endpoints = 
            resolver.resolve(server_ip, std::to_string(port));
        
        // Connect to the server
        asio::error_code ec;
        asio::connect(*socket, endpoints, ec);
        
        if (ec) {
            std::cerr << "Failed to connect to server: " << ec.message() << std::endl;
            std::cerr << "You can start a simple TCP server with: nc -l " << port << std::endl;
            return 1;
        }
        
        // Create socket stream
        AsioSocketStream socket_stream(socket);
        
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
            tee << get_timestamp() << "Message #" << counter++ << ": "
                << "Data sent to both socket and file simultaneously!" << std::endl;
            
            // Wait a bit before sending the next message
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        // Final message
        tee << get_timestamp() << "Connection closed. Sent " << counter << " messages." << std::endl;
        
        // Close the socket gracefully
        if (socket->is_open()) {
            socket->shutdown(asio::ip::tcp::socket::shutdown_both, ec);
            socket->close(ec);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Program completed successfully.\n";
    return 0;
} 