cmake_minimum_required(VERSION 3.25)

project(library_management
    VERSION 0.1.0
    DESCRIPTION "Console library management system"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)          # -std=c++20, not -std=gnu++20
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # lets clang-tidy and IDEs see build flags

option(LIBRARY_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(LIBRARY_BUILD_TESTS "Build unit tests" ON)

include(cmake/Warnings.cmake)
include(FetchContent)

# std::expected-compatible type that works on C++20 toolchains (see src/domain/error.hpp).
FetchContent_Declare(
    tl_expected
    GIT_REPOSITORY https://github.com/TartanLlama/expected.git
    GIT_TAG        v1.1.0)
set(EXPECTED_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(EXPECTED_BUILD_PACKAGE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(tl_expected)

# SQLite ships with macOS; on Linux install libsqlite3-dev.
find_package(SQLite3 REQUIRED)

# libsodium (password hashing), built from source by CMake so nothing needs installing.
FetchContent_Declare(
    libsodium
    GIT_REPOSITORY https://github.com/robinlinden/libsodium-cmake.git
    GIT_TAG        9b2848dfc1b917a9410f0de9d81059b26cbfaa8d)
set(SODIUM_DISABLE_TESTS ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(libsodium)

add_subdirectory(src)

if(LIBRARY_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(tests)
endif()
