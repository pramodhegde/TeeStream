#include "TeeStream.h"

#include <chrono>
#include <cstring>
#include <future>
#include <iomanip>
#include <limits>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

// Test basic functionality
TEST(TeeStreamTest, BasicOutput) {
    std::ostringstream stream1, stream2;
    TeeStream tee;
    
    tee.add_stream(stream1);
    tee.add_stream(stream2);
    
    tee << "Hello, World!" << std::endl;
    
    EXPECT_EQ("Hello, World!\n", stream1.str());
    EXPECT_EQ("Hello, World!\n", stream2.str());
}

// Test with different types of data
TEST(TeeStreamTest, DifferentDataTypes) {
    std::ostringstream stream1, stream2;
    TeeStream tee;
    
    tee.add_stream(stream1);
    tee.add_stream(stream2);
    
    tee << "String: " << 42 << " " << 3.14 << " " << true << std::endl;
    
    EXPECT_EQ("String: 42 3.14 1\n", stream1.str());
    EXPECT_EQ("String: 42 3.14 1\n", stream2.str());
}

// Test adding and removing streams
TEST(TeeStreamTest, AddRemoveStreams) {
    std::ostringstream stream1, stream2, stream3;
    TeeStream tee;
    
    tee.add_stream(stream1);
    tee.add_stream(stream2);
    
    tee << "First output" << std::endl;
    
    EXPECT_EQ("First output\n", stream1.str());
    EXPECT_EQ("First output\n", stream2.str());
    EXPECT_EQ("", stream3.str());
    
    tee.add_stream(stream3);
    tee.remove_stream(stream1);
    
    tee << "Second output" << std::endl;
    
    EXPECT_EQ("First output\n", stream1.str());
    EXPECT_EQ("First output\nSecond output\n", stream2.str());
    EXPECT_EQ("Second output\n", stream3.str());
}

// Test constructor with streams
TEST(TeeStreamTest, ConstructorWithStreams) {
    std::ostringstream stream1, stream2;
    TeeStream tee(stream1, stream2);
    
    tee << "Constructor test" << std::endl;
    
    EXPECT_EQ("Constructor test\n", stream1.str());
    EXPECT_EQ("Constructor test\n", stream2.str());
}

// Test large output (buffer handling)
TEST(TeeStreamTest, LargeOutput) {
    std::ostringstream stream1, stream2;
    TeeStream tee;
    
    tee.add_stream(stream1);
    tee.add_stream(stream2);
    
    // Create a string larger than the default buffer size
    std::string large_string(10000, 'X');
    tee << large_string << std::endl;
    
    EXPECT_EQ(large_string + "\n", stream1.str());
    EXPECT_EQ(large_string + "\n", stream2.str());
}

// Test manual flush
TEST(TeeStreamTest, ManualFlush) {
    std::ostringstream stream1, stream2;
    TeeStream tee;
    
    tee.add_stream(stream1);
    tee.add_stream(stream2);
    
    tee << "Test without flush";
    // No endl, so it might be buffered
    
    // Force flush
    tee.flush_thread_buffer();
    
    EXPECT_EQ("Test without flush", stream1.str());
    EXPECT_EQ("Test without flush", stream2.str());
}

// Test multithreaded access
TEST(TeeStreamTest, MultithreadedAccess) {
    std::ostringstream stream1, stream2;
    TeeStream tee;
    
    tee.add_stream(stream1);
    tee.add_stream(stream2);
    
    const int num_threads = 10;
    const int iterations = 100;
    
    std::mutex output_mutex;  // Add mutex for synchronizing output
    
    auto thread_func = [&tee, &output_mutex](int thread_id) {
        for (int i = 0; i < iterations; i++) {
            // Use the mutex to synchronize access to the tee stream
            std::lock_guard<std::mutex> lock(output_mutex);
            tee << "Thread " << thread_id << " iteration " << i << std::endl;
        }
    };
    
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(thread_func, i);
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    // Ensure final flush
    tee.flush_thread_buffer();
    
    // Verify both streams have the same content
    EXPECT_EQ(stream1.str(), stream2.str());
    
    // Verify the number of lines matches what we expect
    int line_count = 0;
    std::string str = stream1.str();
    for (char c : str) {
        if (c == '\n') line_count++;
    }
    
    EXPECT_EQ(num_threads * iterations, line_count);
}

// Test custom buffer sizes
TEST(TeeStreamTest, CustomBufferSizes) {
    std::ostringstream stream1, stream2;
    
    // Small buffer size and threshold
    TeeStream tee(128, 64);
    
    tee.add_stream(stream1);
    tee.add_stream(stream2);
    
    // Write data larger than the buffer
    std::string data(200, 'A');
    tee << data;
    
    // Should have auto-flushed due to buffer size
    EXPECT_EQ(data, stream1.str());
    EXPECT_EQ(data, stream2.str());
}

// Test with file streams
TEST(TeeStreamTest, FileStreams) {
    // Create temporary file streams
    std::ofstream file1("test_file1.txt");
    std::ofstream file2("test_file2.txt");
    
    {
        TeeStream tee;
        tee.add_stream(file1);
        tee.add_stream(file2);
        
        tee << "Writing to files" << std::endl;
        
        // Close the TeeStream to ensure flushing
    }
    
    // Close the files
    file1.close();
    file2.close();
    
    // Read back the files
    std::ifstream check1("test_file1.txt");
    std::ifstream check2("test_file2.txt");
    
    std::string content1, content2;
    std::getline(check1, content1);
    std::getline(check2, content2);
    
    EXPECT_EQ("Writing to files", content1);
    EXPECT_EQ("Writing to files", content2);
    
    // Clean up
    std::remove("test_file1.txt");
    std::remove("test_file2.txt");
}

// Test empty stream list
TEST(TeeStreamTest, EmptyStreamList) {
    TeeStream tee;
    
    // Writing to a TeeStream with no streams should not crash
    EXPECT_NO_THROW({
        tee << "This should not crash" << std::endl;
        tee.flush_thread_buffer();
    });
}

// Test with very large data
TEST(TeeStreamTest, VeryLargeData) {
    std::ostringstream stream1, stream2;
    TeeStream tee;
    
    tee.add_stream(stream1);
    tee.add_stream(stream2);
    
    // Create a very large string (20MB)
    const size_t large_size = 20 * 1024 * 1024;
    std::string large_string(large_size, 'X');
    
    // Write the large string
    tee << large_string;
    tee.flush_thread_buffer();
    
    // Verify the output
    EXPECT_EQ(large_size, stream1.str().size());
    EXPECT_EQ(large_size, stream2.str().size());
    EXPECT_EQ(large_string, stream1.str());
    EXPECT_EQ(large_string, stream2.str());
}

// Test with binary data
TEST(TeeStreamTest, BinaryData) {
    std::ostringstream stream1, stream2;
    TeeStream tee;
    
    tee.add_stream(stream1);
    tee.add_stream(stream2);
    
    // Create some binary data with null bytes
    std::vector<char> binary_data(1024);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 255);
    
    for (auto& byte : binary_data) {
        byte = static_cast<char>(distrib(gen));
    }
    
    // Write the binary data
    tee.write(binary_data.data(), binary_data.size());
    tee.flush_thread_buffer();
    
    // Verify the output
    EXPECT_EQ(binary_data.size(), stream1.str().size());
    EXPECT_EQ(binary_data.size(), stream2.str().size());
    EXPECT_EQ(0, std::memcmp(binary_data.data(), stream1.str().data(), binary_data.size()));
    EXPECT_EQ(0, std::memcmp(binary_data.data(), stream2.str().data(), binary_data.size()));
}

// Test with different stream types simultaneously
TEST(TeeStreamTest, MixedStreamTypes) {
    std::ostringstream string_stream;
    std::ofstream file_stream("mixed_stream_test.txt");
    
    {
        TeeStream tee;
        tee.add_stream(string_stream);
        tee.add_stream(file_stream);
        
        tee << "Testing mixed stream types" << std::endl;
        tee.flush_thread_buffer();
    }
    
    // Close the file stream
    file_stream.close();
    
    // Read back the file
    std::ifstream check_file("mixed_stream_test.txt");
    std::string file_content;
    std::getline(check_file, file_content);
    
    EXPECT_EQ("Testing mixed stream types", file_content);
    EXPECT_EQ("Testing mixed stream types\n", string_stream.str());
    
    // Clean up
    std::remove("mixed_stream_test.txt");
}

// Test formatting options
TEST(TeeStreamTest, FormattingOptions) {
    std::ostringstream stream1, stream2;
    TeeStream tee;
    
    tee.add_stream(stream1);
    tee.add_stream(stream2);
    
    // Test fixed precision formatting
    tee << std::fixed << std::setprecision(3);
    tee << "Pi: " << 3.14159265359 << std::endl;
    
    // Test hex formatting
    tee << std::hex << std::showbase;
    tee << "Hex: " << 255 << std::endl;
    
    // Reset formatting for the next test to avoid unexpected behavior
    tee << std::dec << std::noshowbase;
    
    // Test width and fill formatting
    // Note: We need to apply these right before the value they affect
    tee << "Padded: " << std::setw(10) << std::setfill('0') << std::right << 42 << std::endl;
    
    // Verify the output - using the actual expected output
    std::string expected = "Pi: 3.142\nHex: 0xff\nPadded: 0000000042\n";
    EXPECT_EQ(expected, stream1.str());
    EXPECT_EQ(expected, stream2.str());
}

// Test adding the same stream multiple times
TEST(TeeStreamTest, DuplicateStreams) {
    std::ostringstream stream;
    TeeStream tee;
    
    // Add the same stream twice
    tee.add_stream(stream);
    tee.add_stream(stream);
    
    // Write to the tee stream
    tee << "Test" << std::endl;
    
    // The output should appear twice in the stream
    EXPECT_EQ("Test\nTest\n", stream.str());
}

// Test with custom buffer sizes that are very small
TEST(TeeStreamTest, VerySmallBuffer) {
    std::ostringstream stream1, stream2;
    
    // Create a TeeStream with a very small buffer
    TeeStream tee(16, 8);  // 16-byte buffer, 8-byte threshold
    
    tee.add_stream(stream1);
    tee.add_stream(stream2);
    
    // Write data larger than the buffer size
    tee << "This string is longer than 16 bytes" << std::endl;
    
    // Verify the output
    EXPECT_EQ("This string is longer than 16 bytes\n", stream1.str());
    EXPECT_EQ("This string is longer than 16 bytes\n", stream2.str());
}

// Test with extreme numeric values
TEST(TeeStreamTest, ExtremeNumericValues) {
    std::ostringstream stream1, stream2;
    TeeStream tee;
    
    tee.add_stream(stream1);
    tee.add_stream(stream2);
    
    // Test with extreme numeric values
    tee << "Max int: " << std::numeric_limits<int>::max() << std::endl;
    tee << "Min int: " << std::numeric_limits<int>::min() << std::endl;
    tee << "Max double: " << std::numeric_limits<double>::max() << std::endl;
    tee << "Min double: " << std::numeric_limits<double>::min() << std::endl;
    tee << "Infinity: " << std::numeric_limits<double>::infinity() << std::endl;
    tee << "NaN: " << std::numeric_limits<double>::quiet_NaN() << std::endl;
    
    // Verify both streams have the same content
    EXPECT_EQ(stream1.str(), stream2.str());
    
    // Check that the content is not empty
    EXPECT_FALSE(stream1.str().empty());
}

// Test with multiple threads writing different data
TEST(TeeStreamTest, MultithreadedDifferentData) {
    std::ostringstream stream1, stream2;
    TeeStream tee;
    
    tee.add_stream(stream1);
    tee.add_stream(stream2);
    
    const int num_threads = 4;
    std::mutex output_mutex;
    
    auto thread_func = [&tee, &output_mutex](int thread_id, const std::string& data) {
        for (int i = 0; i < 10; i++) {
            std::lock_guard<std::mutex> lock(output_mutex);
            tee << "Thread " << thread_id << ": " << data << " - " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    };
    
    std::vector<std::thread> threads;
    std::vector<std::string> test_data = {
        "AAAAA", "BBBBB", "CCCCC", "DDDDD"
    };
    
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(thread_func, i, test_data[i]);
    }
    
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    // Ensure final flush
    tee.flush_thread_buffer();
    
    // Verify both streams have the same content
    EXPECT_EQ(stream1.str(), stream2.str());
    
    // Count the number of lines
    int line_count = 0;
    std::string str = stream1.str();
    for (char c : str) {
        if (c == '\n') line_count++;
    }
    
    EXPECT_EQ(num_threads * 10, line_count);
}

// Test adding and removing streams during operation
TEST(TeeStreamTest, DynamicStreamManagement) {
    std::ostringstream stream1, stream2, stream3;
    TeeStream tee;
    
    // Start with one stream
    tee.add_stream(stream1);
    tee << "First line" << std::endl;
    
    // Add a second stream
    tee.add_stream(stream2);
    tee << "Second line" << std::endl;
    
    // Remove the first stream and add a third
    tee.remove_stream(stream1);
    tee.add_stream(stream3);
    tee << "Third line" << std::endl;
    
    // Verify the output
    EXPECT_EQ("First line\nSecond line\n", stream1.str());
    EXPECT_EQ("Second line\nThird line\n", stream2.str());
    EXPECT_EQ("Third line\n", stream3.str());
}

// Test with a failing stream
TEST(TeeStreamTest, FailingStream) {
    class FailingStream : public std::ostream {
    private:
        class FailingBuf : public std::streambuf {
        protected:
            virtual int overflow(int c) override { return traits_type::eof(); }
            virtual std::streamsize xsputn(const char*, std::streamsize) override { return 0; }
        } buf;
    
    public:
        FailingStream() : std::ostream(&buf) {
            setstate(std::ios::failbit);
        }
    };
    
    std::ostringstream good_stream;
    FailingStream failing_stream;
    
    TeeStream tee;
    tee.add_stream(good_stream);
    tee.add_stream(failing_stream);
    
    // This should not crash, even though one stream is failing
    EXPECT_NO_THROW({
        tee << "This should not crash" << std::endl;
        tee.flush_thread_buffer();
    });
    
    // The good stream should still have received the data
    EXPECT_EQ("This should not crash\n", good_stream.str());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 