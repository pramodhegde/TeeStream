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

// A high-performance thread-safe tee streambuf using thread-local buffers
class TeeStreamBuf : public std::streambuf {
private:
    // Thread-local buffer structure
    struct ThreadBuffer {
        std::unique_ptr<char[]> buffer;
        size_t size;
        size_t used;

        explicit ThreadBuffer(size_t buffer_size);
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
