#include "../engine/Evaluation.h"
#include "../board/Board.h"
#include "../board/MoveGenerator.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <string>
#include <cassert>
#include <random>
#include <unordered_set>
#include <algorithm>
#include <climits>

void test_basic_evaluation() {
    std::cout << "=== Testing Basic Evaluation ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    // Test starting position
    board.set_starting_position();
    int start_eval = eval.evaluate(board);
    std::cout << "Starting position evaluation: " << start_eval << std::endl;
    
    // Test material imbalance
    board.set_from_fen("rnbqkb1r/pppppppp/5n2/8/8/5N2/PPPPPPPP/RNBQKB1R w KQkq - 0 1");
    int material_test = eval.evaluate(board);
    std::cout << "Equal material position: " << material_test << std::endl;
    
    // Test with queen advantage
    board.set_from_fen("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1");
    int queen_advantage = eval.evaluate(board);
    std::cout << "Normal opening position: " << queen_advantage << std::endl;
    
    std::cout << std::endl;
}

void test_evaluation_breakdown() {
    std::cout << "=== Testing Evaluation Breakdown ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    // Test starting position breakdown
    std::cout << "Starting possition:" << std::endl;
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    eval.print_evaluation_breakdown(board);
    
    std::cout << std::endl;
    
    // Test endgame position
    board.set_from_fen("8/8/8/8/8/8/4K3/4k3 w - - 0 1");
    std::cout << "King vs King endgame:" << std::endl;
    eval.print_evaluation_breakdown(board);
    
    std::cout << std::endl;
}

void test_game_phases() {
    std::cout << "=== Testing Game Phase Detection ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    // Opening position
    board.set_starting_position();
    GamePhase phase = eval.get_game_phase(board);
    std::cout << "Starting position phase: " << 
        (phase == OPENING ? "Opening" : phase == MIDDLEGAME ? "Middlegame" : "Endgame") << std::endl;
    
    // Middlegame position
    board.set_from_fen("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 1");
    phase = eval.get_game_phase(board);
    std::cout << "Middlegame position phase: " << 
        (phase == OPENING ? "Opening" : phase == MIDDLEGAME ? "Middlegame" : "Endgame") << std::endl;
    
    // Endgame position
    board.set_from_fen("8/8/8/8/8/8/4K3/4k3 w - - 0 1");
    phase = eval.get_game_phase(board);
    std::cout << "King endgame phase: " << 
        (phase == OPENING ? "Opening" : phase == MIDDLEGAME ? "Middlegame" : "Endgame") << std::endl;
    
    std::cout << std::endl;
}

void test_zobrist_hashing() {
    std::cout << "=== Testing Zobrist Hashing ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    // Test hash consistency
    board.set_starting_position();
    uint64_t hash1 = eval.compute_zobrist_hash(board);
    uint64_t hash2 = eval.compute_zobrist_hash(board);
    
    std::cout << "Hash consistency test: " << (hash1 == hash2 ? "PASSED" : "FAILED") << std::endl;
    std::cout << "Starting position hash: 0x" << std::hex << hash1 << std::dec << std::endl;
    
    // Test different positions have different hashes
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    uint64_t hash3 = eval.compute_zobrist_hash(board);
    
    std::cout << "Different position hash: 0x" << std::hex << hash3 << std::dec << std::endl;
    std::cout << "Hash difference test: " << (hash1 != hash3 ? "PASSED" : "FAILED") << std::endl;
    
    std::cout << std::endl;
}

void test_incremental_evaluation() {
    std::cout << "=== Testing Incremental Evaluation ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    board.set_starting_position();
    eval.initialize_incremental_eval(board);
    
    // Get initial evaluation
    int initial_eval = eval.evaluate(board);
    std::cout << "Initial evaluation: " << initial_eval << std::endl;
    
    // Note: To fully test incremental evaluation, we would need to make moves
    // and compare incremental vs full evaluation. This requires move generation
    // which might not be fully implemented yet.
    
    std::cout << "Incremental evaluation initialized successfully" << std::endl;
    std::cout << std::endl;
}

void test_pawn_structure() {
    std::cout << "=== Testing Pawn Structure Evaluation ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    // Test isolated pawns
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/4P3/8/PPP3PP/RNBQKBNR w KQkq - 0 1");
    board.print();
    std::cout << "Isolated pawn position:" << std::endl;
    eval.print_evaluation_breakdown(board);
    
    // Test passed pawns
    board.set_from_fen("rnbqkbnr/8/4P3/8/3pp3/8/PPP3PP/RNBQKBNR w KQkq - 0 1");
    board.print();
    std::cout << "Passed pawn position:" << std::endl;
    eval.print_evaluation_breakdown(board);
    
    std::cout << std::endl;
}

void test_performance() {
    std::cout << "=== Testing Evaluation Performance ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    board.set_starting_position();
    
    const int num_evaluations = 100000;
    auto start = std::chrono::high_resolution_clock::now();
    
    int total_score = 0;
    for (int i = 0; i < num_evaluations; ++i) {
        total_score += eval.evaluate(board);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double evaluations_per_second = (num_evaluations * 1000000.0) / duration.count();
    
    std::cout << "Performed " << num_evaluations << " evaluations" << std::endl;
    std::cout << "Total time: " << duration.count() << " microseconds" << std::endl;
    std::cout << "Evaluations per second: " << std::fixed << std::setprecision(0) << evaluations_per_second << std::endl;
    std::cout << "Average time per evaluation: " << std::fixed << std::setprecision(2) 
              << (double)duration.count() / num_evaluations << " microseconds" << std::endl;
    
    // Prevent compiler optimization
    if (total_score == 0) {
        std::cout << "Unexpected result" << std::endl;
    }
    
    std::cout << std::endl;
}

void test_material_values() {
    std::cout << "=== Testing Material Values ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    // Test positions with different material
    struct TestPosition {
        std::string fen;
        std::string description;
    };
    
    TestPosition positions[] = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Starting position"},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBN1 w Qkq - 0 1", "White missing rook"},
        {"rnbqkbn1/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQq - 0 1", "Black missing rook"},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQK2R w KQkq - 0 1", "White missing queen"},
        {"rnbqk2r/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Black missing queen"}
    };
    
    for (const auto& pos : positions) {
        board.set_from_fen(pos.fen);
        int eval_score = eval.evaluate(board);
        std::cout << std::setw(25) << pos.description << ": " << std::setw(6) << eval_score << std::endl;
    }
    
    std::cout << std::endl;
}

// Extended test functions
void test_position_evaluations() {
    std::cout << "=== Testing Position Evaluations ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    // Test positions for comprehensive evaluation testing
    struct TestPosition {
        std::string fen;
        std::string description;
        int expected_sign; // 1 for white advantage, -1 for black advantage, 0 for roughly equal
    };
    
    std::vector<TestPosition> test_positions = {
        // Material advantage tests
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Starting position", 0},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQK2R w KQkq - 0 1", "White missing queen", -1},
        {"rnbqk2r/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Black missing queen", 1},
        {"rnbqkbn1/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQq - 0 1", "Black missing rook", 1},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBN1 w Qkq - 0 1", "White missing rook", -1},
        
        // Pawn structure tests
        {"8/8/8/3P4/8/8/8/8 w - - 0 1", "Isolated white pawn", 0},
        {"8/8/8/8/3p4/8/8/8 w - - 0 1", "Isolated black pawn", 0},
        {"8/8/8/2PPP3/8/8/8/8 w - - 0 1", "White pawn chain", 1},
        {"8/8/8/8/2ppp3/8/8/8 w - - 0 1", "Black pawn chain", -1},
        {"8/8/8/3P4/8/8/3p4/8 w - - 0 1", "Passed pawns both sides", 0},
        
        // King safety tests
        {"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", "Kings on back rank", 0},
        {"8/8/8/8/8/8/4K3/4k3 w - - 0 1", "Kings in center", 0},
        {"rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1", "King's pawn opening", 0},
        
        // Endgame positions
        {"8/8/8/8/8/8/K7/k7 w - - 0 1", "King vs King", 0},
        {"8/8/8/8/8/8/KP6/k7 w - - 0 1", "King and pawn vs King", 1},
        {"8/8/8/8/8/8/K7/kp6 w - - 0 1", "King vs King and pawn", -1},
        {"8/8/8/8/8/8/KQ6/kr6 w - - 0 1", "Queen vs Rook endgame", 1},
        
        // Complex middlegame positions
        {"r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 1", "Italian Game", 0},
        {"rnbqkb1r/pp1ppppp/5n2/2p5/2P5/8/PP1PPPPP/RNBQKBNR w KQkq - 0 1", "Sicilian Defense", 0},
        {"rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1", "French Defense", 0},
        
        // Tactical positions
        {"r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQ1RK1 w kq - 0 1", "Castled position", 0},
        {"8/8/8/8/8/8/8/R3K2r w Q - 0 1", "Rook endgame", 0}
    };
    
    int passed_tests = 0;
    int total_tests = 0;
    
    for (const auto& pos : test_positions) {
        board.set_from_fen(pos.fen);
        int score = eval.evaluate(board);
        
        std::cout << std::setw(30) << pos.description << ": " 
                  << std::setw(6) << score;
        
        // Check if evaluation sign matches expectation
        bool test_passed = true;
        if (pos.expected_sign > 0 && score <= 0) {
            test_passed = false;
        } else if (pos.expected_sign < 0 && score >= 0) {
            test_passed = false;
        }
        
        std::cout << " [" << (test_passed ? "PASS" : "FAIL") << "]" << std::endl;
        
        if (test_passed) passed_tests++;
        total_tests++;
    }
    
    std::cout << "\nPosition evaluation tests: " << passed_tests << "/" << total_tests << " passed" << std::endl;
    std::cout << std::endl;
}

void test_evaluation_consistency() {
    std::cout << "=== Testing Evaluation Consistency ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    // Test that evaluation is consistent across multiple calls
    board.set_starting_position();
    
    std::vector<int> scores;
    for (int i = 0; i < 10; ++i) {
        scores.push_back(eval.evaluate(board));
    }
    
    bool consistent = true;
    for (size_t i = 1; i < scores.size(); ++i) {
        if (scores[i] != scores[0]) {
            consistent = false;
            break;
        }
    }
    
    std::cout << "Evaluation consistency test: " << (consistent ? "PASSED" : "FAILED") << std::endl;
    std::cout << "Sample scores: ";
    for (size_t i = 0; i < std::min(size_t(5), scores.size()); ++i) {
        std::cout << scores[i] << " ";
    }
    std::cout << std::endl << std::endl;
}

void test_symmetry() {
    std::cout << "=== Testing Position Symmetry ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    // Test that flipping colors gives opposite evaluation
    std::vector<std::pair<std::string, std::string>> symmetric_positions = {
        {
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
               "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1"},
         // "RNBQKBNR/PPPPPPPP/8/8/8/8/pppppppp/rnbqkbnr b kqKQ - 0 1"},
        // {"8/8/8/8/8/8/KP6/k7 w - - 0 1",
        //  "8/8/8/8/8/8/kp6/K7 b - - 0 1"}
        {"8/8/8/KP6/k7/8/8/8 w - - 0 1",
            "8/8/8/K7/kp6/8/8/8 b - - 0 1"}
    };
    
    for (const auto& pair : symmetric_positions) {
        std::cout << "\nPosition 1: " << pair.first << std::endl;
        board.set_from_fen(pair.first);
        board.print();
        int score1 = eval.evaluate(board);
        eval.print_evaluation_breakdown(board);
        
        std::cout << "\nPosition 2: " << pair.second << std::endl;
        board.set_from_fen(pair.second);
        int score2 = eval.evaluate(board);
        eval.print_evaluation_breakdown(board);
        
        bool symmetric = (score1 == -score2);
        std::cout << "\nSymmetry test: " << score1 << " vs " << score2 
                  << " [" << (symmetric ? "PASS" : "FAIL") << "]" << std::endl;
        std::cout << "Expected: " << score1 << " vs " << -score1 << std::endl;
    }
    
    std::cout << std::endl;
}

void test_zobrist_collision_resistance() {
    std::cout << "=== Testing Zobrist Hash Collision Resistance ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    std::unordered_set<uint64_t> hashes;
    int collisions = 0;
    int total_positions = 0;
    
    // Test various positions for hash collisions
    std::vector<std::string> test_fens = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
        "rnbqkb1r/pppppppp/5n2/8/8/5N2/PPPPPPPP/RNBQKB1R w KQkq - 0 1",
        "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 1",
        "8/8/8/8/8/8/4K3/4k3 w - - 0 1",
        "8/8/8/8/8/8/KP6/k7 w - - 0 1",
        "8/8/8/8/8/8/KQ6/kr6 w - - 0 1"
    };
    
    for (const auto& fen : test_fens) {
        board.set_from_fen(fen);
        uint64_t hash = eval.compute_zobrist_hash(board);
        
        if (hashes.find(hash) != hashes.end()) {
            collisions++;
        } else {
            hashes.insert(hash);
        }
        total_positions++;
    }
    
    std::cout << "Hash collision test: " << collisions << "/" << total_positions 
              << " collisions detected" << std::endl;
    std::cout << "Unique hashes generated: " << hashes.size() << std::endl;
    std::cout << std::endl;
}

void test_evaluation_bounds() {
    std::cout << "=== Testing Evaluation Bounds ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    int min_eval = INT_MAX;
    int max_eval = INT_MIN;
    
    // Test evaluation bounds across various positions
    std::vector<std::string> test_fens = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQK2R w KQkq - 0 1",
        "rnbqk2r/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "8/8/8/8/8/8/4K3/4k3 w - - 0 1",
        "8/8/8/8/8/8/KQ6/kr6 w - - 0 1",
        "8/8/8/8/8/8/KP6/k7 w - - 0 1"
    };
    
    for (const auto& fen : test_fens) {
        board.set_from_fen(fen);
        int score = eval.evaluate(board);
        
        min_eval = std::min(min_eval, score);
        max_eval = std::max(max_eval, score);
    }
    
    std::cout << "Evaluation range: [" << min_eval << ", " << max_eval << "]" << std::endl;
    
    // Check for reasonable bounds (not too extreme)
    bool reasonable_bounds = (abs(min_eval) < 10000 && abs(max_eval) < 10000);
    std::cout << "Reasonable bounds test: " << (reasonable_bounds ? "PASSED" : "FAILED") << std::endl;
    std::cout << std::endl;
}

void test_game_phase_transitions() {
    std::cout << "=== Testing Game Phase Transitions ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    struct PhaseTest {
        std::string fen;
        GamePhase expected_phase;
        std::string description;
    };
    
    std::vector<PhaseTest> phase_tests = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", OPENING, "Starting position"},
        {"r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 1", MIDDLEGAME, "Middlegame"},
        {"8/8/8/8/8/8/4K3/4k3 w - - 0 1", ENDGAME, "King endgame"},
        {"8/8/8/8/8/8/KQ6/kr6 w - - 0 1", ENDGAME, "Queen vs Rook endgame"}
    };
    
    int passed = 0;
    for (const auto& test : phase_tests) {
        board.set_from_fen(test.fen);
        GamePhase detected_phase = eval.get_game_phase(board);
        
        bool correct = (detected_phase == test.expected_phase);
        std::cout << std::setw(25) << test.description << ": "
                  << (detected_phase == OPENING ? "Opening" : 
                      detected_phase == MIDDLEGAME ? "Middlegame" : "Endgame")
                  << " [" << (correct ? "PASS" : "FAIL") << "]" << std::endl;
        
        if (correct) passed++;
    }
    
    std::cout << "Phase detection tests: " << passed << "/" << phase_tests.size() << " passed" << std::endl;
    std::cout << std::endl;
}

void test_pawn_hash_table() {
    std::cout << "=== Testing Pawn Hash Table ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    // Test pawn hash table functionality
    board.set_from_fen("8/pppppppp/8/8/8/8/PPPPPPPP/8 w - - 0 1");
    
    // First evaluation should populate hash table
    auto start1 = std::chrono::high_resolution_clock::now();
    int score1 = eval.evaluate_pawn_structure(board);
    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - start1);
    
    // Second evaluation should use hash table (faster)
    auto start2 = std::chrono::high_resolution_clock::now();
    int score2 = eval.evaluate_pawn_structure(board);
    auto end2 = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::nanoseconds>(end2 - start2);
    
    bool scores_match = (score1 == score2);
    bool second_faster = (duration2.count() <= duration1.count());
    
    std::cout << "Pawn hash consistency: " << (scores_match ? "PASSED" : "FAILED") << std::endl;
    std::cout << "Hash table speedup: " << (second_faster ? "DETECTED" : "NOT DETECTED") << std::endl;
    std::cout << "First eval: " << duration1.count() << "ns, Second eval: " << duration2.count() << "ns" << std::endl;
    
    // Clear hash table and test
    eval.clear_pawn_hash_table();
    std::cout << "Hash table cleared successfully" << std::endl;
    std::cout << std::endl;
}

void stress_test_performance() {
    std::cout << "=== Stress Testing Performance ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    const int stress_iterations = 1000000;
    
    // Test with random positions
    std::mt19937 rng(42);
    std::vector<std::string> test_fens = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 1",
        "8/8/8/8/8/8/4K3/4k3 w - - 0 1",
        "8/8/8/8/8/8/KQ6/kr6 w - - 0 1",
        "rnbqkb1r/pp1ppppp/5n2/2p5/2P5/8/PP1PPPPP/RNBQKBNR w KQkq - 0 1"
    };
    
    std::uniform_int_distribution<int> pos_dist(0, test_fens.size() - 1);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    long long total_score = 0;
    for (int i = 0; i < stress_iterations; ++i) {
        int pos_idx = pos_dist(rng);
        board.set_from_fen(test_fens[pos_idx]);
        total_score += eval.evaluate(board);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double evals_per_second = (stress_iterations * 1000000.0) / duration.count();
    double avg_time_ns = (duration.count() * 1000.0) / stress_iterations;
    
    std::cout << "Stress test completed: " << stress_iterations << " evaluations" << std::endl;
    std::cout << "Total time: " << duration.count() << " microseconds" << std::endl;
    std::cout << "Evaluations per second: " << std::fixed << std::setprecision(0) << evals_per_second << std::endl;
    std::cout << "Average time per evaluation: " << std::fixed << std::setprecision(2) << avg_time_ns << " nanoseconds" << std::endl;
    
    // Prevent compiler optimization
    if (total_score == 0) {
        std::cout << "Unexpected total score" << std::endl;
    }
    
    std::cout << std::endl;
}

void test_evaluation_components() {
    std::cout << "=== Testing Individual Evaluation Components ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    board.set_starting_position();
    
    // Test individual components
    int material = eval.evaluate_material(board);
    int positional = eval.evaluate_piece_square_tables(board);
    int pawn_structure = eval.evaluate_pawn_structure(board);
    int king_safety = eval.evaluate_king_safety(board);
    int mobility = eval.evaluate_mobility(board);
    
    std::cout << "Component breakdown for starting position:" << std::endl;
    std::cout << "  Material:      " << std::setw(6) << material << std::endl;
    std::cout << "  Positional:    " << std::setw(6) << positional << std::endl;
    std::cout << "  Pawn structure:" << std::setw(6) << pawn_structure << std::endl;
    std::cout << "  King safety:   " << std::setw(6) << king_safety << std::endl;
    std::cout << "  Mobility:      " << std::setw(6) << mobility << std::endl;
    
    int total_components = material + positional + pawn_structure + king_safety + mobility;
    int full_eval = eval.evaluate(board);
    
    std::cout << "  Sum of parts:  " << std::setw(6) << total_components << std::endl;
    std::cout << "  Full eval:     " << std::setw(6) << full_eval << std::endl;
    
    // Note: These might not match exactly due to phase-based adjustments
    bool components_reasonable = (abs(total_components - full_eval) < 100);
    std::cout << "Components reasonably close: " << (components_reasonable ? "YES" : "NO") << std::endl;
    
    std::cout << std::endl;
}

void test_piece_coordination() {
    std::cout << "=== Testing Piece Coordination Evaluation ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    // Test bishop pair
    board.set_from_fen("rnbqk1nr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    int bishop_pair_score = eval.evaluate_piece_coordination(board);
    std::cout << "Bishop pair position: " << bishop_pair_score << std::endl;
    
    // Test rook on open file
    board.set_from_fen("rnbqkbnr/ppp1pppp/8/3p4/3P4/8/PPP1PPPP/RNBQKBNR w KQkq - 0 1");
    int open_file_score = eval.evaluate_piece_coordination(board);
    std::cout << "Open file position: " << open_file_score << std::endl;
    
    // Test knight outpost
    board.set_from_fen("rnbqkb1r/pppppppp/5n2/8/8/5N2/PPPPPPPP/RNBQKB1R w KQkq - 0 1");
    int outpost_score = eval.evaluate_piece_coordination(board);
    std::cout << "Knight outpost position: " << outpost_score << std::endl;
    
    std::cout << std::endl;
}

void test_endgame_factors() {
    std::cout << "=== Testing Endgame Factors ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    // Test king activity in endgame
    board.set_from_fen("8/8/8/4K3/8/8/4k3/8 w - - 0 1");
    int king_activity = eval.evaluate_endgame_factors(board);
    std::cout << "King activity endgame: " << king_activity << std::endl;
    
    // Test opposition
    board.set_from_fen("8/8/8/4k3/8/4K3/8/8 w - - 0 1");
    int opposition = eval.evaluate_endgame_factors(board);
    std::cout << "Opposition position: " << opposition << std::endl;
    
    // Test connected passed pawns
    board.set_from_fen("8/8/8/2PP4/8/8/8/8 w - - 0 1");
    int connected_pawns = eval.evaluate_endgame_factors(board);
    std::cout << "Connected passed pawns: " << connected_pawns << std::endl;
    
    std::cout << std::endl;
}

void test_development_evaluation() {
    std::cout << "=== Testing Development Evaluation ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    // Test starting position (no development)
    board.set_starting_position();
    int start_dev = eval.evaluate_development(board);
    std::cout << "Starting position development: " << start_dev << std::endl;
    
    // Test developed position
    board.set_from_fen("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 1");
    // board.set_from_fen("rnbqkbnr/pppp1ppp/8/4p3/2B1P3/3P4/PPP2PPP/RNBQK1NR w KQkq - 0 1");
    int developed = eval.evaluate_development(board);
    std::cout << "Developed position: " << developed << std::endl;
    
    // Test early queen development (penalty)
    board.set_from_fen("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPPQPPP/RNB1KBNR b KQkq - 0 1");
    int early_queen = eval.evaluate_development(board);
    std::cout << "Early queen development: " << early_queen << std::endl;
    
    // Test castled position
    board.set_from_fen("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQ1RK1 w kq - 0 1");
    int castled = eval.evaluate_development(board);
    std::cout << "Castled position: " << castled << std::endl;
    
    std::cout << std::endl;
}

void test_tapered_evaluation() {
    std::cout << "=== Testing Tapered Evaluation ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    // Test different game phases
    struct PhaseTest {
        std::string fen;
        std::string description;
    };
    
    std::vector<PhaseTest> phase_tests = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Opening"},
        {"r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 1", "Middlegame"},
        {"8/8/8/8/8/8/4K3/4k3 w - - 0 1", "Endgame"}
    };
    
    for (const auto& test : phase_tests) {
        board.set_from_fen(test.fen);
        GamePhase phase = eval.get_game_phase(board);
        int phase_value = eval.get_phase_value(board);
        int evaluation = eval.evaluate(board);
        
        std::cout << std::setw(15) << test.description 
                  << " - Phase: " << phase 
                  << ", Value: " << phase_value 
                  << ", Eval: " << evaluation << std::endl;
    }
    
    std::cout << std::endl;
}

void test_pawn_structure_detailed() {
    std::cout << "=== Testing Detailed Pawn Structure ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    struct PawnTest {
        std::string fen;
        std::string description;
    };
    
    std::vector<PawnTest> pawn_tests = {
        // Isolated pawn tests (at least 3)
        {"8/8/8/8/3P4/8/8/8 w - - 0 1", "Isolated pawn - center"},
        {"8/8/8/8/P7/8/8/8 w - - 0 1", "Isolated pawn - a-file"},
        {"8/8/8/8/7P/8/8/8 w - - 0 1", "Isolated pawn - h-file"},
        {"8/8/8/8/2P1P3/8/8/8 w - - 0 1", "Two isolated pawns"},
        
        // Doubled pawn tests (at least 3)
        {"8/8/8/8/3P4/3P4/8/8 w - - 0 1", "Doubled pawns - same file"},
        {"8/8/3P4/8/3P4/8/8/8 w - - 0 1", "Doubled pawns - gap between"},
        {"8/3P4/8/8/3P4/3P4/8/8 w - - 0 1", "Tripled pawns"},
        {"8/8/8/8/P3P3/P3P3/8/8 w - - 0 1", "Multiple doubled pawns"},
        
        // Backward pawn tests (at least 3)
        {"8/8/8/8/8/2p5/3P4/8 w - - 0 1", "Backward pawn - basic"},
        {"8/8/8/8/2p1p3/8/3P4/8 w - - 0 1", "Backward pawn - blocked"},
        {"8/8/8/8/8/1p1p4/2P5/8 w - - 0 1", "Backward pawn - no support"},
        {"8/8/8/8/8/p5p1/1P3P2/8 w - - 0 1", "Multiple backward pawns"},
        
        // Passed pawn tests (at least 3)
        {"8/8/8/3P4/8/8/8/8 w - - 0 1", "Passed pawn - clear path"},
        {"8/8/8/3P4/8/8/2p1p3/8 w - - 0 1", "Passed pawn - enemy pawns behind"},
        {"8/8/6P1/8/8/8/8/8 w - - 0 1", "Passed pawn - advanced"},
        {"8/8/8/2PP4/8/8/8/8 w - - 0 1", "Connected passed pawns"},
        {"8/8/8/P6P/8/8/8/8 w - - 0 1", "Multiple passed pawns"},
        
        // Pawn chain tests (at least 3)
        {"8/8/8/2PPP3/8/8/8/8 w - - 0 1", "Pawn chain - basic"},
        {"8/8/2P5/3P4/4P3/5P2/8/8 w - - 0 1", "Pawn chain - diagonal"},
        {"8/8/8/1P6/2P5/3P4/8/8 w - - 0 1", "Pawn chain - long diagonal"},
        {"8/8/8/2P1P3/3P4/8/8/8 w - - 0 1", "Pawn chain - supported center"},
        
        // Connected pawns tests (at least 3)
        {"8/8/8/2PP4/8/8/8/8 w - - 0 1", "Connected pawns - adjacent"},
        {"8/8/8/1PPP4/8/8/8/8 w - - 0 1", "Connected pawns - three in row"},
        {"8/8/8/PP2PP2/8/8/8/8 w - - 0 1", "Multiple connected groups"},
        
        // Complex pawn structure tests
        {"8/8/8/2PP4/8/8/2pp4/8 w - - 0 1", "Opposing pawn chains"},
        {"8/8/8/2P1p3/3P4/8/8/8 w - - 0 1", "Mixed structure - chain vs isolated"},
        {"8/8/3P4/2P1P3/3p4/2p1p3/8/8 w - - 0 1", "Complex pawn tension"},
        {"8/8/8/8/P1P1P1P1/8/8/8 w - - 0 1", "All isolated pawns"},
        {"8/P7/P7/P7/8/8/8/8 w - - 0 1", "Extreme doubled pawns"},
        
        // Positions with other pieces - Isolated pawns
        {"rnbqkbnr/ppp1pppp/8/3p4/3P4/8/PPP1PPPP/RNBQKBNR w KQkq - 0 1", "Isolated d-pawn opening"},
        {"rnbqkbnr/ppp1pppp/8/3p4/3P4/8/PP3PPP/RNBQKBNR w KQkq - 0 1", "Isolated d-pawn opening"},
        {"r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 1", "Isolated e-pawn with pieces"},
        {"rnbqkb1r/pppp1ppp/5n2/4p3/2B1P3/8/PPPP1PPP/RNBQK1NR w KQkq - 0 1", "Isolated e-pawn Italian game"},
        
        // Positions with other pieces - Doubled pawns
        {"rnbqkbnr/ppp2ppp/8/3pp3/3PP3/8/PPP2PPP/RNBQKBNR w KQkq - 0 1", "Doubled e-pawns center"},
        {"r1bqkbnr/pppp1ppp/2n5/8/3pP3/5N2/PPP2PPP/RNBQKB1R w KQkq - 0 1", "Doubled f-pawns after capture"},
        {"rnbqkb1r/ppp2ppp/5n2/3p4/3P4/2N2N2/PPP2PPP/R1BQKB1R w KQkq - 0 1", "Doubled c-pawns Queen's Gambit"},
        
        // Positions with other pieces - Backward pawns
        {"rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1", "Backward d-pawn Sicilian"},
        {"r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1", "Backward f-pawn Italian"},
        {"rnbqkb1r/ppp2ppp/5n2/3pp3/3P4/2N2N2/PPP2PPP/R1BQKB1R w KQkq - 0 1", "Backward e-pawn French"},
        
        // Positions with other pieces - Passed pawns
        {"8/8/8/3P4/8/8/5k2/5K2 w - - 0 1", "Passed pawn endgame"},
        {"r3k2r/ppp2ppp/2n1bn2/3p4/3P4/2N1BN2/PPP2PPP/R3K2R w KQkq - 0 1", "Passed d-pawn middlegame"},
        {"rnbqk2r/ppp2ppp/5n2/3p4/1b1P4/2N2N2/PPP2PPP/R1BQKB1R w KQkq - 0 1", "Passed d-pawn with pressure"},
        
        // Positions with other pieces - Pawn chains
        {"rnbqkbnr/pp2pppp/8/2pp4/3PP3/8/PPP2PPP/RNBQKBNR w KQkq - 0 1", "Central pawn chain French"},
        {"r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 1", "Pawn chain Italian setup"},
        {"rnbqkb1r/ppp2ppp/5n2/3pp3/2PP4/2N2N2/PP3PPP/R1BQKB1R w KQkq - 0 1", "Advanced pawn chain"},
        
        // Complex middlegame positions
        {"r2qkb1r/ppp2ppp/2n1bn2/3p4/3P1B2/2N2N2/PPP2PPP/R2QKB1R w KQkq - 0 1", "Complex pawn structure middlegame"},
        {"rnbq1rk1/ppp1bppp/4pn2/3p4/2PP4/2N1PN2/PP3PPP/R1BQKB1R w KQ - 0 1", "Fianchetto with pawn tension"},
        {"r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 1", "Spanish opening pawn structure"},
        
        // Endgame positions with pawns and pieces
        {"8/2k5/8/3P4/8/3K4/8/8 w - - 0 1", "King and pawn vs king"},
        {"8/8/2k5/3p4/3P4/3K4/8/8 w - - 0 1", "Opposition with pawns"},
        {"8/8/8/2kPp3/8/8/3K4/8 w - e6 0 1", "En passant in endgame"}
    };
    
    std::cout << "\n--- Individual Pawn Structure Tests ---" << std::endl;
    for (const auto& test : pawn_tests) {
        board.set_from_fen(test.fen);
        board.print();
        int pawn_score = eval.evaluate_pawn_structure(board);
        std::cout << test.description << ": " << std::setw(4) << pawn_score << std::endl;
        eval.print_evaluation_breakdown(board);
    }
    
    std::cout << std::endl;
}

void print_king_safety_breakdown(const Board& board, const Evaluation& eval) {
    std::cout << "  King Safety Component Breakdown:" << std::endl;
    
    // I. STRUCTURAL SAFETY (Static Factors)
    // White king safety components
    int white_pawn_shield = eval.evaluate_pawn_shield(board, Board::WHITE);
    int white_open_files = eval.evaluate_open_files_near_king(board, Board::WHITE);
    int white_position = eval.evaluate_king_position_safety(board, Board::WHITE);
    int white_pawn_storms = eval.evaluate_pawn_storms(board, Board::WHITE);
    int white_piece_cover = eval.evaluate_piece_cover(board, Board::WHITE);
    
    // II. THREAT EVALUATION (Dynamic Factors)
    int white_attacking_pieces = eval.evaluate_attacking_pieces_nearby(board, Board::WHITE);
    int white_mobility_escape = eval.evaluate_king_mobility_and_escape(board, Board::WHITE);
    int white_tactical_threats = eval.evaluate_tactical_threats_to_king(board, Board::WHITE);
    int white_attack_maps = eval.evaluate_attack_maps_pressure_zones(board, Board::WHITE);
    
    // III. GAME PHASE ADJUSTMENTS
    // (No specific functions for this category - handled within other evaluations)
    
    // IV. DYNAMIC CONSIDERATIONS
    
    // Black king safety components
    int black_pawn_shield = eval.evaluate_pawn_shield(board, Board::BLACK);
    int black_open_files = eval.evaluate_open_files_near_king(board, Board::BLACK);
    int black_position = eval.evaluate_king_position_safety(board, Board::BLACK);
    int black_pawn_storms = eval.evaluate_pawn_storms(board, Board::BLACK);
    int black_piece_cover = eval.evaluate_piece_cover(board, Board::BLACK);
    
    int black_attacking_pieces = eval.evaluate_attacking_pieces_nearby(board, Board::BLACK);
    int black_mobility_escape = eval.evaluate_king_mobility_and_escape(board, Board::BLACK);
    int black_tactical_threats = eval.evaluate_tactical_threats_to_king(board, Board::BLACK);
    int black_attack_maps = eval.evaluate_attack_maps_pressure_zones(board, Board::BLACK);
    
    

    // Side-by-side comparison format
    std::cout << "    " << std::left << std::setw(40) << "WHITE KING COMPONENTS" << "BLACK KING COMPONENTS" << std::endl;
    std::cout << "    " << std::string(80, '=') << std::endl;
    
    std::cout << "    I. STRUCTURAL SAFETY (Static Factors):" << std::endl;
    std::cout << "    " << std::left << std::setw(32) << "  Pawn Shield:" << std::right << std::setw(6) << white_pawn_shield
              << "    " << std::left << std::setw(32) << "  Pawn Shield:" << std::right << std::setw(6) << black_pawn_shield << std::endl;
    std::cout << "    " << std::left << std::setw(32) << "  Open Files Near King:" << std::right << std::setw(6) << white_open_files 
              << "    " << std::left << std::setw(32) << "  Open Files Near King:" << std::right << std::setw(6) << black_open_files << std::endl;
    std::cout << "    " << std::left << std::setw(32) << "  King Position Safety:" << std::right << std::setw(6) << white_position 
              << "    " << std::left << std::setw(32) << "  King Position Safety:" << std::right << std::setw(6) << black_position << std::endl;
    std::cout << "    " << std::left << std::setw(32) << "  Pawn Storms:" << std::right << std::setw(6) << white_pawn_storms 
              << "    " << std::left << std::setw(32) << "  Pawn Storms:" << std::right << std::setw(6) << black_pawn_storms << std::endl;
    std::cout << "    " << std::left << std::setw(32) << "  Piece Cover:" << std::right << std::setw(6) << white_piece_cover 
              << "    " << std::left << std::setw(32) << "  Piece Cover:" << std::right << std::setw(6) << black_piece_cover << std::endl;
    
    std::cout << "    II. THREAT EVALUATION (Dynamic Factors):" << std::endl;
    std::cout << "    " << std::left << std::setw(32) << "  Attacking Pieces Nearby:" << std::right << std::setw(6) << white_attacking_pieces 
              << "    " << std::left << std::setw(32) << "  Attacking Pieces Nearby:" << std::right << std::setw(6) << black_attacking_pieces << std::endl;
    std::cout << "    " << std::left << std::setw(32) << "  King Mobility and Escape:" << std::right << std::setw(6) << white_mobility_escape 
              << "    " << std::left << std::setw(32) << "  King Mobility and Escape:" << std::right << std::setw(6) << black_mobility_escape << std::endl;
    std::cout << "    " << std::left << std::setw(32) << "  Tactical Threats Comp.:" << std::right << std::setw(6) << white_tactical_threats 
              << "    " << std::left << std::setw(32) << "  Tactical Threats Comp.:" << std::right << std::setw(6) << black_tactical_threats << std::endl;
    std::cout << "    " << std::left << std::setw(32) << "  Attack Maps Pressure:" << std::right << std::setw(6) << white_attack_maps
              << "    " << std::left << std::setw(32) << "  Attack Maps Pressure:" << std::right << std::setw(6) << black_attack_maps << std::endl;
    
    std::cout << "    III. DYNAMIC CONSIDERATIONS:" << std::endl;
    std::cout << "    " << std::left << std::setw(32) << "  (None currently)" << std::right << std::setw(6) << "0"
              << "    " << std::left << std::setw(32) << "  (None currently)" << std::right << std::setw(6) << "0" << std::endl;
    
    

    int white_total = white_pawn_shield + white_open_files + white_position + white_pawn_storms + white_piece_cover +
                     white_attacking_pieces + white_mobility_escape + white_tactical_threats +
                     white_attack_maps;
    int black_total = black_pawn_shield + black_open_files + black_position + black_pawn_storms + black_piece_cover +
                     black_attacking_pieces + black_mobility_escape + black_tactical_threats +
                     black_attack_maps;
    
    std::cout << "    " << std::string(80, '-') << std::endl;
    std::cout << "    " << std::left << std::setw(32) << "  TOTAL:" << std::right << std::setw(6) << white_total 
              << "    " << std::left << std::setw(32) << "  TOTAL:" << std::right << std::setw(6) << black_total << std::endl;
    std::cout << "    " << std::string(80, '=') << std::endl;
    std::cout << "    NET KING SAFETY SCORE (White - Black): " << std::setw(6) << (white_total - black_total) << std::endl;
}

void test_king_safety_detailed() {
    std::cout << "=== Testing Detailed King Safety ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    struct KingSafetyTest {
        std::string fen;
        std::string description;
    };
    
    // Test overall king safety evaluation
    std::cout << "--- Overall King Safety Tests ---" << std::endl;
    std::vector<KingSafetyTest> safety_tests = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Starting position"},
        {"rnbqk2r/pppp1ppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "King on open file"},
        {"rnbq1rk1/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQ - 0 1", "Castled king"},
        {"rnbqkbnr/ppp1pppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Weakened king side"},
        {"8/8/8/8/8/8/4K3/4k3 w - - 0 1", "Exposed kings"},
        {"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", "Kings on back rank"}
    };

    for (const auto& test : safety_tests) {
        board.set_from_fen(test.fen);
        board.print();
        int king_score = eval.evaluate_king_safety(board);
        std::cout << test.description << ": " << std::setw(4) << king_score << std::endl;
        print_king_safety_breakdown(board, eval);
    }
    
    // Test king exposure evaluation
    std::cout << "\n--- King Exposure Tests ---" << std::endl;
    std::vector<KingSafetyTest> exposure_tests = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Normal king shelter"},
        {"rnbqkb1r/pppppp1p/6p1/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Fianchetto setup"},
        {"rnbqk2r/pppppppp/8/8/8/8/PPPPPPPP/RNBQKB1R w KQkq - 0 1", "Missing fianchetto bishop"},
        {"4k3/8/8/8/8/8/8/4K3 w - - 0 1", "Exposed kings center"},
        {"7k/8/8/8/8/8/8/K7 w - - 0 1", "Kings in corners"},
        {"rnbqkbnr/ppp1pppp/8/3p4/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Slightly exposed king"}
    };

    for (const auto& test : exposure_tests) {
        board.set_from_fen(test.fen);
        board.print();
        int king_score = eval.evaluate_king_safety(board);
        std::cout << test.description << ": " << std::setw(4) << king_score << std::endl;
        print_king_safety_breakdown(board, eval);
    }
    
    // Test king attackers evaluation
    std::cout << "\n--- King Attackers Tests ---" << std::endl;
    std::vector<KingSafetyTest> attackers_tests = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "No attackers"},
        {"rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1", "Center pawns"},
        {"r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 1", "Developed pieces"},
        {"r2qkb1r/ppp2ppp/2np1n2/4p1B1/2B1P3/3P1N2/PPP2PPP/RN1QK2R w KQkq - 0 1", "Multiple attackers"},
        {"r1bq1rk1/pppp1ppp/2n2n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQ1RK1 w - - 0 1", "Castled positions"},
        {"2rq1rk1/ppp2ppp/2np1n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQ1RK1 w - - 0 1", "Queen near king"}
    };

    for (const auto& test : attackers_tests) {
        board.set_from_fen(test.fen);
        board.print();
        int king_score = eval.evaluate_king_safety(board);
        std::cout << test.description << ": " << std::setw(4) << king_score << std::endl;
        print_king_safety_breakdown(board, eval);
    }
    
    // Test king tropism evaluation
    std::cout << "\n--- King Tropism Tests ---" << std::endl;
    std::vector<KingSafetyTest> tropism_tests = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Starting position"},
        {"r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 1", "Pieces near kings"},
        {"8/8/8/3nk3/8/3K4/8/8 w - - 0 1", "Knight near king"},
        {"8/8/8/2bk4/8/3K4/8/8 w - - 0 1", "Bishop near king"},
        {"8/8/8/3k4/3r4/3K4/8/8 w - - 0 1", "Rook near king"},
        {"8/8/8/2qk4/8/3K4/8/8 w - - 0 1", "Queen near king"}
    };

    for (const auto& test : tropism_tests) {
        board.set_from_fen(test.fen);
        board.print();
        int king_score = eval.evaluate_king_safety(board);
        std::cout << test.description << ": " << std::setw(4) << king_score << std::endl;
        print_king_safety_breakdown(board, eval);
    }
    
    // Test pawn shield evaluation
    std::cout << "\n--- Pawn Shield Tests ---" << std::endl;
    std::vector<KingSafetyTest> shield_tests = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Full pawn shield"},
        {"rnbqkbnr/ppp1pppp/8/3p4/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Broken black shield"},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPP1PPPP/RNBQKBNR w KQkq - 0 1", "Broken white shield"},
        {"rnbq1rk1/ppp1ppbp/3p1np1/8/8/3P1NP1/PPP1PPBP/RNBQ1RK1 w - - 0 1", "Castled with shield"},
        {"rnbq1rk1/pp2ppbp/3p1np1/2p5/8/3P1NP1/PPP1PPBP/RNBQ1RK1 w - - 0 1", "Weakened castled shield"},
        {"8/8/8/8/8/8/4K3/8 w - - 0 1", "No pawn shield"}
    };

    for (const auto& test : shield_tests) {
        board.set_from_fen(test.fen);
        board.print();
        int king_score = eval.evaluate_king_safety(board);
        std::cout << test.description << ": " << std::setw(4) << king_score << std::endl;
        print_king_safety_breakdown(board, eval);
    }
    
    // Test castling safety evaluation
    std::cout << "\n--- Castling Safety Tests ---" << std::endl;
    std::vector<KingSafetyTest> castling_tests = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "All castling rights"},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Kq - 0 1", "Partial castling rights"},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1", "No castling rights"},
        {"rnbq1rk1/pppppppp/8/8/8/8/PPPPPPPP/RNBQKB1R w KQ - 0 1", "Black castled"},
        {"rnbqkb1r/pppppppp/8/8/8/8/PPPPPPPP/RNBQ1RK1 w kq - 0 1", "White castled"},
        {"rnbq1rk1/pppppppp/8/8/8/8/PPPPPPPP/RNBQ1RK1 w - - 0 1", "Both castled"}
    };

    for (const auto& test : castling_tests) {
        board.set_from_fen(test.fen);
        board.print();
        int king_score = eval.evaluate_king_safety(board);
        std::cout << test.description << ": " << std::setw(4) << king_score << std::endl;
        print_king_safety_breakdown(board, eval);
    }
    
    // Test back rank safety evaluation
    std::cout << "\n--- Back Rank Safety Tests ---" << std::endl;
    std::vector<KingSafetyTest> backrank_tests = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Safe back rank"},
        {"r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1", "Rooks on back rank"},
        {"r3k3/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQ - 0 1", "Black back rank weak"},
        {"r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K3 w Qkq - 0 1", "White back rank weak"},
        {"4k3/pppppppp/8/8/8/8/PPPPPPPP/4K3 w - - 0 1", "Both back ranks weak"},
        {"rnbq1rk1/ppp1ppbp/3p1np1/8/8/3P1NP1/PPP1PPBP/RNBQ1RK1 w - - 0 1", "Castled safety"}
    };

    for (const auto& test : backrank_tests) {
        board.set_from_fen(test.fen);
        board.print();
        int king_score = eval.evaluate_king_safety(board);
        std::cout << test.description << ": " << std::setw(4) << king_score << std::endl;
        print_king_safety_breakdown(board, eval);
    }
    
    // Test king escape squares evaluation
    std::cout << "\n--- King Escape Squares Tests ---" << std::endl;
    std::vector<KingSafetyTest> escape_tests = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Limited escape squares"},
        {"8/8/8/8/8/8/4K3/8 w - - 0 1", "Many escape squares"},
        {"rnbq1rk1/ppp1ppbp/3p1np1/8/8/3P1NP1/PPP1PPBP/RNBQ1RK1 w - - 0 1", "Castled king escape"},
        {"8/8/8/8/8/8/PPP5/RK6 w - - 0 1", "Trapped king"},
        {"8/8/8/8/8/8/5PPP/6KR w - - 0 1", "Trapped king other side"},
        {"4k3/4p3/4P3/8/8/8/8/4K3 w - - 0 1", "Blocked escape squares"}
    };

    for (const auto& test : escape_tests) {
        board.set_from_fen(test.fen);
        board.print();
        int king_score = eval.evaluate_king_safety(board);
        std::cout << test.description << ": " << std::setw(4) << king_score << std::endl;
        print_king_safety_breakdown(board, eval);
    }
    
    // Test tactical threats to king evaluation
    std::cout << "\n--- Tactical Threats Tests ---" << std::endl;
    std::vector<KingSafetyTest> threats_tests = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "No immediate threats"},
        {"rnbqkb1r/pppp1ppp/5n2/4p3/2B1P3/8/PPPP1PPP/RNBQK1NR w KQkq - 0 1", "Pin on f7"},
        {"r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 1", "Potential discoveries"},
        {"r1bq1rk1/ppp2ppp/2np1n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQ1RK1 w - - 0 1", "Complex position"},
        {"8/8/8/8/8/2B1k3/8/4K3 w - - 0 1", "Simple pin threat"},
        {"8/8/8/8/8/8/4k3/R3K2R w KQ - 0 1", "Back rank threats"}
    };

    for (const auto& test : threats_tests) {
        board.set_from_fen(test.fen);
        board.print();
        int king_score = eval.evaluate_king_safety(board);
        std::cout << test.description << ": " << std::setw(4) << king_score << std::endl;
        print_king_safety_breakdown(board, eval);
    }
    
    std::cout << std::endl;
}

void test_mobility_detailed() {
    std::cout << "=== Testing Detailed Mobility ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    struct MobilityTest {
        std::string fen;
        std::string description;
    };
    
    std::vector<MobilityTest> mobility_tests = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Starting position"},
        {"8/8/8/8/3N4/8/8/8 w - - 0 1", "Central knight"},
        {"8/8/8/8/3B4/8/8/8 w - - 0 1", "Central bishop"},
        {"8/8/8/8/3R4/8/8/8 w - - 0 1", "Central rook"},
        {"8/8/8/8/3Q4/8/8/8 w - - 0 1", "Central queen"},
        {"N7/8/8/8/8/8/8/8 w - - 0 1", "Corner knight"},
        {"r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 1", "Developed pieces"}
    };
    
    for (const auto& test : mobility_tests) {
        board.set_from_fen(test.fen);
        int mobility_score = eval.evaluate_mobility(board);
        std::cout << std::setw(25) << test.description << ": " << mobility_score << std::endl;
    }
    
    std::cout << std::endl;
}

void test_incremental_evaluation_detailed() {
    std::cout << "=== Testing Detailed Incremental Evaluation ===" << std::endl;
    
    Board board;
    Evaluation eval;
    MoveGenerator move_gen;
    
    board.set_starting_position();
    eval.initialize_incremental_eval(board);
    
    // Test that incremental evaluation matches full evaluation
    int full_eval = eval.evaluate(board);
    std::cout << "Initial full evaluation: " << full_eval << std::endl;
    
    // Test incremental updates with common opening moves
    std::vector<std::string> test_moves = {
        "e2e4", "g1f3", "b1c3", "d2d4",
    };
    
    for (const auto& move_str : test_moves) {
        // Generate legal moves to find the move
        std::vector<Move> legal_moves = move_gen.generate_legal_moves(board);
        
        Move test_move;
        bool move_found = false;
        
        // Find the move in legal moves (simplified parsing)
        for (const auto& move : legal_moves) {
            // This is a simplified check - in practice you'd need proper move parsing
            if (move_str.length() >= 4) {
                char from_file = move_str[0];
                char from_rank = move_str[1];
                char to_file = move_str[2];
                char to_rank = move_str[3];
                
                if (move.from_file == from_file - 'a' && 
                    move.from_rank == from_rank - '1' &&
                    move.to_file == to_file - 'a' && 
                    move.to_rank == to_rank - '1') {
                    test_move = move;
                    move_found = true;
                    break;
                }
            }
        }
        
        if (move_found) {
            // Make the move and test incremental evaluation
            BitboardMoveUndoData undo_data = board.make_move(test_move);
            board.print();
            
            int incremental_eval = eval.evaluate_incremental(board, test_move, undo_data);
            int full_eval_after = eval.evaluate(board);
            
            bool evaluations_match = (abs(incremental_eval - full_eval_after) < 10);
            
            std::cout << "Move " << move_str << ": Incremental=" << incremental_eval 
                      << ", Full=" << full_eval_after 
                      << " [" << (evaluations_match ? "MATCH" : "DIFFER") << "]" << std::endl;
            
            // Undo the move
            board.undo_move(undo_data);
            eval.undo_incremental_eval(board, test_move, undo_data);
        }
    }
    
    std::cout << std::endl;
}

void test_zobrist_hashing_detailed() {
    std::cout << "=== Testing Detailed Zobrist Hashing ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    // Test hash updates with moves
    board.set_starting_position();
    uint64_t initial_hash = eval.compute_zobrist_hash(board);
    std::cout << "Initial hash: 0x" << std::hex << initial_hash << std::dec << std::endl;
    
    // Test that different positions have different hashes
    std::vector<std::string> test_positions = {
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
        "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2",
        "rnbqkb1r/pppp1ppp/5n2/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 2 3",
        "r1bqkb1r/pppp1ppp/2n2n2/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 4 4"
    };
    
    std::unordered_set<uint64_t> unique_hashes;
    unique_hashes.insert(initial_hash);
    
    for (const auto& fen : test_positions) {
        board.set_from_fen(fen);
        uint64_t hash = eval.compute_zobrist_hash(board);
        
        bool is_unique = (unique_hashes.find(hash) == unique_hashes.end());
        unique_hashes.insert(hash);
        
        std::cout << "Position hash: 0x" << std::hex << hash << std::dec 
                  << " [" << (is_unique ? "UNIQUE" : "COLLISION") << "]" << std::endl;
    }
    
    std::cout << "Total unique hashes: " << unique_hashes.size() 
              << "/" << (test_positions.size() + 1) << std::endl;
    
    std::cout << std::endl;
}

void test_edge_cases() {
    std::cout << "=== Testing Edge Cases ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    // Test empty board (should not crash)
    board.set_from_fen("8/8/8/8/8/8/8/8 w - - 0 1");
    int empty_eval = eval.evaluate(board);
    std::cout << "Empty board evaluation: " << empty_eval << std::endl;
    
    // Test only kings
    board.set_from_fen("8/8/8/8/8/8/4K3/4k3 w - - 0 1");
    int kings_only = eval.evaluate(board);
    std::cout << "Kings only evaluation: " << kings_only << std::endl;
    
    // Test maximum material
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    int max_material = eval.evaluate(board);
    std::cout << "Maximum material evaluation: " << max_material << std::endl;
    
    // Test promotion scenarios
    board.set_from_fen("8/P7/8/8/8/8/8/8 w - - 0 1");
    int promotion_eval = eval.evaluate(board);
    std::cout << "Promotion scenario evaluation: " << promotion_eval << std::endl;
    
    // Test stalemate position
    board.set_from_fen("8/8/8/8/8/8/8/k6K w - - 0 1");
    int stalemate_eval = eval.evaluate(board);
    std::cout << "Stalemate position evaluation: " << stalemate_eval << std::endl;
    
    std::cout << std::endl;
}

void test_evaluation_stability() {
    std::cout << "=== Testing Evaluation Stability ===" << std::endl;
    
    Board board;
    Evaluation eval;
    
    // Test that evaluation is stable across multiple calls
    std::vector<std::string> test_positions = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 1",
        "8/8/8/8/8/8/4K3/4k3 w - - 0 1"
    };
    
    for (const auto& fen : test_positions) {
        board.set_from_fen(fen);
        
        std::vector<int> evaluations;
        for (int i = 0; i < 100; ++i) {
            evaluations.push_back(eval.evaluate(board));
        }
        
        bool all_same = std::all_of(evaluations.begin(), evaluations.end(), 
                                   [&](int val) { return val == evaluations[0]; });
        
        std::cout << "Position stability test: " << (all_same ? "STABLE" : "UNSTABLE") 
                  << " (" << evaluations[0] << ")" << std::endl;
    }
    
    std::cout << std::endl;
}

void test_move_evaluations() {
    std::cout << "=== Testing Move Evaluations ==="  << std::endl;
    
    Board board;
    Evaluation eval;
    MoveGenerator move_gen;
    
    // Test positions with their descriptions
    std::vector<std::pair<std::string, std::string>> test_positions = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Starting Position"},
        {"r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 1", "Italian Game Opening"},
        {"rnbqkb1r/ppp2ppp/3p1n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 1", "Italian Game - Black h6"},
        {"r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 1", "Italian Game - Bc5"},
        {"8/8/8/4k3/4P3/4K3/8/8 w - - 0 1", "King and Pawn Endgame"},
        {"8/8/8/8/8/8/4K3/4k3 w - - 0 1", "King vs King"},
        {"r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1", "Italian Game Early"},
        {"rnbqk2r/pppp1ppp/5n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 1", "Italian Game - Bc5 Early"},
        {"r1bq1rk1/pppp1ppp/2n2n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQ1RK1 w - - 0 1", "Italian Game - Both Castled"},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", "Complex Endgame Position"}
    };
    
    for (const auto& pos : test_positions) {
        const std::string& fen = pos.first;
        const std::string& description = pos.second;
        
        std::cout << "\n--- " << description << " ---" << std::endl;
        std::cout << "FEN: " << fen << std::endl;
        
        // Set up the position
        board.set_from_fen(fen);
        
        // Get the base evaluation of the position
        int base_eval = eval.evaluate(board);
        std::cout << "Base position evaluation: " << base_eval << " cp" << std::endl;

        // Generate all legal moves
        std::vector<Move> legal_moves = move_gen.generate_legal_moves(board);
        std::cout << "Legal moves found: " << legal_moves.size() << std::endl;
        
        if (legal_moves.empty()) {
            std::cout << "No legal moves available (checkmate or stalemate)" << std::endl;
            continue;
        }
        
        // Evaluate each move
        std::vector<std::pair<Move, int>> move_evaluations;
        
        for (const Move& move : legal_moves) {
            // Make the move
            BitboardMoveUndoData undo_data = board.make_move(move);
            move.print();
            board.print();
            eval.print_evaluation_breakdown(board);

            // Evaluate the position after the move
            int move_eval = eval.evaluate(board);  // Negate because it's opponent's turn
            move_evaluations.emplace_back(move, move_eval);
            
            // Undo the move
            board.undo_move(undo_data);
            
        }
        
        // Sort moves by evaluation (best first)
        std::sort(move_evaluations.begin(), move_evaluations.end(), 
                 [](const std::pair<Move, int>& a, const std::pair<Move, int>& b) {
                     return a.second > b.second;
                 });
        
        // Display top 10 moves (or all if fewer than 10)
        int moves_to_show = std::min(10, static_cast<int>(move_evaluations.size()));
        std::cout << "\nTop " << moves_to_show << " moves by evaluation:" << std::endl;
        
        for (int i = 0; i < moves_to_show; ++i) {
            const Move& move = move_evaluations[i].first;
            int eval_score = move_evaluations[i].second;
            int eval_diff = eval_score - base_eval;
            
            std::cout << std::setw(2) << (i + 1) << ". " 
                      << move.to_algebraic() << " "
                      << std::setw(6) << eval_score << " cp "
                      << "(" << std::showpos << eval_diff << std::noshowpos << ")";
            
            // Add move type information
            if (move.is_capture()) {
                std::cout << " [Capture: " << move.captured_piece << "]";
            }
            if (move.is_promotion()) {
                std::cout << " [Promotion: " << move.promotion_piece << "]";
            }
            if (move.is_castling) {
                std::cout << " [Castling]";
            }
            if (move.is_en_passant) {
                std::cout << " [En Passant]";
            }
            
            std::cout << std::endl;
        }
        
        // Show worst moves too (bottom 3)
        if (move_evaluations.size() > 3) {
            std::cout << "\nWorst 3 moves:" << std::endl;
            int start_idx = std::max(0, static_cast<int>(move_evaluations.size()) - 3);
            
            for (int i = start_idx; i < static_cast<int>(move_evaluations.size()); ++i) {
                const Move& move = move_evaluations[i].first;
                int eval_score = move_evaluations[i].second;
                int eval_diff = eval_score - base_eval;
                
                std::cout << std::setw(2) << (i + 1) << ". " 
                          << move.to_algebraic() << " "
                          << std::setw(6) << eval_score << " cp "
                          << "(" << std::showpos << eval_diff << std::noshowpos << ")" << std::endl;
            }
        }
        
        // Calculate evaluation statistics
        if (!move_evaluations.empty()) {
            int best_eval = move_evaluations.front().second;
            int worst_eval = move_evaluations.back().second;
            int eval_range = best_eval - worst_eval;
            
            // Calculate average evaluation
            int total_eval = 0;
            for (const auto& move_eval : move_evaluations) {
                total_eval += move_eval.second;
            }
            int avg_eval = total_eval / static_cast<int>(move_evaluations.size());
            
            std::cout << "\nEvaluation Statistics:" << std::endl;
            std::cout << "  Best move eval:  " << std::setw(6) << best_eval << " cp" << std::endl;
            std::cout << "  Worst move eval: " << std::setw(6) << worst_eval << " cp" << std::endl;
            std::cout << "  Average eval:    " << std::setw(6) << avg_eval << " cp" << std::endl;
            std::cout << "  Evaluation range: " << std::setw(6) << eval_range << " cp" << std::endl;
        }
    }
    
    std::cout << std::endl;
}

void test_custom_fen_move_evaluations() {
    std::cout << "=== Testing Custom FEN Move Evaluations ===" << std::endl;
    
    Board board;
    Evaluation eval;
    MoveGenerator move_gen;
    
    // Allow user to input custom FEN positions
    std::vector<std::string> custom_fens = {
        // Add your custom FEN positions here
        "r2qkb1r/ppp2ppp/2np1n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 1",  // Ruy Lopez
        "rnbqkb1r/pp1ppppp/5n2/2p5/2P5/8/PP1PPPPP/RNBQKBNR w KQkq - 0 1",           // Sicilian Defense
        "rnbqkb1r/pppppppp/5n2/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1",            // King's Pawn Opening
    };
    
    for (const std::string& fen : custom_fens) {
        std::cout << "\n--- Custom Position ---" << std::endl;
        std::cout << "FEN: " << fen << std::endl;
        
        try {
            board.set_from_fen(fen);
            
            // Get base evaluation
            int base_eval = eval.evaluate(board);
            std::cout << "Position evaluation: " << base_eval << " cp" << std::endl;
            
            // Generate and evaluate moves
            std::vector<Move> legal_moves = move_gen.generate_legal_moves(board);
            std::cout << "Legal moves: " << legal_moves.size() << std::endl;
            
            if (!legal_moves.empty()) {
                // Find best move
                Move best_move;
                int best_eval = INT_MIN;
                
                for (const Move& move : legal_moves) {
                    // Make the move
                    BitboardMoveUndoData undo_data = board.make_move(move);
                    
                    // Evaluate the position after the move
                    int move_eval = -eval.evaluate(board);
                    if (move_eval > best_eval) {
                        best_eval = move_eval;
                        best_move = move;
                    }
                    
                    // Undo the move
                    board.undo_move(undo_data);
                }
                
                std::cout << "Best move: " << best_move.to_algebraic() 
                          << " (" << best_eval << " cp)" << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cout << "Error processing FEN: " << e.what() << std::endl;
        }
    }
    
    std::cout << std::endl;
}

int main() {
    std::cout << "Extended Chess Engine Evaluation Test Suite" << std::endl;
    std::cout << "==========================================" << std::endl << std::endl;
    
    try {
//        // Original tests
//        test_basic_evaluation();
//        test_evaluation_breakdown();
//        test_game_phases();
//        test_zobrist_hashing();
//        test_incremental_evaluation();
//        test_pawn_structure();
//        test_material_values();
//        test_performance();
//
//        // Extended comprehensive tests
//        test_position_evaluations();
//        test_evaluation_consistency();
//        test_symmetry();
//        test_zobrist_collision_resistance();
//        test_evaluation_bounds();
//        test_game_phase_transitions();
//        test_pawn_hash_table();
//        test_evaluation_components();
//        stress_test_performance();
//
//        // Detailed component tests
//        test_piece_coordination();
//        test_endgame_factors();
//        test_development_evaluation();
//        test_tapered_evaluation();
//        test_pawn_structure_detailed();
        test_king_safety_detailed();
//        test_mobility_detailed();
//        test_incremental_evaluation_detailed();
//        test_zobrist_hashing_detailed();
//        test_edge_cases();
//        test_evaluation_stability();
//        test_move_evaluations();
//        test_custom_fen_move_evaluations();
        
        std::cout << "All extended tests completed successfully!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Extended test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}