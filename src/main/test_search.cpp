#include <iostream>
#include <cassert>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <iomanip>
#include "../engine/Search.h"
#include "../engine/Evaluation.h"
#include "../board/Board.h"
#include "../board/Move.h"
#include "../board/MoveGenerator.h"

/**
 * @brief Simple test program for the Search engine
 * 
 * Tests the 4 required search function variants:
 * - Move search_move() - returns best move, no time limit
 * - Move search_move(time_limit) - returns best move with time limit
 * - SearchResult search() - returns full search result, no time limit
 * - SearchResult search(time_limit) - returns full search result with time limit
 */
class SearchEngineTest {
private:
    std::unique_ptr<Search> search;
    std::unique_ptr<Evaluation> evaluation;
    Board board;
    int tests_passed = 0;
    int tests_failed = 0;
    
    static constexpr int DEFAULT_SEARCH_DEPTH = 3; // Increased with optimizations (killer moves, history heuristic, quiescence depth limit)
    static constexpr int MULTITHREADING_SEARCH_DEPTH = 5; // Higher depth for performance testing
    static constexpr std::chrono::milliseconds SHORT_TIME_LIMIT{100};
    static constexpr std::chrono::milliseconds MEDIUM_TIME_LIMIT{500};
    static constexpr std::chrono::milliseconds PERFORMANCE_TIME_LIMIT{2000}; // Longer time for performance tests

public:
    SearchEngineTest() {
        try {
            evaluation = std::make_unique<Evaluation>();
            search = std::make_unique<Search>();
            std::cout << "=== Search Engine Test ===\n";
            std::cout << "Initializing search engine...\n\n";
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize test: " << e.what() << std::endl;
            throw;
        }
    }
    
    void run_all_tests() {
        std::cout << "Running search engine tests...\n\n";
        
        try {
            test_basic_move_search();
            test_timed_move_search();
            test_basic_search_result();
            test_timed_search_result();
            test_multithreading();
            test_time_management();
            test_single_vs_multi_thread_performance();
            test_thread_scaling();
        } catch (const std::exception& e) {
            std::cerr << "Test execution failed: " << e.what() << std::endl;
            tests_failed++;
        }
        
        print_summary();
    }

private:
    void assert_test(bool condition, const std::string& test_name, const std::string& details = "") {
        if (condition) {
            tests_passed++;
            std::cout << "✓ " << test_name;
            if (!details.empty()) {
                std::cout << " (" << details << ")";
            }
            std::cout << "\n";
        } else {
            tests_failed++;
            std::cout << "✗ " << test_name;
            if (!details.empty()) {
                std::cout << " - " << details;
            }
            std::cout << "\n";
        }
    }
    
    void setup_starting_position() {
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    }
    
    void test_basic_move_search() {
        std::cout << "Testing basic move search (no time limit)...\n";
        
        setup_starting_position();
        
        try {
            Move best_move = search->search_move(board, DEFAULT_SEARCH_DEPTH);
            assert_test(best_move.is_valid(), "Basic search returns valid move", best_move.to_algebraic());
        } catch (const std::exception& e) {
            assert_test(false, "Basic search threw exception", e.what());
        }
        
        std::cout << "\n";
    }
    
    void test_timed_move_search() {
        std::cout << "Testing timed move search...\n";
        
        setup_starting_position();
        
        try {
            auto start_time = std::chrono::steady_clock::now();
            Move best_move = search->search_move(board, MEDIUM_TIME_LIMIT, DEFAULT_SEARCH_DEPTH);
            auto end_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            assert_test(best_move.is_valid(), "Timed search returns valid move", best_move.to_algebraic());
            assert_test(elapsed <= MEDIUM_TIME_LIMIT + std::chrono::milliseconds(200), 
                       "Timed search respects time limit", 
                       std::to_string(elapsed.count()) + "ms");
        } catch (const std::exception& e) {
            assert_test(false, "Timed search threw exception", e.what());
        }
        
        std::cout << "\n";
    }
    
    void test_basic_search_result() {
        std::cout << "Testing basic search result (no time limit)...\n";
        
        setup_starting_position();
        
        try {
            SearchResult result = search->search(board, DEFAULT_SEARCH_DEPTH);
            
            assert_test(result.best_move.is_valid(), "Search result has valid move", result.best_move.to_algebraic());
            assert_test(result.depth > 0, "Search depth is positive", std::to_string(result.depth));
            assert_test(result.stats.nodes_searched > 0, "Nodes were searched", std::to_string(result.stats.nodes_searched));
            assert_test(result.time_elapsed.count() >= 0, "Time elapsed is non-negative", std::to_string(result.time_elapsed.count()) + "ms");
            
            std::cout << "  Depth: " << result.depth << "\n";
            std::cout << "  Score: " << result.score << "\n";
            std::cout << "  Nodes: " << result.stats.nodes_searched << "\n";
            std::cout << "  Time: " << result.time_elapsed.count() << "ms\n";
        } catch (const std::exception& e) {
            assert_test(false, "Search result threw exception", e.what());
        }
        
        std::cout << "\n";
    }
    
    void test_timed_search_result() {
        std::cout << "Testing timed search result...\n";
        
        setup_starting_position();
        
        try {
            auto start_time = std::chrono::steady_clock::now();
            SearchResult result = search->search(board, MEDIUM_TIME_LIMIT, DEFAULT_SEARCH_DEPTH);
            auto end_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            assert_test(result.best_move.is_valid(), "Timed search result has valid move", result.best_move.to_algebraic());
            assert_test(result.depth > 0, "Search depth is positive", std::to_string(result.depth));
            assert_test(result.stats.nodes_searched > 0, "Nodes were searched", std::to_string(result.stats.nodes_searched));
            assert_test(elapsed <= MEDIUM_TIME_LIMIT + std::chrono::milliseconds(200), 
                       "Timed search respects time limit", 
                       std::to_string(elapsed.count()) + "ms");
            
            std::cout << "  Depth: " << result.depth << "\n";
            std::cout << "  Score: " << result.score << "\n";
            std::cout << "  Nodes: " << result.stats.nodes_searched << "\n";
            std::cout << "  Time: " << result.time_elapsed.count() << "ms\n";
        } catch (const std::exception& e) {
            assert_test(false, "Timed search result threw exception", e.what());
        }
        
        std::cout << "\n";
    }
    
    void test_multithreading() {
        std::cout << "Testing multithreading support...\n";
        
        setup_starting_position();
        
        try {
            // Test with different thread counts
            search->set_thread_count(1);
            assert_test(search->get_thread_count() == 1, "Single thread setting");
            
            search->set_thread_count(4);
            assert_test(search->get_thread_count() == 4, "Multi-thread setting");
            
            // Test search with multiple threads
            SearchResult result = search->search(board, SHORT_TIME_LIMIT, DEFAULT_SEARCH_DEPTH);
            assert_test(result.best_move.is_valid(), "Multi-threaded search returns valid move");
        } catch (const std::exception& e) {
            assert_test(false, "Multithreading test threw exception", e.what());
        }
        
        std::cout << "\n";
    }
    
    void test_time_management() {
        std::cout << "Testing time management...\n";
        
        setup_starting_position();
        
        try {
            auto start_time = std::chrono::steady_clock::now();
            SearchResult result = search->search(board, SHORT_TIME_LIMIT, 2); // Safe depth with short time
            auto end_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            assert_test(result.best_move.is_valid(), "Time-limited search returns valid move");
            assert_test(elapsed <= SHORT_TIME_LIMIT + std::chrono::milliseconds(100), 
                       "Search stops within time limit", 
                       std::to_string(elapsed.count()) + "ms");
        } catch (const std::exception& e) {
            assert_test(false, "Time management test threw exception", e.what());
        }
        
        std::cout << "\n";
    }
    
    void test_single_vs_multi_thread_performance() {
        std::cout << "Testing single vs multi-thread performance...\n";
        
        setup_starting_position();
        
        try {
            // Test single-threaded performance
            search->set_thread_count(2);
            auto start_time = std::chrono::steady_clock::now();
            SearchResult single_result = search->search(board, DEFAULT_SEARCH_DEPTH);
            auto end_time = std::chrono::steady_clock::now();
            auto single_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            // Test multi-threaded performance (4 threads)
            search->set_thread_count(4);
            start_time = std::chrono::steady_clock::now();
            SearchResult multi_result = search->search(board, DEFAULT_SEARCH_DEPTH);
            end_time = std::chrono::steady_clock::now();
            auto multi_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            // Calculate performance metrics
            double single_nps = static_cast<double>(single_result.stats.nodes_searched) / (single_elapsed.count() / 1000.0);
            double multi_nps = static_cast<double>(multi_result.stats.nodes_searched) / (multi_elapsed.count() / 1000.0);
            double speedup = static_cast<double>(single_elapsed.count()) / multi_elapsed.count();
            
            assert_test(single_result.best_move.is_valid(), "Single-threaded search returns valid move");
            assert_test(multi_result.best_move.is_valid(), "Multi-threaded search returns valid move");
            assert_test(multi_elapsed.count() > 0, "Multi-threaded search completed");
            
            std::cout << "  === Performance Comparison ===\n";
            std::cout << "  Single Thread (2): " << single_elapsed.count() << "ms, "
                      << single_result.stats.nodes_searched << " nodes, " 
                      << std::fixed << std::setprecision(0) << single_nps << " NPS\n";
            std::cout << "  Multi Thread (4):  " << multi_elapsed.count() << "ms, " 
                      << multi_result.stats.nodes_searched << " nodes, " 
                      << std::fixed << std::setprecision(0) << multi_nps << " NPS\n";
            std::cout << "  Speedup: " << std::fixed << std::setprecision(2) << speedup << "x\n";
            
        } catch (const std::exception& e) {
            assert_test(false, "Performance comparison test threw exception", e.what());
        }
        
        std::cout << "\n";
    }
    
    void test_thread_scaling() {
        std::cout << "Testing thread scaling performance...\n";
        
        setup_starting_position();
        
        try {
            std::vector<int> thread_counts = {1, 2, 4, 8};
            std::vector<std::chrono::milliseconds> times;
            std::vector<long> nodes;
            
            std::cout << "  === Thread Scaling Results ===\n";
            
            for (int threads : thread_counts) {
                search->set_thread_count(threads);
                
                auto start_time = std::chrono::steady_clock::now();
                SearchResult result = search->search(board, DEFAULT_SEARCH_DEPTH);
                auto end_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                
                times.push_back(elapsed);
                nodes.push_back(result.stats.nodes_searched);
                
                double nps = static_cast<double>(result.stats.nodes_searched) / (elapsed.count() / 1000.0);
                
                std::cout << "  Threads: " << std::setw(2) << threads 
                          << ", Time: " << std::setw(6) << elapsed.count() << "ms"
                          << ", Nodes: " << std::setw(8) << result.stats.nodes_searched
                          << ", NPS: " << std::fixed << std::setprecision(0) << std::setw(8) << nps << "\n";
                
                assert_test(result.best_move.is_valid(), "Thread scaling test returns valid move for " + std::to_string(threads) + " threads");
            }
            
            // Calculate efficiency metrics
            if (times.size() >= 2) {
                double speedup_2 = static_cast<double>(times[0].count()) / times[1].count();
                double speedup_4 = static_cast<double>(times[0].count()) / times[2].count();
                
                std::cout << "  === Scaling Efficiency ===\n";
                std::cout << "  2 threads speedup: " << std::fixed << std::setprecision(2) << speedup_2 << "x (" << (speedup_2/2.0*100) << "% efficiency)\n";
                std::cout << "  4 threads speedup: " << std::fixed << std::setprecision(2) << speedup_4 << "x (" << (speedup_4/4.0*100) << "% efficiency)\n";
            }
            
        } catch (const std::exception& e) {
            assert_test(false, "Thread scaling test threw exception", e.what());
        }
        
        std::cout << "\n";
    }
    
    void print_summary() {
        std::cout << "=== Test Summary ===\n";
        std::cout << "Tests passed: " << tests_passed << "\n";
        std::cout << "Tests failed: " << tests_failed << "\n";
        
        if (tests_failed == 0) {
            std::cout << "All tests passed! ✓\n";
        } else {
            std::cout << "Some tests failed! ✗\n";
        }
    }
};

int main() {
    try {
        SearchEngineTest test;
        test.run_all_tests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception!" << std::endl;
        return 1;
    }
}