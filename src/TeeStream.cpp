#include "TeeStream.h"
#include <cstring>

// Initialize thread-local storage
thread_local std::unique_ptr<TeeStreamBuf::ThreadBuffer> TeeStreamBuf::local_buffer;

// ThreadBuffer implementation
TeeStreamBuf::ThreadBuffer::ThreadBuffer(size_t buffer_size)
    : buffer(std::make_unique<char[]>(buffer_size)),
      size(buffer_size),
      used(0) {}

// Get or create thread-local buffer
TeeStreamBuf::ThreadBuffer* TeeStreamBuf::get_thread_buffer() {
    if (!local_buffer) {
        local_buffer = std::make_unique<ThreadBuffer>(buffer_size);
    }
    return local_buffer.get();
}

// Constructor
TeeStreamBuf::TeeStreamBuf(size_t buffer_size, size_t flush_threshold)
    : buffer_size(buffer_size), flush_threshold(flush_threshold) {
    // Validate parameters
    if (flush_threshold >= buffer_size) {
        this->flush_threshold = buffer_size * 3 / 4; // Default to 75% if invalid
    }
}

// Destructor
TeeStreamBuf::~TeeStreamBuf() {
    sync(); // Flush any remaining data
}

// Add a stream to write to
void TeeStreamBuf::add_stream(std::ostream& stream) {
    std::unique_lock<std::shared_mutex> lock(streams_mutex);
    streams.emplace_back(stream);
}

// Remove a stream
void TeeStreamBuf::remove_stream(std::ostream& stream) {
    std::unique_lock<std::shared_mutex> lock(streams_mutex);
    streams.erase(
        std::remove_if(streams.begin(), streams.end(),
            [&stream](const std::reference_wrapper<std::ostream>& ref) {
                return &ref.get() == &stream;
            }
        ),
        streams.end()
    );
}

// Flush the thread-local buffer
void TeeStreamBuf::flush_thread_buffer() {
    auto tb = get_thread_buffer();
    
    // Nothing to flush
    if (tb->used == 0) {
        return;
    }

    // Make a copy of the buffer data to avoid holding the lock while writing
    std::unique_ptr<char[]> buffer_copy = std::make_unique<char[]>(tb->used);
    size_t buffer_size = tb->used;
    
    // Copy the data
    memcpy(buffer_copy.get(), tb->buffer.get(), buffer_size);
    
    // Reset the buffer before we start writing to streams
    tb->used = 0;

    // Take a shared lock to read the streams (allows multiple threads to flush simultaneously)
    std::shared_lock<std::shared_mutex> lock(streams_mutex);

    // Make sure there are streams to write to
    if (!streams.empty()) {
        // Write to each stream
        for (auto& stream_ref : streams) {
            std::ostream& stream = stream_ref.get();
            stream.write(buffer_copy.get(), buffer_size);
        }
    }
}

// Handle single character overflow
TeeStreamBuf::int_type TeeStreamBuf::overflow(int_type c) {
    if (c != traits_type::eof()) {
        char ch = traits_type::to_char_type(c);
        return xsputn(&ch, 1) == 1 ? c : traits_type::eof();
    }
    return traits_type::eof();
}

// Write multiple characters
std::streamsize TeeStreamBuf::xsputn(const char* s, std::streamsize n) {
    if (n <= 0) {
        return 0;
    }

    auto tb = get_thread_buffer();

    // If n is larger than our buffer, write directly to streams
    if (static_cast<size_t>(n) >= tb->size) {
        // Take a shared lock to read the streams
        std::shared_lock<std::shared_mutex> lock(streams_mutex);

        bool all_good = true;
        for (auto& stream_ref : streams) {
            std::ostream& stream = stream_ref.get();
            if (!stream.write(s, n)) {
                all_good = false;
            }
        }
        
        return all_good ? n : 0;
    }

    // If adding n would overflow the buffer, flush first
    if (tb->used + n > tb->size) {
        flush_thread_buffer();
    }

    // Copy to the thread-local buffer
    memcpy(tb->buffer.get() + tb->used, s, static_cast<size_t>(n));
    tb->used += n;

    // Auto-flush if we're above the threshold
    if (tb->used >= flush_threshold) {
        flush_thread_buffer();
    }

    return n;
}

// Sync/flush the buffer
int TeeStreamBuf::sync() {
    flush_thread_buffer();
    
    // Take a shared lock to read the streams
    std::shared_lock<std::shared_mutex> lock(streams_mutex);
    
    bool all_good = true;
    for (auto& stream_ref : streams) {
        std::ostream& stream = stream_ref.get();
        if (stream.rdbuf()->pubsync() == -1) {
            all_good = false;
        }
    }
    
    return all_good ? 0 : -1;
}

// TeeStream implementation

// Constructor
TeeStream::TeeStream(size_t buffer_size, size_t flush_threshold)
    : std::ostream(&buffer), buffer(buffer_size, flush_threshold) {
}

// Add a stream
void TeeStream::add_stream(std::ostream& stream) {
    buffer.add_stream(stream);
}

// Remove a stream
void TeeStream::remove_stream(std::ostream& stream) {
    buffer.remove_stream(stream);
}

// Flush the thread-local buffer
void TeeStream::flush_thread_buffer() {
    buffer.flush_thread_buffer();
} 