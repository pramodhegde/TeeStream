#!/bin/bash
set -e  # Exit on error

echo "=== Building TeeStream for Benchmarking ==="

# Create build directory if it doesn't exist
mkdir -p build
cd build

echo "=== Configuring with CMake in Release mode ==="
cmake .. -DCMAKE_BUILD_TYPE=Release -DTEESTREAM_BUILD_BENCHMARKS=ON

echo "=== Building ==="
# Determine number of CPU cores for parallel build
CORES=$(nproc 2>/dev/null || echo 4)
echo "Building with $CORES cores"
cmake --build . -- -j$CORES

if [ -f benchmarks/teestream_benchmark ]; then
    echo -e "\n=== Running benchmarks ==="
    
    # Parse command line arguments
    ARGS=""
    while [ "$#" -gt 0 ]; do
        case "$1" in
            --quick)
                # Quick benchmark with fewer iterations
                ARGS="--throughput-iterations 10 --latency-iterations 100 --scalability-iterations 100 --buffer-iterations 100 --stream-iterations 100"
                shift
                ;;
            --throughput-only)
                # Run only throughput benchmark
                ./benchmarks/teestream_benchmark --throughput-size 1048576 --throughput-iterations 100
                cd ..
                exit 0
                ;;
            --latency-only)
                # Run only latency benchmark
                ./benchmarks/teestream_benchmark --latency-iterations 1000
                cd ..
                exit 0
                ;;
            --scalability-only)
                # Run only scalability benchmark
                ./benchmarks/teestream_benchmark --scalability-size 65536 --scalability-iterations 1000
                cd ..
                exit 0
                ;;
            --buffer-only)
                # Run only buffer size benchmark
                ./benchmarks/teestream_benchmark --buffer-size 65536 --buffer-iterations 1000
                cd ..
                exit 0
                ;;
            --stream-only)
                # Run only stream count benchmark
                ./benchmarks/teestream_benchmark --stream-size 65536 --stream-iterations 1000
                cd ..
                exit 0
                ;;
            *)
                # Add other arguments directly
                ARGS="$ARGS $1"
                shift
                ;;
        esac
    done
    
    # Run the benchmark with any additional arguments
    ./benchmarks/teestream_benchmark $ARGS
else
    echo "Error: Benchmark executable not found!"
    exit 1
fi

cd ..
echo -e "\n=== Benchmarking completed ===" 