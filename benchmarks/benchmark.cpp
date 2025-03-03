#include "TeeStream.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>
#include <numeric>
#include <algorithm>
#include <random>
#include <functional>
#include <memory>

// Simple timer class for benchmarking
class Timer {
private:
    std::chrono::high_resolution_clock::time_point start_time;
    std::string name;

public:
    explicit Timer(const std::string& name = "") : name(name) {
        start_time = std::chrono::high_resolution_clock::now();
    }

    ~Timer() {
        stop();
    }

    double stop() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        double seconds = duration / 1000000.0;
        
        if (!name.empty()) {
            std::cout << name << ": " << std::fixed << std::setprecision(6) << seconds << " seconds" << std::endl;
        }
        
        return seconds;
    }
};

// Generate random data for testing
std::string generate_random_data(size_t size) {
    std::string data(size, 0);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(32, 126); // Printable ASCII
    
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<char>(distrib(gen));
    }
    
    return data;
}

// Benchmark 1: Throughput test - how much data can be processed per second
void benchmark_throughput(size_t data_size, int iterations) {
    std::cout << "\n=== Throughput Benchmark ===" << std::endl;
    std::cout << "Data size: " << data_size << " bytes, Iterations: " << iterations << std::endl;
    
    // Generate test data
    std::string data = generate_random_data(data_size);
    
    // Create null streams that discard output
    class NullBuffer : public std::streambuf {
    protected:
        virtual int overflow(int c) override { return c; }
        virtual std::streamsize xsputn(const char*, std::streamsize n) override { return n; }
    };
    
    NullBuffer null_buffer;
    std::ostream null_stream1(&null_buffer);
    std::ostream null_stream2(&null_buffer);
    
    // Benchmark TeeStream
    {
        TeeStream tee;
        tee.add_stream(null_stream1);
        tee.add_stream(null_stream2);
        
        Timer timer("TeeStream throughput");
        for (int i = 0; i < iterations; ++i) {
            tee.write(data.data(), data.size());
        }
        tee.flush_thread_buffer();
        
        double seconds = timer.stop();
        double total_mb = (data_size * iterations) / (1024.0 * 1024.0);
        double mb_per_sec = total_mb / seconds;
        
        std::cout << "Total data: " << std::fixed << std::setprecision(2) << total_mb << " MB" << std::endl;
        std::cout << "Throughput: " << std::fixed << std::setprecision(2) << mb_per_sec << " MB/s" << std::endl;
    }
    
    // Benchmark naive implementation for comparison
    {
        Timer timer("Naive implementation throughput");
        for (int i = 0; i < iterations; ++i) {
            null_stream1.write(data.data(), data.size());
            null_stream2.write(data.data(), data.size());
        }
        null_stream1.flush();
        null_stream2.flush();
        
        double seconds = timer.stop();
        double total_mb = (data_size * iterations) / (1024.0 * 1024.0);
        double mb_per_sec = total_mb / seconds;
        
        std::cout << "Total data: " << std::fixed << std::setprecision(2) << total_mb << " MB" << std::endl;
        std::cout << "Throughput: " << std::fixed << std::setprecision(2) << mb_per_sec << " MB/s" << std::endl;
    }
}

// Benchmark 2: Latency test - how long individual operations take
void benchmark_latency(int iterations) {
    std::cout << "\n=== Latency Benchmark ===" << std::endl;
    std::cout << "Iterations: " << iterations << std::endl;
    
    // Create null streams that discard output
    class NullBuffer : public std::streambuf {
    protected:
        virtual int overflow(int c) override { return c; }
        virtual std::streamsize xsputn(const char*, std::streamsize n) override { return n; }
    };
    
    NullBuffer null_buffer;
    std::ostream null_stream1(&null_buffer);
    std::ostream null_stream2(&null_buffer);
    
    // Test data sizes
    std::vector<size_t> sizes = {8, 64, 512, 4096, 32768, 262144};
    
    std::cout << "\nTeeStream latency:" << std::endl;
    for (size_t size : sizes) {
        std::string data = generate_random_data(size);
        
        TeeStream tee;
        tee.add_stream(null_stream1);
        tee.add_stream(null_stream2);
        
        std::vector<double> latencies;
        latencies.reserve(iterations);
        
        for (int i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            
            tee.write(data.data(), data.size());
            tee.flush_thread_buffer();
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            latencies.push_back(duration);
        }
        
        // Calculate statistics
        std::sort(latencies.begin(), latencies.end());
        double avg = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        double median = latencies[latencies.size() / 2];
        double p95 = latencies[static_cast<size_t>(latencies.size() * 0.95)];
        double p99 = latencies[static_cast<size_t>(latencies.size() * 0.99)];
        
        std::cout << "Size: " << std::setw(8) << size << " bytes | "
                  << "Avg: " << std::setw(8) << std::fixed << std::setprecision(2) << avg << " µs | "
                  << "Median: " << std::setw(8) << median << " µs | "
                  << "p95: " << std::setw(8) << p95 << " µs | "
                  << "p99: " << std::setw(8) << p99 << " µs" << std::endl;
    }
    
    std::cout << "\nNaive implementation latency:" << std::endl;
    for (size_t size : sizes) {
        std::string data = generate_random_data(size);
        
        std::vector<double> latencies;
        latencies.reserve(iterations);
        
        for (int i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            
            null_stream1.write(data.data(), data.size());
            null_stream2.write(data.data(), data.size());
            null_stream1.flush();
            null_stream2.flush();
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            latencies.push_back(duration);
        }
        
        // Calculate statistics
        std::sort(latencies.begin(), latencies.end());
        double avg = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        double median = latencies[latencies.size() / 2];
        double p95 = latencies[static_cast<size_t>(latencies.size() * 0.95)];
        double p99 = latencies[static_cast<size_t>(latencies.size() * 0.99)];
        
        std::cout << "Size: " << std::setw(8) << size << " bytes | "
                  << "Avg: " << std::setw(8) << std::fixed << std::setprecision(2) << avg << " µs | "
                  << "Median: " << std::setw(8) << median << " µs | "
                  << "p95: " << std::setw(8) << p95 << " µs | "
                  << "p99: " << std::setw(8) << p99 << " µs" << std::endl;
    }
}

// Benchmark 3: Scalability test - how performance scales with multiple threads
void benchmark_scalability(size_t data_size, int iterations_per_thread) {
    std::cout << "\n=== Scalability Benchmark ===" << std::endl;
    std::cout << "Data size: " << data_size << " bytes, Iterations per thread: " << iterations_per_thread << std::endl;
    
    // Create null streams that discard output
    class NullBuffer : public std::streambuf {
    protected:
        virtual int overflow(int c) override { return c; }
        virtual std::streamsize xsputn(const char*, std::streamsize n) override { return n; }
    };
    
    NullBuffer null_buffer;
    std::ostream null_stream1(&null_buffer);
    std::ostream null_stream2(&null_buffer);
    
    // Generate test data
    std::string data = generate_random_data(data_size);
    
    // Test with different numbers of threads
    std::vector<int> thread_counts = {1, 2, 4, 8, 16, 32};
    
    std::cout << "\nTeeStream scalability:" << std::endl;
    for (int num_threads : thread_counts) {
        TeeStream tee;
        tee.add_stream(null_stream1);
        tee.add_stream(null_stream2);
        
        std::vector<std::thread> threads;
        std::atomic<int> ready_count(0);
        std::atomic<bool> start_flag(false);
        
        auto thread_func = [&]() {
            // Signal that this thread is ready
            ready_count++;
            
            // Wait for the start signal
            while (!start_flag.load()) {
                std::this_thread::yield();
            }
            
            // Run the benchmark
            for (int i = 0; i < iterations_per_thread; ++i) {
                tee.write(data.data(), data.size());
            }
        };
        
        // Create threads
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back(thread_func);
        }
        
        // Wait for all threads to be ready
        while (ready_count.load() < num_threads) {
            std::this_thread::yield();
        }
        
        // Start the benchmark
        Timer timer("TeeStream with " + std::to_string(num_threads) + " threads");
        start_flag.store(true);
        
        // Wait for all threads to finish
        for (auto& t : threads) {
            t.join();
        }
        
        // Ensure all data is flushed
        tee.flush_thread_buffer();
        
        double seconds = timer.stop();
        double total_mb = (data_size * iterations_per_thread * num_threads) / (1024.0 * 1024.0);
        double mb_per_sec = total_mb / seconds;
        
        std::cout << "Total data: " << std::fixed << std::setprecision(2) << total_mb << " MB" << std::endl;
        std::cout << "Throughput: " << std::fixed << std::setprecision(2) << mb_per_sec << " MB/s" << std::endl;
    }
}

// Benchmark 4: Buffer size impact - how different buffer sizes affect performance
void benchmark_buffer_sizes(size_t data_size, int iterations) {
    std::cout << "\n=== Buffer Size Impact Benchmark ===" << std::endl;
    std::cout << "Data size: " << data_size << " bytes, Iterations: " << iterations << std::endl;
    
    // Create null streams that discard output
    class NullBuffer : public std::streambuf {
    protected:
        virtual int overflow(int c) override { return c; }
        virtual std::streamsize xsputn(const char*, std::streamsize n) override { return n; }
    };
    
    NullBuffer null_buffer;
    std::ostream null_stream1(&null_buffer);
    std::ostream null_stream2(&null_buffer);
    
    // Generate test data
    std::string data = generate_random_data(data_size);
    
    // Test with different buffer sizes
    std::vector<size_t> buffer_sizes = {1024, 4096, 16384, 65536, 262144, 1048576};
    
    std::cout << "\nTeeStream with different buffer sizes:" << std::endl;
    for (size_t buffer_size : buffer_sizes) {
        // Create TeeStream with specific buffer size
        TeeStream tee(buffer_size, buffer_size * 0.75);
        tee.add_stream(null_stream1);
        tee.add_stream(null_stream2);
        
        Timer timer("Buffer size: " + std::to_string(buffer_size) + " bytes");
        for (int i = 0; i < iterations; ++i) {
            tee.write(data.data(), data.size());
        }
        tee.flush_thread_buffer();
        
        double seconds = timer.stop();
        double total_mb = (data_size * iterations) / (1024.0 * 1024.0);
        double mb_per_sec = total_mb / seconds;
        
        std::cout << "Throughput: " << std::fixed << std::setprecision(2) << mb_per_sec << " MB/s" << std::endl;
    }
}

// Benchmark 5: Stream count impact - how the number of streams affects performance
void benchmark_stream_count(size_t data_size, int iterations) {
    std::cout << "\n=== Stream Count Impact Benchmark ===" << std::endl;
    std::cout << "Data size: " << data_size << " bytes, Iterations: " << iterations << std::endl;
    
    // Create null streams that discard output
    class NullBuffer : public std::streambuf {
    protected:
        virtual int overflow(int c) override { return c; }
        virtual std::streamsize xsputn(const char*, std::streamsize n) override { return n; }
    };
    
    // Generate test data
    std::string data = generate_random_data(data_size);
    
    // Test with different numbers of streams
    std::vector<int> stream_counts = {1, 2, 4, 8, 16, 32};
    
    std::cout << "\nTeeStream with different stream counts:" << std::endl;
    for (int stream_count : stream_counts) {
        // Create streams
        std::vector<std::unique_ptr<NullBuffer>> buffers;
        std::vector<std::unique_ptr<std::ostream>> streams;
        
        for (int i = 0; i < stream_count; ++i) {
            buffers.push_back(std::make_unique<NullBuffer>());
            streams.push_back(std::make_unique<std::ostream>(buffers.back().get()));
        }
        
        // Create TeeStream and add all streams
        TeeStream tee;
        for (auto& stream : streams) {
            tee.add_stream(*stream);
        }
        
        Timer timer(std::to_string(stream_count) + " streams");
        for (int i = 0; i < iterations; ++i) {
            tee.write(data.data(), data.size());
        }
        tee.flush_thread_buffer();
        
        double seconds = timer.stop();
        double total_mb = (data_size * iterations) / (1024.0 * 1024.0);
        double mb_per_sec = total_mb / seconds;
        
        std::cout << "Throughput: " << std::fixed << std::setprecision(2) << mb_per_sec << " MB/s" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::cout << "TeeStream Performance Benchmarks" << std::endl;
    std::cout << "===============================" << std::endl;
    
    // Default parameters
    size_t throughput_data_size = 1024 * 1024;  // 1 MB
    int throughput_iterations = 100;
    
    int latency_iterations = 1000;
    
    size_t scalability_data_size = 1024 * 64;  // 64 KB
    int scalability_iterations = 1000;
    
    size_t buffer_test_data_size = 1024 * 64;  // 64 KB
    int buffer_test_iterations = 1000;
    
    size_t stream_count_data_size = 1024 * 64;  // 64 KB
    int stream_count_iterations = 1000;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i += 2) {
        if (i + 1 >= argc) {
            std::cerr << "Missing value for parameter " << argv[i] << std::endl;
            return 1;
        }
        
        std::string param = argv[i];
        std::string value = argv[i + 1];
        
        if (param == "--throughput-size") {
            throughput_data_size = std::stoul(value);
        } else if (param == "--throughput-iterations") {
            throughput_iterations = std::stoi(value);
        } else if (param == "--latency-iterations") {
            latency_iterations = std::stoi(value);
        } else if (param == "--scalability-size") {
            scalability_data_size = std::stoul(value);
        } else if (param == "--scalability-iterations") {
            scalability_iterations = std::stoi(value);
        } else if (param == "--buffer-size") {
            buffer_test_data_size = std::stoul(value);
        } else if (param == "--buffer-iterations") {
            buffer_test_iterations = std::stoi(value);
        } else if (param == "--stream-size") {
            stream_count_data_size = std::stoul(value);
        } else if (param == "--stream-iterations") {
            stream_count_iterations = std::stoi(value);
        } else {
            std::cerr << "Unknown parameter: " << param << std::endl;
            return 1;
        }
    }
    
    // Run benchmarks
    benchmark_throughput(throughput_data_size, throughput_iterations);
    benchmark_latency(latency_iterations);
    benchmark_scalability(scalability_data_size, scalability_iterations);
    benchmark_buffer_sizes(buffer_test_data_size, buffer_test_iterations);
    benchmark_stream_count(stream_count_data_size, stream_count_iterations);
    
    return 0;
} 