#include "TeeStream.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <vector>

// Helper function to get current timestamp as string
std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_tm = std::localtime(&now_time_t);
    
    std::ostringstream oss;
    oss << std::put_time(now_tm, "[%Y-%m-%d %H:%M:%S] ");
    return oss.str();
}

// Example of using TeeStream with multiple output streams
void basic_usage_example() {
    std::cout << "\n=== Basic Usage Example ===\n";
    
    // Create output streams
    std::ofstream log_file("basic_example.log");
    std::ostringstream string_stream;
    
    // Create a TeeStream that writes to console, file, and string stream
    TeeStream tee;
    tee.add_stream(std::cout);
    tee.add_stream(log_file);
    tee.add_stream(string_stream);
    
    // Write to all streams at once
    tee << get_timestamp() << "Hello, World!" << std::endl;
    tee << get_timestamp() << "This message goes to all three streams." << std::endl;
    
    // Show that we can access the string stream content
    std::cout << "\nContent captured in string_stream:\n";
    std::cout << "-----------------------------------\n";
    std::cout << string_stream.str();
    std::cout << "-----------------------------------\n";
}

// Example of using the constructor with streams
void constructor_example() {
    std::cout << "\n=== Constructor Example ===\n";
    
    // Create output streams
    std::ofstream log_file("constructor_example.log");
    std::ostringstream string_stream;
    
    // Create a TeeStream with streams directly in the constructor
    TeeStream tee(std::cout, log_file, string_stream);
    
    // Write to all streams at once
    tee << get_timestamp() << "Initialized with multiple streams in constructor." << std::endl;
    tee << get_timestamp() << "This is more concise for simple cases." << std::endl;
    
    // Show that we can access the string stream content
    std::cout << "\nContent captured in string_stream:\n";
    std::cout << "-----------------------------------\n";
    std::cout << string_stream.str();
    std::cout << "-----------------------------------\n";
}

// Example of using custom buffer sizes
void buffer_size_example() {
    std::cout << "\n=== Custom Buffer Size Example ===\n";
    
    // Create a TeeStream with custom buffer size and flush threshold
    // buffer_size = 128 bytes (small for demonstration)
    // flush_threshold = 64 bytes (50% of buffer)
    TeeStream tee(128, 64);
    
    std::ofstream log_file("buffer_example.log");
    tee.add_stream(std::cout);
    tee.add_stream(log_file);
    
    // Write data that will trigger automatic flushing due to buffer threshold
    tee << get_timestamp() << "Using a small buffer size of 128 bytes with a flush threshold of 64 bytes." << std::endl;
    
    // Generate a string larger than the buffer
    std::string large_string(200, '*');
    tee << get_timestamp() << "Large string that exceeds buffer size: " << large_string << std::endl;
    
    // Manual flush demonstration
    tee << get_timestamp() << "This message might be buffered...";
    tee.flush_thread_buffer();  // Explicitly flush
    tee << " (flushed manually)" << std::endl;
}

// Example of adding and removing streams dynamically
void dynamic_streams_example() {
    std::cout << "\n=== Dynamic Streams Example ===\n";
    
    // Create output streams
    std::ofstream log_file1("dynamic_example1.log");
    std::ofstream log_file2("dynamic_example2.log");
    std::ostringstream string_stream;
    
    // Create a TeeStream with just one stream initially
    TeeStream tee;
    tee.add_stream(std::cout);
    
    // First message goes only to cout
    tee << get_timestamp() << "Message 1: This only goes to std::cout" << std::endl;
    
    // Add a second stream
    tee.add_stream(log_file1);
    tee << get_timestamp() << "Message 2: This goes to std::cout and log_file1" << std::endl;
    
    // Add a third stream
    tee.add_stream(string_stream);
    tee << get_timestamp() << "Message 3: This goes to std::cout, log_file1, and string_stream" << std::endl;
    
    // Add a fourth stream
    tee.add_stream(log_file2);
    tee << get_timestamp() << "Message 4: This goes to all four streams" << std::endl;
    
    // Remove a stream
    tee.remove_stream(log_file1);
    tee << get_timestamp() << "Message 5: This goes to std::cout, string_stream, and log_file2" << std::endl;
    
    // Show the content of the string stream
    std::cout << "\nContent captured in string_stream:\n";
    std::cout << "-----------------------------------\n";
    std::cout << string_stream.str();
    std::cout << "-----------------------------------\n";
}

// Example of using TeeStream with multiple threads
void multithreaded_example() {
    std::cout << "\n=== Multithreaded Example ===\n";
    
    // Create output streams
    std::ofstream log_file("multithreaded_example.log");
    
    // Create a TeeStream
    TeeStream tee;
    tee.add_stream(std::cout);
    tee.add_stream(log_file);
    
    // Function for worker threads
    auto worker = [&tee](int id, int iterations) {
        for (int i = 0; i < iterations; ++i) {
            tee << get_timestamp() << "Thread " << id << ": iteration " << i << std::endl;
            
            // Small sleep to make output more readable
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };
    
    // Create and start threads
    const int num_threads = 4;
    const int iterations = 5;
    std::vector<std::thread> threads;
    
    tee << get_timestamp() << "Starting " << num_threads << " threads..." << std::endl;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker, i, iterations);
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }
    
    tee << get_timestamp() << "All threads completed." << std::endl;
}

// Example of formatting options with TeeStream
void formatting_example() {
    std::cout << "\n=== Formatting Example ===\n";
    
    // Create output streams
    std::ofstream log_file("formatting_example.log");
    
    // Create a TeeStream
    TeeStream tee;
    tee.add_stream(std::cout);
    tee.add_stream(log_file);
    
    // Demonstrate various formatting options
    tee << get_timestamp() << "Default formatting: " << 42 << " " << 3.14159 << std::endl;
    
    // Fixed precision
    tee << get_timestamp() << "Fixed precision (3): " 
        << std::fixed << std::setprecision(3) << 3.14159 << std::endl;
    
    // Reset to default formatting
    tee.copyfmt(std::ios(nullptr));
    
    // Hex formatting
    tee << get_timestamp() << "Hexadecimal: " 
        << std::hex << std::showbase << 255 << std::endl;
    
    // Reset to default formatting
    tee.copyfmt(std::ios(nullptr));
    
    // Width and fill
    tee << get_timestamp() << "Width and fill: [" 
        << std::setw(10) << std::setfill('0') << std::right << 42 << "]" << std::endl;
    
    // Reset to default formatting
    tee.copyfmt(std::ios(nullptr));
    
    // Boolean values
    tee << get_timestamp() << "Boolean (default): " << true << " " << false << std::endl;
    tee << get_timestamp() << "Boolean (alpha): " 
        << std::boolalpha << true << " " << false << std::endl;
}

int main() {
    std::cout << "TeeStream Basic Examples" << std::endl;
    std::cout << "=======================" << std::endl;
    
    // Run all examples
    basic_usage_example();
    constructor_example();
    buffer_size_example();
    dynamic_streams_example();
    multithreaded_example();
    formatting_example();
    
    std::cout << "\nAll examples completed. Check the generated log files for output." << std::endl;
    
    return 0;
} 