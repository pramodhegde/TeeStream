#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <streambuf>
#include <ostream>
#include <mutex>
#include <thread>
#include <functional>
#include <shared_mutex>
#include <sstream>  // For std::ostringstream in batch_write

// A high-performance thread-safe tee streambuf using thread-local buffers
class TeeStreamBuf : public std::streambuf {
private:
    // Thread-local buffer structure
    struct ThreadBuffer {
        std::unique_ptr<char[]> buffer;
        size_t size;
        size_t used;
        
        // For adaptive buffer sizing
        double avg_write_size;
        int sample_count;
        size_t max_buffer_size;

        explicit ThreadBuffer(size_t buffer_size, size_t max_size = 1048576);
        
        // Update buffer statistics for adaptive sizing
        void update_stats(size_t write_size);
        
        // Resize buffer if needed based on usage patterns
        bool adapt_buffer_size();
    };

    // Thread-local storage for buffers
    static thread_local std::unique_ptr<ThreadBuffer> local_buffer;

    // Master stream list with shared mutex for reader/writer lock
    std::vector<std::reference_wrapper<std::ostream>> streams;
    mutable std::shared_mutex streams_mutex;

    // Buffer configuration
    size_t buffer_size;
    size_t flush_threshold;

    // Initialize thread-local buffer if not already done
    ThreadBuffer* get_thread_buffer();

public:
    // Constructor with configurable buffer size and flush threshold
    explicit TeeStreamBuf(size_t buffer_size = 8192, size_t flush_threshold = 6144);
    
    // Destructor - flush any remaining data
    ~TeeStreamBuf();

    // Thread-safe stream management
    void add_stream(std::ostream& stream);
    void remove_stream(std::ostream& stream);
    
    // Manually flush the thread-local buffer
    void flush_thread_buffer();
    
    // Batch write multiple items
    template<typename... Args>
    void batch_write(const Args&... args) {
        std::ostringstream batch_buffer;
        (batch_buffer << ... << args);  // C++17 fold expression
        std::string str = batch_buffer.str();
        this->xsputn(str.c_str(), str.size());
    }

protected:
    // Override streambuf methods
    virtual int_type overflow(int_type c) override;
    virtual std::streamsize xsputn(const char* s, std::streamsize n) override;
    virtual int sync() override;
};

// A high-performance thread-safe tee stream
class TeeStream : public std::ostream {
private:
    TeeStreamBuf buffer;

public:
    // Constructor with configurable buffer size and flush threshold
    explicit TeeStream(size_t buffer_size = 8192, size_t flush_threshold = 6144);
    
    // Constructor that takes a list of streams to write to
    template<typename... Streams>
    explicit TeeStream(Streams&... streams) : std::ostream(&buffer) {
        (add_stream(streams), ...);
    }

    // Stream management
    void add_stream(std::ostream& stream);
    void remove_stream(std::ostream& stream);
    
    // Manually flush the thread-local buffer
    void flush_thread_buffer();
};
