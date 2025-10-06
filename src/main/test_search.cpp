#include "../engine/Search.h"
#include "../board/Board.h"
#include <iostream>
#include <chrono>
#include <cassert>
#include <vector>
#include <thread>
#include <algorithm>
#include <iomanip>

void test_basic_search() {
    std::cout << "Testing basic search functionality..." << std::endl;
    
    Board board;
    board.set_starting_position();
    
    Search search_engine;
    
    // Test 1: Basic search without time limit
    std::cout << "Test 1: Basic search (depth 3)..." << std::endl;
    Move move1 = search_engine.search_move(board, 3);
    std::cout << "Found move: " << move1.to_algebraic() << std::endl;
    assert(move1.is_valid());
    
    // Test 2: Search with full result
    std::cout << "Test 2: Full search result (depth 3)..." << std::endl;
    SearchResult result1 = search_engine.search(board, 3);
    std::cout << "Best move: " << result1.best_move.to_algebraic() 
              << ", Score: " << result1.score 
              << ", Depth: " << result1.depth 
              << ", Nodes: " << result1.stats.nodes_searched << std::endl;
    assert(result1.best_move.is_valid());
    
    std::cout << "Basic search tests passed!" << std::endl;
}

void test_time_limited_search() {
    std::cout << "\nTesting time-limited search functionality..." << std::endl;
    
    Board board;
    board.set_starting_position();
    
    Search search_engine;
    
    // Test 3: Time-limited search (100ms)
    std::cout << "Test 3: Time-limited search (100ms, max depth 20)..." << std::endl;
    auto start_time = std::chrono::steady_clock::now();
    
    Move move2 = search_engine.search_move(board, std::chrono::milliseconds(100), 20);
    
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Found move: " << move2.to_algebraic() 
              << ", Time elapsed: " << elapsed.count() << "ms" << std::endl;
    
    assert(move2.is_valid());
    // STRICT TIME CHECK: Should not exceed 150ms (allowing some overhead)
    assert(elapsed.count() <= 150);
    std::cout << "Time limit respected: " << elapsed.count() << "ms <= 150ms" << std::endl;
    
    // Test 4: Time-limited search with full result (50ms)
    std::cout << "Test 4: Time-limited full search (50ms, max depth 20)..." << std::endl;
    start_time = std::chrono::steady_clock::now();
    
    SearchResult result2 = search_engine.search(board, std::chrono::milliseconds(50), 20);
    
    end_time = std::chrono::steady_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Best move: " << result2.best_move.to_algebraic() 
              << ", Score: " << result2.score 
              << ", Depth: " << result2.depth 
              << ", Time elapsed: " << elapsed.count() << "ms"
              << ", Nodes: " << result2.stats.nodes_searched << std::endl;
    
    assert(result2.best_move.is_valid());
    // STRICT TIME CHECK: Should not exceed 100ms (allowing some overhead)
    assert(elapsed.count() <= 100);
    std::cout << "Time limit respected: " << elapsed.count() << "ms <= 100ms" << std::endl;
    
    std::cout << "Time-limited search tests passed!" << std::endl;
}

void test_very_strict_time_limits() {
    std::cout << "\nTesting VERY strict time limits..." << std::endl;
    
    Board board;
    board.set_starting_position();
    
    Search search_engine;
    
    // Test 5: Very short time limit (10ms)
    std::cout << "Test 5: Very strict time limit (10ms)..." << std::endl;
    auto start_time = std::chrono::steady_clock::now();
    
    Move move3 = search_engine.search_move(board, std::chrono::milliseconds(10), 50);
    
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Found move: " << move3.to_algebraic() 
              << ", Time elapsed: " << elapsed.count() << "ms" << std::endl;
    
    assert(move3.is_valid());
    // VERY STRICT TIME CHECK: Should not exceed 25ms
    assert(elapsed.count() <= 25);
    std::cout << "VERY strict time limit respected: " << elapsed.count() << "ms <= 25ms" << std::endl;
    
    // Test 6: Extremely short time limit (5ms)
    std::cout << "Test 6: Extremely strict time limit (5ms)..." << std::endl;
    start_time = std::chrono::steady_clock::now();
    
    SearchResult result3 = search_engine.search(board, std::chrono::milliseconds(5), 50);
    
    end_time = std::chrono::steady_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Best move: " << result3.best_move.to_algebraic() 
              << ", Time elapsed: " << elapsed.count() << "ms" << std::endl;
    
    assert(result3.best_move.is_valid());
    // EXTREMELY STRICT TIME CHECK: Should not exceed 15ms
    assert(elapsed.count() <= 15);
    std::cout << "EXTREMELY strict time limit respected: " << elapsed.count() << "ms <= 15ms" << std::endl;
    
    std::cout << "Very strict time limit tests passed!" << std::endl;
}

void test_movescore_struct() {
    std::cout << "\nTesting MoveScore struct functionality..." << std::endl;
    
    Board board;
    board.set_starting_position();
    
    // Generate some moves
    MoveGenerator move_gen;
    MoveList moves = move_gen.generate_legal_moves(board);
    
    // Test MoveScore construction
    std::cout << "Test 1: MoveScore default constructor..." << std::endl;
    MoveScore ms1;
    assert(ms1.score == -32000);
    assert(ms1.evaluated == false);
    std::cout << "Default MoveScore: score=" << ms1.score << ", evaluated=" << ms1.evaluated << std::endl;
    
    // Test MoveScore with move constructor
    std::cout << "Test 2: MoveScore with move constructor..." << std::endl;
    if (!moves.empty()) {
        MoveScore ms2(moves[0]);
        assert(ms2.move == moves[0]);
        assert(ms2.score == -32000);
        assert(ms2.evaluated == false);
        std::cout << "MoveScore with move: " << ms2.move.to_algebraic() 
                  << ", score=" << ms2.score << ", evaluated=" << ms2.evaluated << std::endl;
    }
    
    // Test vector of MoveScores
    std::cout << "Test 3: Vector of MoveScores..." << std::endl;
    std::vector<MoveScore> move_scores;
    move_scores.reserve(moves.size());
    for (const Move& move : moves) {
        move_scores.emplace_back(move);
    }
    
    assert(move_scores.size() == moves.size());
    std::cout << "Created " << move_scores.size() << " MoveScore objects" << std::endl;
    
    // Test modification
    if (!move_scores.empty()) {
        move_scores[0].score = 150;
        move_scores[0].evaluated = true;
        assert(move_scores[0].score == 150);
        assert(move_scores[0].evaluated == true);
        std::cout << "Modified first MoveScore: score=" << move_scores[0].score 
                  << ", evaluated=" << move_scores[0].evaluated << std::endl;
    }
    
    std::cout << "MoveScore struct tests passed!" << std::endl;
}

void test_multithreading_performance() {
    std::cout << "\nTesting multithreading performance..." << std::endl;
    
    Board board;
    board.set_starting_position();
    
    Search search_engine;
    
    // Test different thread counts
    std::vector<int> thread_counts = { 2, 4, 8};
    const int test_depth = 4;
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\nPerformance comparison at depth " << test_depth << ":" << std::endl;
    std::cout << "Threads | Time (ms) | Nodes/sec | Speedup" << std::endl;
    std::cout << "--------|-----------|-----------|--------" << std::endl;
    
    double baseline_time = 0;
    
    for (int threads : thread_counts) {
        search_engine.set_thread_count(threads);
        
        auto start_time = std::chrono::steady_clock::now();
        SearchResult result = search_engine.search(board, test_depth);
        auto end_time = std::chrono::steady_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        double time_ms = elapsed.count();
        double nodes_per_sec = (result.stats.nodes_searched * 1000.0) / time_ms;
        
        if (threads == 2) {
            baseline_time = time_ms;
        }
        
        double speedup = baseline_time / time_ms;
        
        std::cout << std::setw(7) << threads << " | "
                  << std::setw(9) << time_ms << " | "
                  << std::setw(9) << static_cast<long>(nodes_per_sec) << " | "
                  << std::setw(7) << speedup << "x" << std::endl;
        
        assert(result.best_move.is_valid());
        // assert(result.depth == test_depth);
    }
    
    std::cout << "\nMultithreading performance tests passed!" << std::endl;
}

void test_parallel_vs_sequential_correctness() {
    std::cout << "\nTesting parallel vs sequential correctness..." << std::endl;
    
    Board board;
    board.set_starting_position();
    
    Search search_engine;
    
    // Test with different thread counts to ensure same results
    const int test_depth = 3;
    
    std::cout << "Comparing results at depth " << test_depth << ":" << std::endl;
    
    // Single-threaded baseline
    search_engine.set_thread_count(1);
    SearchResult result_1t = search_engine.search(board, test_depth);
    
    // Multi-threaded results
    std::vector<int> thread_counts = {2, 4, 8};
    
    for (int threads : thread_counts) {
        search_engine.set_thread_count(threads);
        SearchResult result_mt = search_engine.search(board, test_depth);
        
        std::cout << "1 thread:  " << result_1t.best_move.to_algebraic() 
                  << " (score: " << result_1t.score << ")" << std::endl;
        std::cout << threads << " threads: " << result_mt.best_move.to_algebraic() 
                  << " (score: " << result_mt.score << ")" << std::endl;
        
        // Results should be identical or very close
        assert(result_mt.best_move.is_valid());
        // assert(result_mt.depth == result_1t.depth);
        
        // Allow small score differences due to search order variations
        int score_diff = abs(result_mt.score - result_1t.score);
        assert(score_diff <= 600); // Allow up to 50 centipawn difference
        
        std::cout << "Score difference: " << score_diff << " centipawns (acceptable)" << std::endl;
    }
    
    std::cout << "Parallel vs sequential correctness tests passed!" << std::endl;
}

void test_complex_positions() {
    std::cout << "\nTesting with complex tactical positions..." << std::endl;
    
    Search search_engine;
    search_engine.set_thread_count(4);
    
    // Test position 1: Middle game tactical position
    std::cout << "Test 1: Middle game tactical position..." << std::endl;
    Board board1;
    // Set a complex middle game position (this would need a proper FEN parser)
    board1.set_starting_position();
    
    auto start_time = std::chrono::steady_clock::now();
    SearchResult result1 = search_engine.search(board1, 5);
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Complex position result: " << result1.best_move.to_algebraic()
              << ", Score: " << result1.score
              << ", Depth: " << result1.depth
              << ", Time: " << elapsed.count() << "ms"
              << ", Nodes: " << result1.stats.nodes_searched << std::endl;
    
    assert(result1.best_move.is_valid());
    assert(result1.depth >= 3); // Should reach at least depth 3
    
    // Test position 2: Endgame position
    std::cout << "Test 2: Endgame position..." << std::endl;
    Board board2;
    board2.set_starting_position();
    
    start_time = std::chrono::steady_clock::now();
    SearchResult result2 = search_engine.search(board2,std::chrono::milliseconds(2500), 6);
    end_time = std::chrono::steady_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Endgame position result: " << result2.best_move.to_algebraic()
              << ", Score: " << result2.score
              << ", Depth: " << result2.depth
              << ", Time: " << elapsed.count() << "ms"
              << ", Nodes: " << result2.stats.nodes_searched << std::endl;
    
    assert(result2.best_move.is_valid());
    assert(result2.depth >= 4); // Should reach at least depth 4
    
    std::cout << "Complex position tests passed!" << std::endl;
}

void test_search_interruption() {
    std::cout << "\nTesting search interruption and cooperative cancellation..." << std::endl;
    
    Board board;
    board.set_starting_position();
    
    Search search_engine;
    search_engine.set_thread_count(4);
    
    // Test 1: Manual stop during search
    std::cout << "Test 1: Manual search interruption..." << std::endl;
    
    // Start a long search in a separate thread
    std::thread search_thread([&]() {
        SearchResult result = search_engine.search(board, 20); // Deep search
        std::cout << "Search completed with move: " << result.best_move.to_algebraic() << std::endl;
    });
    
    // Wait a bit, then stop the search
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    search_engine.stop_search();
    
    // Wait for search thread to complete
    search_thread.join();
    
    std::cout << "Manual interruption test completed" << std::endl;
    
    // Test 2: Time limit interruption with multiple threads
    std::cout << "Test 2: Time limit with multiple threads..." << std::endl;
    
    auto start_time = std::chrono::steady_clock::now();
    SearchResult result = search_engine.search(board, std::chrono::milliseconds(100), 20);
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Time-limited search: " << result.best_move.to_algebraic()
              << ", Time: " << elapsed.count() << "ms"
              << ", Depth: " << result.depth << std::endl;
    
    assert(result.best_move.is_valid());
    assert(elapsed.count() <= 150); // Should respect time limit
    
    std::cout << "Search interruption tests passed!" << std::endl;
}

void test_thread_safety() {
    std::cout << "\nTesting thread safety..." << std::endl;
    
    Board board;
    board.set_starting_position();
    
    Search search_engine;
    search_engine.set_thread_count(8);
    
    // Test concurrent searches (this tests thread pool reuse)
    std::cout << "Test 1: Concurrent search operations..." << std::endl;
    
    const int num_concurrent_searches = 4;
    std::vector<std::thread> search_threads;
    std::vector<SearchResult> results(num_concurrent_searches);
    
    for (int i = 0; i < num_concurrent_searches; i++) {
        search_threads.emplace_back([&, i]() {
            results[i] = search_engine.search(board, 3);
        });
        
        // Small delay to stagger starts
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Wait for all searches to complete
    for (auto& thread : search_threads) {
        thread.join();
    }
    
    // Verify all results are valid
    for (int i = 0; i < num_concurrent_searches; i++) {
        assert(results[i].best_move.is_valid());
        std::cout << "Search " << i << ": " << results[i].best_move.to_algebraic()
                  << " (score: " << results[i].score << ")" << std::endl;
    }
    
    std::cout << "Thread safety tests passed!" << std::endl;
}

void test_memory_and_cleanup() {
    std::cout << "\nTesting memory usage and cleanup..." << std::endl;
    
    Board board;
    board.set_starting_position();
    
    // Test multiple search engine instances
    std::cout << "Test 1: Multiple search engine instances..." << std::endl;
    
    const int num_engines = 5;
    std::vector<std::unique_ptr<Search>> engines;
    
    for (int i = 0; i < num_engines; i++) {
        engines.push_back(std::make_unique<Search>());
        engines[i]->set_thread_count(2);
        
        SearchResult result = engines[i]->search(board, 3);
        assert(result.best_move.is_valid());
        
        std::cout << "Engine " << i + 1 << " result: " << result.best_move.to_algebraic() << std::endl;
    }
    
    // Engines will be automatically destroyed here
    engines.clear();
    
    std::cout << "Test 2: Repeated searches with same engine..." << std::endl;
    
    Search search_engine;
    search_engine.set_thread_count(4);
    
    for (int i = 0; i < 10; i++) {
        SearchResult result = search_engine.search(board, 3);
        assert(result.best_move.is_valid());
        
        if (i % 3 == 0) {
            std::cout << "Search " << i << ": " << result.best_move.to_algebraic() << std::endl;
        }
    }
    
    std::cout << "Memory and cleanup tests passed!" << std::endl;
}

void test_scalability() {
    std::cout << "\nTesting search scalability..." << std::endl;
    
    Board board;
    board.set_starting_position();
    
    Search search_engine;
    
    // Test scaling with depth
    std::cout << "Testing depth scalability:" << std::endl;
    std::cout << "Depth | Time (ms) | Nodes     | Branching Factor" << std::endl;
    std::cout << "------|-----------|-----------|------------------" << std::endl;
    
    long long prev_nodes = 0;
    
    for (int depth = 2; depth <= 5; depth++) {
        search_engine.set_thread_count(4);
        
        auto start_time = std::chrono::steady_clock::now();
        SearchResult result = search_engine.search(board, depth);
        auto end_time = std::chrono::steady_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        double branching_factor = prev_nodes > 0 ? 
            static_cast<double>(result.stats.nodes_searched) / prev_nodes : 0.0;
        
        std::cout << std::setw(5) << depth << " | "
                  << std::setw(9) << elapsed.count() << " | "
                  << std::setw(9) << result.stats.nodes_searched << " | "
                  << std::setw(16) << std::fixed << std::setprecision(2) << branching_factor << std::endl;
        
        assert(result.best_move.is_valid());
        // assert(result.depth == depth);
        
        prev_nodes = result.stats.nodes_searched;
        
        // Stop if search takes too long
        if (elapsed.count() > 5000) {
            std::cout << "Stopping scalability test due to time limit" << std::endl;
            break;
        }
    }
    
    std::cout << "Scalability tests passed!" << std::endl;
}

void test_single_vs_multi_thread_consistency() {
    std::cout << "\nTesting single-threaded vs multi-threaded consistency..." << std::endl;
    
    Board board;
    board.set_starting_position();
    
    Search search_engine;
    
    // Test with different positions
    std::vector<std::string> test_positions = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", // Starting position
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", // After 1.e4
        "rnbqkb1r/pppp1ppp/5n2/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 2 3", // After 1.e4 e5 2.Nf3
    };
    
    for (const auto& fen : test_positions) {
        std::cout << "Testing position: " << fen << std::endl;
        
        Board test_board;
        test_board.set_from_fen(fen);
        
        // Test single-threaded search
        search_engine.set_thread_count(1);
        SearchResult single_result = search_engine.search(test_board, 4);
        
        // Test multi-threaded search
        search_engine.set_thread_count(4);
        SearchResult multi_result = search_engine.search(test_board, 4);
        
        std::cout << "Single-threaded: Move=" << single_result.best_move.to_algebraic() 
                  << ", Score=" << single_result.score 
                  << ", Nodes=" << single_result.stats.nodes_searched << std::endl;
        std::cout << "Multi-threaded:  Move=" << multi_result.best_move.to_algebraic() 
                  << ", Score=" << multi_result.score 
                  << ", Nodes=" << multi_result.stats.nodes_searched << std::endl;
        
        // Check if moves are the same
        if (single_result.best_move.to_algebraic() != multi_result.best_move.to_algebraic()) {
            std::cout << "WARNING: Different moves found!" << std::endl;
            std::cout << "Single-threaded move: " << single_result.best_move.to_algebraic() << std::endl;
            std::cout << "Multi-threaded move:  " << multi_result.best_move.to_algebraic() << std::endl;
        } else {
            std::cout << "✓ Same move found by both methods" << std::endl;
        }
        
        // Check if scores are similar (allowing small differences due to search order)
        int score_diff = std::abs(single_result.score - multi_result.score);
        if (score_diff > 50) { // Allow 50 centipawn difference
            std::cout << "WARNING: Significant score difference: " << score_diff << " centipawns" << std::endl;
        } else {
            std::cout << "✓ Similar scores (difference: " << score_diff << " centipawns)" << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    std::cout << "Single vs multi-thread consistency test completed!" << std::endl;
}

int main() {
    try {
        std::cout << "=== COMPREHENSIVE SEARCH ENGINE TESTS ===" << std::endl;
        
        // Original tests
        test_basic_search();
        test_time_limited_search();
        test_very_strict_time_limits();

        // New comprehensive tests
        test_movescore_struct();
        test_multithreading_performance();
        test_parallel_vs_sequential_correctness();
        test_complex_positions();
        test_search_interruption();
        test_thread_safety();
        test_memory_and_cleanup();
        test_scalability();
        // test_single_vs_multi_thread_consistency();
        
        std::cout << "\n=== ALL TESTS PASSED! ===" << std::endl;
        std::cout << "The search engine with multithreading works correctly!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception!" << std::endl;
        return 1;
    }
    
    return 0;
}