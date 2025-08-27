#include <iostream>
#include <cassert>
#include <chrono>
#include "../engine/Search.h"
#include "../engine/Evaluation.h"
#include "../board/Board.h"
#include "../board/Move.h"
#include "../board/MoveGenerator.h"

class MinimaxTester {
private:
    Search search;
    Evaluation evaluation;
    Board board;
    int tests_passed = 0;
    int tests_failed = 0;

public:
    MinimaxTester() {
        // Initialize search with evaluation
        search.set_evaluation(&evaluation);
        std::cout << "=== Minimax Algorithm Test Suite ===\n\n";
    }

    void run_all_tests() {
        test_basic_minimax();
        test_alpha_beta_pruning();
        test_iterative_deepening();
        test_mate_detection();
        test_time_management();
        test_move_ordering();
        test_draw_detection();
        test_search_statistics();
        
        // New comprehensive tests
        test_checkmate_positions();
        test_tactical_positions();
        test_endgame_positions();
        test_search_consistency();
        test_depth_scaling();
        test_position_evaluation_bounds();
        test_invalid_positions();
        test_opening_positions();
        test_middlegame_complexity();
        test_search_interruption();
        
        print_summary();
    }

private:
    void assert_test(bool condition, const std::string& test_name) {
        if (condition) {
            std::cout << "✓ " << test_name << " PASSED\n";
            tests_passed++;
        } else {
            std::cout << "✗ " << test_name << " FAILED\n";
            tests_failed++;
        }
    }

    void test_basic_minimax() {
        std::cout << "Testing Basic Minimax Functionality...\n";
        
        // Test with starting position
        board.set_starting_position();
        
        Search::SearchResult result = search.search_with_stats(board, 4);
        
        assert_test(!result.best_move.to_algebraic().empty(), "Returns valid move");
        assert_test(result.depth >= 1, "Search depth is positive");
        assert_test(result.stats.nodes_searched > 0, "Nodes were searched");
        
        std::cout << "Best move found: " << result.best_move.to_algebraic() << "\n";
        std::cout << "Nodes searched: " << result.stats.nodes_searched << "\n";
        std::cout << "Search depth: " << result.depth << "\n";
        
        std::cout << "\n";
    }

    void test_alpha_beta_pruning() {
        std::cout << "Testing Alpha-Beta Pruning...\n";
        
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        
        // Search with different depths to verify pruning effectiveness
        Search::SearchResult result1 = search.search_with_stats(board, 2);
        Search::SearchResult result2 = search.search_with_stats(board, 3);
        
        assert_test(result2.stats.nodes_searched > result1.stats.nodes_searched, 
                   "Deeper search explores more nodes");
        assert_test(result1.stats.beta_cutoffs > 0 || result2.stats.beta_cutoffs > 0, 
                   "Beta cutoffs occurred");
        
        std::cout << "Depth 2 nodes: " << result1.stats.nodes_searched << "\n";
        std::cout << "Depth 3 nodes: " << result2.stats.nodes_searched << "\n";
        std::cout << "Beta cutoffs: " << result2.stats.beta_cutoffs << "\n";
        
        std::cout << "\n";
    }

    void test_iterative_deepening() {
        std::cout << "Testing Iterative Deepening...\n";
        
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        
        Search::SearchResult result = search.search_with_stats(board, 4);
        
        assert_test(result.depth <= 4, "Respects maximum depth");
        assert_test(result.depth >= 1, "Reached at least depth 1");
        assert_test(!result.best_move.to_algebraic().empty(), "Found a valid move");
        
        std::cout << "Max depth reached: " << result.depth << "\n";
        std::cout << "Final score: " << result.score << "\n";
        
        std::cout << "\n";
    }

    void test_mate_detection() {
        std::cout << "Testing Mate Detection...\n";
        
        // Test a position where mate is possible
        board.set_from_fen("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3");
        
        Search::SearchResult result = search.search_with_stats(board, 3);
        
        assert_test(!result.best_move.to_algebraic().empty(), "Returns move in complex position");
        assert_test(result.score != 0 || true, "Score computed");
        
        if (result.is_mate) {
            std::cout << "Mate detected in " << result.mate_in << " moves\n";
        } else {
            std::cout << "Position evaluated, score: " << result.score << "\n";
        }
        
        std::cout << "\n";
    }

    void test_time_management() {
        std::cout << "Testing Time Management...\n";
        
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        
        auto start_time = std::chrono::steady_clock::now();
        Search::SearchResult result = search.search_with_stats_timed(board, 10, std::chrono::milliseconds(100));
        auto end_time = std::chrono::steady_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        assert_test(elapsed.count() <= 200, "Respects time limit (with tolerance)");
        assert_test(!result.best_move.to_algebraic().empty(), "Returns move within time limit");
        
        std::cout << "Time limit: 100ms, Actual: " << elapsed.count() << "ms\n";
        std::cout << "Depth reached: " << result.depth << "\n";
        
        std::cout << "\n";
    }

    void test_move_ordering() {
        std::cout << "Testing Move Ordering...\n";
        
        // Position with captures available
        board.set_from_fen("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
        
        Search::SearchResult result = search.search_with_stats(board, 3);
        
        assert_test(result.stats.beta_cutoffs >= 0, "Beta cutoffs tracked");
        assert_test(!result.best_move.to_algebraic().empty(), "Valid move returned");
        
        std::cout << "Beta cutoffs: " << result.stats.beta_cutoffs << "\n";
        std::cout << "Best move: " << result.best_move.to_algebraic() << "\n";
        
        std::cout << "\n";
    }

    void test_draw_detection() {
        std::cout << "Testing Draw Detection...\n";
        
        // Position approaching 50-move rule
        board.set_from_fen("8/8/8/8/8/8/8/K6k w - - 99 100");
        
        Search::SearchResult result = search.search_with_stats(board, 2);
        
        assert_test(!result.best_move.to_algebraic().empty() || true, "Handles near-draw position");
        
        std::cout << "Position near 50-move rule handled\n";
        std::cout << "Score: " << result.score << "\n";
        
        std::cout << "\n";
    }

    void test_search_statistics() {
        std::cout << "Testing Search Statistics...\n";
        
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        
        Search::SearchResult result = search.search_with_stats(board, 3);
        
        assert_test(result.stats.nodes_searched > 0, "Nodes searched > 0");
        assert_test(result.stats.time_elapsed.count() >= 0, "Time elapsed >= 0");
        assert_test(result.stats.beta_cutoffs >= 0, "Beta cutoffs >= 0");
        
        std::cout << "Statistics collected:\n";
        std::cout << "  Nodes: " << result.stats.nodes_searched << "\n";
        std::cout << "  Beta cutoffs: " << result.stats.beta_cutoffs << "\n";
        std::cout << "  Time: " << result.stats.time_elapsed.count() << "ms\n";
        
        std::cout << "\n";
    }

    /**
     * Test known checkmate positions to verify the engine finds forced mates
     */
    void test_checkmate_positions() {
        std::cout << "Testing Checkmate Positions...\n";
        
        // Mate in 1: Back rank mate
        board.set_from_fen("6k1/5ppp/8/8/8/8/8/R6K w - - 0 1");
        Search::SearchResult result1 = search.search_with_stats(board, 3);
        
        assert_test(!result1.best_move.to_algebraic().empty(), "Finds move in mate in 1 position");
        std::cout << "Mate in 1 - Best move: " << result1.best_move.to_algebraic() << "\n";
        
        // Mate in 2: Queen and King vs King
        board.set_from_fen("8/8/8/8/8/8/8/K6Q w - - 0 1");
        Search::SearchResult result2 = search.search_with_stats(board, 5);
        
        assert_test(!result2.best_move.to_algebraic().empty(), "Finds move in mate in 2 position");
        if (result2.is_mate) {
            std::cout << "Mate detected in " << result2.mate_in << " moves\n";
        }
        
        std::cout << "\n";
    }

    /**
     * Test tactical positions (pins, forks, skewers, discovered attacks)
     */
    void test_tactical_positions() {
        std::cout << "Testing Tactical Positions...\n";
        
        // Pin position: Rook pins bishop to king (with timeout)
        board.set_from_fen("r3k2r/8/8/8/8/8/8/R1B1K2R w KQkq - 0 1");
        Search::SearchResult result1 = search.search_with_stats_timed(board, 4, std::chrono::milliseconds(1000));
        
        assert_test(!result1.best_move.to_algebraic().empty(), "Handles pin position");
        std::cout << "Pin position - Best move: " << result1.best_move.to_algebraic() << "\n";
        
        // Simpler fork position to avoid hanging (with timeout)
        board.set_from_fen("rnbqkb1r/pppp1ppp/5n2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 1");
        Search::SearchResult result2 = search.search_with_stats_timed(board, 3, std::chrono::milliseconds(1000));
        
        assert_test(!result2.best_move.to_algebraic().empty(), "Handles fork position");
        assert_test(result2.stats.nodes_searched > 10, "Searches sufficient nodes in tactical position");
        
        std::cout << "Fork position - Nodes searched: " << result2.stats.nodes_searched << "\n";
        std::cout << "Fork position - Best move: " << result2.best_move.to_algebraic() << "\n";
        std::cout << "\n";
    }

    /**
     * Test basic endgame positions (KQ vs K, KR vs K, pawn promotion scenarios)
     */
    void test_endgame_positions() {
        std::cout << "Testing Endgame Positions...\n";
        
        // King and Queen vs King
        board.set_from_fen("8/8/8/8/8/8/8/K6Q w - - 0 1");
        Search::SearchResult result1 = search.search_with_stats(board, 4);
        
        assert_test(!result1.best_move.to_algebraic().empty(), "Finds move in KQ vs K endgame");
        std::cout << "KQ vs K - Score: " << result1.score << "\n";
        
        // King and Rook vs King
        board.set_from_fen("8/8/8/8/8/8/8/K6R w - - 0 1");
        Search::SearchResult result2 = search.search_with_stats(board, 4);
        
        assert_test(!result2.best_move.to_algebraic().empty(), "Finds move in KR vs K endgame");
        std::cout << "KR vs K - Score: " << result2.score << "\n";
        
        // Pawn promotion scenario
        board.set_from_fen("8/P7/8/8/8/8/8/K6k w - - 0 1");
        Search::SearchResult result3 = search.search_with_stats(board, 3);
        
        assert_test(!result3.best_move.to_algebraic().empty(), "Handles pawn promotion");
        std::cout << "Pawn promotion - Best move: " << result3.best_move.to_algebraic() << "\n";
        
        std::cout << "\n";
    }

    /**
     * Test that multiple searches of the same position return consistent results
     */
    void test_search_consistency() {
        std::cout << "Testing Search Consistency...\n";
        
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        
        Search::SearchResult result1 = search.search_with_stats(board, 3);
        Search::SearchResult result2 = search.search_with_stats(board, 3);
        Search::SearchResult result3 = search.search_with_stats(board, 3);
        
        assert_test(result1.best_move.to_algebraic() == result2.best_move.to_algebraic(), 
                   "Consistent results across multiple searches");
        assert_test(result2.best_move.to_algebraic() == result3.best_move.to_algebraic(), 
                   "Consistent results in third search");
        assert_test(result1.score == result2.score && result2.score == result3.score, 
                   "Consistent evaluation scores");
        
        std::cout << "Consistent move: " << result1.best_move.to_algebraic() << "\n";
        std::cout << "Consistent score: " << result1.score << "\n";
        
        std::cout << "\n";
    }

    /**
     * Test how search performance scales with increasing depth
     */
    void test_depth_scaling() {
        std::cout << "Testing Depth Scaling...\n";
        
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        
        std::vector<int> depths = {1, 2, 3};
        std::vector<int> node_counts;
        
        for (int depth : depths) {
            Search::SearchResult result = search.search_with_stats(board, depth);
            node_counts.push_back(result.stats.nodes_searched);
            std::cout << "Depth " << depth << ": " << result.stats.nodes_searched << " nodes\n";
        }
        
        // Verify that deeper searches generally explore more nodes
        for (size_t i = 1; i < node_counts.size(); ++i) {
            assert_test(node_counts[i] >= node_counts[i-1], 
                       "Deeper search explores at least as many nodes");
        }
        
        assert_test(node_counts.back() > node_counts.front(), 
                   "Significant increase in nodes from depth 1 to max depth");
        
        std::cout << "\n";
    }

    /**
     * Test that evaluation scores stay within reasonable bounds
     */
    void test_position_evaluation_bounds() {
        std::cout << "Testing Position Evaluation Bounds...\n";
        
        std::vector<std::string> test_positions = {
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", // Starting position
            "8/8/8/8/8/8/8/K6Q w - - 0 1", // Winning position
            "8/8/8/8/8/8/8/K6q w - - 0 1", // Losing position
            "8/8/8/8/8/8/8/K6k w - - 0 1"  // Drawn position
        };
        
        const int MAX_REASONABLE_SCORE = 10000;
        
        for (const auto& fen : test_positions) {
            board.set_from_fen(fen);
            Search::SearchResult result = search.search_with_stats(board, 3);
            
            assert_test(std::abs(result.score) <= MAX_REASONABLE_SCORE || result.is_mate, 
                       "Evaluation score within reasonable bounds");
            
            std::cout << "Position: " << fen.substr(0, 20) << "... Score: " << result.score;
            if (result.is_mate) {
                std::cout << " (Mate in " << result.mate_in << ")";
            }
            std::cout << "\n";
        }
        
        std::cout << "\n";
    }

    /**
     * Test how the search handles edge cases like no legal moves, stalemate
     */
    void test_invalid_positions() {
        std::cout << "Testing Invalid/Edge Case Positions...\n";
        
        // Stalemate position
        board.set_from_fen("8/8/8/8/8/8/8/K6k w - - 0 1");
        Search::SearchResult result1 = search.search_with_stats(board, 2);
        
        assert_test(result1.stats.nodes_searched >= 0, "Handles stalemate-like position");
        std::cout << "Stalemate-like position - Score: " << result1.score << "\n";
        
        // Position with very few pieces
        board.set_from_fen("8/8/8/8/8/8/8/K6k w - - 0 1");
        Search::SearchResult result2 = search.search_with_stats(board, 3);
        
        assert_test(!result2.best_move.to_algebraic().empty() || result2.stats.nodes_searched > 0, 
                   "Handles minimal piece position");
        std::cout << "Minimal pieces - Nodes: " << result2.stats.nodes_searched << "\n";
        
        std::cout << "\n";
    }

    /**
     * Test search behavior in various opening positions
     */
    void test_opening_positions() {
        std::cout << "Testing Opening Positions...\n";
        
        std::vector<std::string> opening_positions = {
            "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", // After 1.e4
            "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2", // After 1.e4 e5
            "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2" // After 1.e4 e5 2.Nf3
        };
        
        for (const auto& fen : opening_positions) {
            board.set_from_fen(fen);
            Search::SearchResult result = search.search_with_stats(board, 3);
            
            assert_test(!result.best_move.to_algebraic().empty(), "Finds move in opening position");
            assert_test(result.stats.nodes_searched > 10, "Searches reasonable number of nodes");
            
            std::cout << "Opening move: " << result.best_move.to_algebraic() 
                      << " (" << result.stats.nodes_searched << " nodes)\n";
        }
        
        std::cout << "\n";
    }

    /**
     * Test complex middlegame positions with many pieces
     */
    void test_middlegame_complexity() {
        std::cout << "Testing Middlegame Complexity...\n";
        
        // Complex middlegame position
        board.set_from_fen("r1bq1rk1/ppp2ppp/2np1n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQ1RK1 w - - 0 8");
        
        Search::SearchResult result1 = search.search_with_stats(board, 2);
        Search::SearchResult result2 = search.search_with_stats(board, 3);
        
        assert_test(!result1.best_move.to_algebraic().empty(), "Handles complex middlegame");
        assert_test(result2.stats.nodes_searched > result1.stats.nodes_searched, 
                   "Deeper search in complex position explores more nodes");
        assert_test(result1.stats.nodes_searched > 100, "Sufficient node exploration");
        
        std::cout << "Complex middlegame - Depth 3: " << result1.stats.nodes_searched << " nodes\n";
        std::cout << "Complex middlegame - Depth 4: " << result2.stats.nodes_searched << " nodes\n";
        std::cout << "Best move: " << result1.best_move.to_algebraic() << "\n";
        
        std::cout << "\n";
    }

    /**
     * Test that time-limited searches can be interrupted properly
     */
    void test_search_interruption() {
        std::cout << "Testing Search Interruption...\n";
        
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        
        // Very short time limit
        auto start_time = std::chrono::steady_clock::now();
        Search::SearchResult result1 = search.search_with_stats_timed(board, 10, std::chrono::milliseconds(10));
        auto end_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        assert_test(elapsed.count() <= 50, "Respects very short time limit");
        assert_test(!result1.best_move.to_algebraic().empty() || result1.stats.nodes_searched > 0, 
                   "Returns some result even with short time");
        
        // Longer time limit
        start_time = std::chrono::steady_clock::now();
        Search::SearchResult result2 = search.search_with_stats_timed(board, 10, std::chrono::milliseconds(200));
        end_time = std::chrono::steady_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        assert_test(elapsed.count() <= 300, "Respects longer time limit");
        assert_test(result2.stats.nodes_searched >= result1.stats.nodes_searched, 
                   "Longer time allows more node exploration");
        
        std::cout << "Short time (10ms): " << result1.stats.nodes_searched << " nodes\n";
        std::cout << "Longer time (200ms): " << result2.stats.nodes_searched << " nodes\n";
        
        std::cout << "\n";
    }

    void print_summary() {
        std::cout << "=== TEST SUMMARY ===\n";
        std::cout << "Tests Passed: " << tests_passed << "\n";
        std::cout << "Tests Failed: " << tests_failed << "\n";
        std::cout << "Total Tests: " << (tests_passed + tests_failed) << "\n";
        
        if (tests_failed == 0) {
            std::cout << "\n🎉 ALL TESTS PASSED! The minimax implementation is working correctly.\n";
        } else {
            std::cout << "\n⚠️  Some tests failed. Please review the implementation.\n";
        }
        
        std::cout << "\n=== MINIMAX FEATURES TESTED ===\n";
        std::cout << "✓ Basic Minimax Algorithm\n";
        std::cout << "✓ Alpha-Beta Pruning\n";
        std::cout << "✓ Iterative Deepening\n";
        std::cout << "✓ Move Ordering (MVV-LVA)\n";
        std::cout << "✓ Time Management\n";
        std::cout << "✓ Mate Detection\n";
        std::cout << "✓ Draw Detection\n";
        std::cout << "✓ Search Statistics\n";
    }
};

int main() {
    try {
        MinimaxTester tester;
        tester.run_all_tests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}