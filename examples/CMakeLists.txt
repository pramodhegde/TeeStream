# Basic example
add_executable(basic_example basic_example.cpp)
target_link_libraries(basic_example PRIVATE teestream)
target_include_directories(basic_example PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Socket example
add_executable(socket_example socket_example.cpp)
target_link_libraries(socket_example PRIVATE teestream pthread)
target_include_directories(socket_example PRIVATE ${CMAKE_SOURCE_DIR}/include) 