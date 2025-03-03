#!/bin/bash
set -e  # Exit on error

echo "=== Building TeeStream ==="

# Create build directory if it doesn't exist
mkdir -p build
cd build

echo "=== Configuring with CMake ==="
cmake .. -DCMAKE_BUILD_TYPE=Release  # Use Release mode for benchmarks

echo "=== Building ==="
# Determine number of CPU cores for parallel build
CORES=$(nproc 2>/dev/null || echo 4)
echo "Building with $CORES cores"
cmake --build . -- -j$CORES

# Run tests if they were built
if [ -f tests/teestream_tests ]; then
    echo -e "\n=== Running tests ==="
    ctest --output-on-failure
    
    # Check if any tests failed
    if [ $? -ne 0 ]; then
        echo -e "\n⚠️  Some tests failed!"
    else
        echo -e "\n✅ All tests passed!"
    fi
fi

# Run example if it was built
if [ -f examples/basic_example ]; then
    echo -e "\n=== Running basic example ==="
    ./examples/basic_example
fi

# Run benchmarks if requested
if [ -f benchmarks/teestream_benchmark ] && [ "$1" == "--benchmark" ]; then
    echo -e "\n=== Running benchmarks ==="
    ./benchmarks/teestream_benchmark
fi

cd ..
echo -e "\n=== Build completed ===" 