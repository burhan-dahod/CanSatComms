cmake_minimum_required(VERSION 3.14)
project(CanSatComms)

# Enforce modern C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Build your communications module as a library
add_library(comms_lib src/comms.cpp)
target_include_directories(comms_lib PUBLIC include)

# 2. Build the standalone executable (DISABLED FOR TESTING)
# add_executable(cansat_main src/main.cpp)
# target_link_libraries(cansat_main PRIVATE comms_lib)

# 3. Fetch Google Test directly from GitHub
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

# 4. Include the tests subdirectory
enable_testing()
add_subdirectory(tests)