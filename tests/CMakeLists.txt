# Test executables

# Create bitboard test executable
add_executable(test_bitboards
    test_bitboards.cpp
)

# Create attack test executable
add_executable(attack_test
    attack_test.cpp
)

# Create move/undo test executable
add_executable(test_move_undo
    test_move_undo.cpp
)

# Create piece movement test executable
add_executable(test_piece_movement
    test_piece_movement.cpp
)

# Create evaluation test executable
add_executable(test_evaluation
    test_evaluation.cpp
)

# Create search test executable
add_executable(test_search
    test_search.cpp
)

# Create search fixes test executable
add_executable(test_search_fixes
    test_search_fixes.cpp
)

# Link to parent include directories for all test executables
target_include_directories(test_bitboards PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(attack_test PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_move_undo PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_piece_movement PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_evaluation PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_search PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_search_fixes PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Link board library to basic board tests
target_link_libraries(test_bitboards board)
target_link_libraries(attack_test board)
target_link_libraries(test_move_undo board)
target_link_libraries(test_piece_movement board)

# Link both engine and board libraries to engine tests
target_link_libraries(test_evaluation engine board)
target_link_libraries(test_search engine board atomic)
target_link_libraries(test_search_fixes engine board atomic)