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
- `TEESTREAM_BUILD_EXAMPLES`: Build the example programs (ON by default)
- `TEESTREAM_BUILD_BENCHMARKS`: Build the benchmarking suite (ON by default)

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

## Performance Benchmarking

TeeStream includes a comprehensive benchmarking suite to evaluate its performance under various conditions.

### Running Benchmarks

The simplest way to run benchmarks is using the provided script:

```bash
# Make the script executable
chmod +x benchmark.sh

# Run all benchmarks
./benchmark.sh

# Run a quick benchmark with fewer iterations
./benchmark.sh --quick
```

### Benchmark Options

You can run specific benchmarks:

```bash
# Run only throughput benchmark
./benchmark.sh --throughput-only

# Run only latency benchmark
./benchmark.sh --latency-only

# Run only scalability benchmark
./benchmark.sh --scalability-only

# Run only buffer size impact benchmark
./benchmark.sh --buffer-only

# Run only stream count impact benchmark
./benchmark.sh --stream-only
```

### Custom Benchmark Parameters

You can customize benchmark parameters:

```bash
# Custom throughput benchmark
./benchmark.sh --throughput-size 2097152 --throughput-iterations 50

# Custom latency benchmark
./benchmark.sh --latency-iterations 2000
```

### What's Being Measured

The benchmarking suite measures:

1. **Throughput**: How many MB/s the TeeStream can process
2. **Latency**: Operation latency across different data sizes (8B to 256KB)
3. **Scalability**: How performance scales with 1-32 threads
4. **Buffer Size Impact**: How different buffer sizes affect performance
5. **Stream Count Impact**: How performance changes with different numbers of output streams

### Building Benchmarks Manually

You can also build and run benchmarks manually:

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DTEESTREAM_BUILD_BENCHMARKS=ON
make
./benchmarks/teestream_benchmark
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