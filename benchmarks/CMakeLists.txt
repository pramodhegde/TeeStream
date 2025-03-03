add_executable(teestream_benchmark benchmark.cpp)

target_link_libraries(teestream_benchmark
    PRIVATE
        teestream
        pthread
)

target_include_directories(teestream_benchmark
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
) 