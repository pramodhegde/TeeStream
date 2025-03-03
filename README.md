# TeeStream

A high-performance thread-safe C++ library for writing to multiple output streams simultaneously, similar to the Unix `tee` command.

## Features

- Write to multiple output streams with a single operation
- Thread-safe design for concurrent access
- High-performance implementation using thread-local buffers
- Compatible with any `std::ostream` derived class (files, string streams, etc.)
- Configurable buffer sizes for performance tuning
- Modern C++ implementation (C++17)

## Requirements

- C++17 compatible compiler
- CMake 3.14 or higher (for building)
- GoogleTest (automatically downloaded if not found, for testing)

## Installation

### Using CMake

```bash
mkdir build && cd build
cmake ..
make
make install
```

### CMake Options

- `TEESTREAM_BUILD_TESTS`: Build the test suite (ON by default)

## Usage

### Basic Usage

```cpp
#include <TeeStream.h>
#include <iostream>
#include <fstream>

int main() {
    // Create output streams
    std::ofstream log_file("log.txt");
    
    // Create a TeeStream that writes to both console and file
    TeeStream tee;
    tee.add_stream(std::cout);
    tee.add_stream(log_file);
    
    // Write to both streams at once
    tee << "Hello, World!" << std::endl;
    
    return 0;
}
```

### Constructor with Streams

```cpp
#include <TeeStream.h>
#include <iostream>
#include <fstream>
#include <sstream>

int main() {
    std::ofstream log_file("log.txt");
    std::ostringstream string_stream;
    
    // Initialize with multiple streams
    TeeStream tee(std::cout, log_file, string_stream);
    
    tee << "This goes to all three streams" << std::endl;
    
    // Access the string stream content
    std::string content = string_stream.str();
    
    return 0;
}
```

### Custom Buffer Sizes

```cpp
#include <TeeStream.h>
#include <iostream>

int main() {
    // Create a TeeStream with custom buffer size and flush threshold
    // buffer_size = 4096 bytes
    // flush_threshold = 3072 bytes (75% of buffer)
    TeeStream tee(4096, 3072);
    
    tee.add_stream(std::cout);
    
    // Use the stream
    tee << "Using custom buffer sizes" << std::endl;
    
    return 0;
}
```

### Thread Safety

TeeStream is designed to be safely used from multiple threads simultaneously. Each thread maintains its own buffer, and the stream list is protected by a reader-writer lock.

```cpp
#include <TeeStream.h>
#include <iostream>
#include <thread>
#include <vector>

void worker_function(TeeStream& tee, int id) {
    for (int i = 0; i < 100; i++) {
        tee << "Thread " << id << ": iteration " << i << std::endl;
    }
}

int main() {
    TeeStream tee(std::cout);
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back(worker_function, std::ref(tee), i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    return 0;
}
```

## API Reference

### TeeStream Class

```cpp
class TeeStream : public std::ostream {
public:
    // Constructor with configurable buffer size and flush threshold
    explicit TeeStream(size_t buffer_size = 8192, size_t flush_threshold = 6144);
    
    // Constructor that takes a list of streams to write to
    template<typename... Streams>
    explicit TeeStream(Streams&... streams);

    // Stream management
    void add_stream(std::ostream& stream);
    void remove_stream(std::ostream& stream);
    
    // Manually flush the thread-local buffer
    void flush_thread_buffer();
};
```

## License

This project is licensed under the MIT License - see the LICENSE file for details. 