cmake_minimum_required(VERSION 3.31.6)
project(TeeStream VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(TEESTREAM_BUILD_TESTS "Build TeeStream tests" ON)
option(TEESTREAM_BUILD_EXAMPLES "Build TeeStream examples" ON)
option(TEESTREAM_BUILD_BENCHMARKS "Build TeeStream benchmarks" ON)

# Library target
add_library(teestream
    src/TeeStream.cpp
)

target_include_directories(teestream
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Set up installation
include(GNUInstallDirs)
install(TARGETS teestream
    EXPORT teestreamTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export targets
install(EXPORT teestreamTargets
    FILE teestreamTargets.cmake
    NAMESPACE TeeStream::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/teestream
)

# Create and install config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/teestreamConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/teestreamConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/teestream
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/teestreamConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/teestreamConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/teestreamConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/teestream
)

# Benchmarks
if(TEESTREAM_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Examples
if(TEESTREAM_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(TEESTREAM_BUILD_TESTS)
    # Set up GoogleTest
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    # No Windows-specific settings
    FetchContent_MakeAvailable(googletest)
    
    enable_testing()
    add_subdirectory(tests)
endif()
