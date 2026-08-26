cmake_minimum_required(VERSION 3.14)
project(CanSatComms)

# Enforce modern C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build communications module as a library
add_library(comms_lib src/comms.cpp)
target_include_directories(comms_lib PUBLIC include)

# Fetch Google Test directly from GitHub
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

# Include the tests subdirectory
enable_testing()
add_subdirectory(tests)