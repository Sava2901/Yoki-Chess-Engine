# Board library - handles chess board representation and move generation
add_library(board
    Bitboard.cpp
    Board.cpp
    Move.cpp
    MoveGenerator.cpp
)

# Link to parent include directories (already set in root CMakeLists.txt)
target_include_directories(board PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)