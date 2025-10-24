cmake_minimum_required(VERSION 3.10)
project(YokiChessEngine)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

## For better cpu's: uncomment
## Set compiler flags for optimization and BMI2 support
#if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
#    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mbmi2 -DNDEBUG")
#    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -march=native -mbmi2")
#endif()

# Add include directories
include_directories(include)

# Google Test setup
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)