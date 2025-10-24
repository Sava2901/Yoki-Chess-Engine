# Test executables using Google Test

# Helper function to create a test with gtest
function(add_gtest_executable test_name source_file)
    add_executable(${test_name} ${source_file})
    target_include_directories(${test_name} PUBLIC ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(${test_name} GTest::gtest_main)
    
    # Parse additional libraries from ARGN
    foreach(lib ${ARGN})
        target_link_libraries(${test_name} ${lib})
    endforeach()
    
    # Register with CTest
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

# Board tests (depend on board library)
add_gtest_executable(test_bitboards test_bitboards.cpp board)
add_gtest_executable(test_attack test_attack.cpp board)
add_gtest_executable(test_move_undo test_move_undo.cpp board)
add_gtest_executable(test_piece_movement test_piece_movement.cpp board)

# Engine tests (depend on engine and board libraries)
add_gtest_executable(test_evaluation test_evaluation.cpp engine board)
add_gtest_executable(test_search test_search.cpp engine board atomic)
add_gtest_executable(test_search_fixes test_search_fixes.cpp engine board atomic)
add_gtest_executable(test_advanced_multithreading test_advanced_multithreading.cpp engine board atomic)
add_gtest_executable(test_performance_benchmark test_performance_benchmark.cpp engine board atomic)
add_gtest_executable(test_node_counting test_node_counting.cpp engine board atomic)
add_gtest_executable(test_legal_moves test_legal_moves.cpp engine board atomic)
add_gtest_executable(test_perft test_perft.cpp engine board atomic)
add_gtest_executable(test_node_debug test_node_debug.cpp engine board atomic)
add_gtest_executable(test_move_ordering test_move_ordering.cpp engine board atomic)
add_gtest_executable(test_move_ordering_correctness test_move_ordering_correctness.cpp engine board atomic)
add_gtest_executable(test_real_move_ordering test_real_move_ordering.cpp engine board atomic)
add_gtest_executable(test_pvs_explanation test_pvs_explanation.cpp engine board atomic)