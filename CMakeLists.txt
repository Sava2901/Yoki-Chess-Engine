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

# Create bitboard test executable
add_executable(test_bitboards
    src/main/test_bitboards.cpp
)

# Create attack test executable
add_executable(attack_test
    src/main/attack_test.cpp
)

# Create move/undo test executable
add_executable(test_move_undo
    src/main/test_move_undo.cpp
)

# Create piece movement test executable
add_executable(test_piece_movement
    src/main/test_piece_movement.cpp
)

# Link bitboard library to bitboard test executable
target_link_libraries(test_bitboards bitboard)

# Link bitboard library to attack test executable
target_link_libraries(attack_test bitboard)

# Link bitboard library to move/undo test executable
target_link_libraries(test_move_undo bitboard)

# Link bitboard library to piece movement test executable
target_link_libraries(test_piece_movement bitboard)
