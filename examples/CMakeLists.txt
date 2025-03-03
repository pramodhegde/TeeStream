# Find Asio (standalone version)
include(FetchContent)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-30-2
)
FetchContent_MakeAvailable(asio)

# Basic example
add_executable(basic_example basic_example.cpp)
target_link_libraries(basic_example PRIVATE teestream)
target_include_directories(basic_example PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Socket example with Asio
add_executable(socket_example socket_example.cpp)
target_link_libraries(socket_example PRIVATE teestream pthread)
target_include_directories(socket_example 
    PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
        ${asio_SOURCE_DIR}/asio/include
)
target_compile_definitions(socket_example PRIVATE ASIO_STANDALONE) 