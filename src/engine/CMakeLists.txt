# Engine library - handles search, evaluation, and transposition table
add_library(engine
    Engine.cpp
    Evaluation.cpp
    Search.cpp
    TranspositionTable.cpp
)

# Link to parent include directories and board library
target_include_directories(engine PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Link with board library
target_link_libraries(engine board)

# Link with atomic library for thread-safe operations
target_link_libraries(engine atomic)