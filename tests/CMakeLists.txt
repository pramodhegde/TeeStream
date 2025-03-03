find_package(GTest QUIET)

if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Add test executable
add_executable(teestream_tests
    test_teestream.cpp
)

# Include directories
target_include_directories(teestream_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Link against our library and GoogleTest
target_link_libraries(teestream_tests
    PRIVATE
        teestream
        gtest
        gtest_main
        pthread
)

# Register tests
include(GoogleTest)
gtest_discover_tests(teestream_tests) 