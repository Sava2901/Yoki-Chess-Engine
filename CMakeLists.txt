cmake_minimum_required(VERSION 3.10)
project(YokiChessEngine)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories
include_directories(src)

# Create engine library
add_library(engine
    src/engine/Engine.cpp
    src/engine/Engine.h
    src/engine/Search.cpp
    src/engine/Search.h
    src/engine/Evaluation.cpp
    src/engine/Evaluation.h
        src/board/Move.cpp
    src/board/Move.h
)

# Create bitboard library
add_library(bitboard
    src/board/Bitboard.cpp
    src/board/Bitboard.h
        src/board/Board.cpp
        src/board/Board.h
        src/board/MoveGenerator.cpp
        src/board/MoveGenerator.h
    src/board/Move.cpp
    src/board/Move.h
)

# Create test executable
add_executable(yokichess_test
    src/main/test.cpp
)

# Create bitboard test executable
add_executable(test_bitboards
    src/board/test_bitboards.cpp
)

# Link engine library to test executable
target_link_libraries(yokichess_test engine)

# Link bitboard library to bitboard test executable
target_link_libraries(test_bitboards bitboard)

# Set output directory for executable
set_target_properties(yokichess_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Optional: Add compiler flags for debugging
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(engine PRIVATE -g -O0)
    target_compile_options(yokichess_test PRIVATE -g -O0)
endif()