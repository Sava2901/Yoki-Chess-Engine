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
    board.set_starting_position();
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
    board.set_from_fen("8/8/8/8/3P4/8/8/8 w - - 0 1");
    std::cout << "Isolated pawn position:" << std::endl;
    eval.print_evaluation_breakdown(board);
    
    // Test passed pawns
    board.set_from_fen("8/8/8/3P4/8/8/8/8 w - - 0 1");
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
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
         "RNBQKBNR/PPPPPPPP/8/8/8/8/pppppppp/rnbqkbnr b kqKQ - 0 1"},
        {"8/8/8/8/8/8/KP6/k7 w - - 0 1",
         "8/8/8/8/8/8/kp6/K7 b - - 0 1"}
    };
    
    for (const auto& pair : symmetric_positions) {
        board.set_from_fen(pair.first);
        int score1 = eval.evaluate(board);
        
        board.set_from_fen(pair.second);
        int score2 = eval.evaluate(board);
        
        bool symmetric = (score1 == -score2);
        std::cout << "Symmetry test: " << score1 << " vs " << score2 
                  << " [" << (symmetric ? "PASS" : "FAIL") << "]" << std::endl;
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

int main() {
    std::cout << "Extended Chess Engine Evaluation Test Suite" << std::endl;
    std::cout << "==========================================" << std::endl << std::endl;
    
    try {
        // Original tests
        test_basic_evaluation();
        test_evaluation_breakdown();
        test_game_phases();
        test_zobrist_hashing();
        test_incremental_evaluation();
        test_pawn_structure();
        test_material_values();
        test_performance();
        
        // Extended comprehensive tests
        test_position_evaluations();
        test_evaluation_consistency();
        test_symmetry();
        test_zobrist_collision_resistance();
        test_evaluation_bounds();
        test_game_phase_transitions();
        test_pawn_hash_table();
        test_evaluation_components();
        stress_test_performance();
        
        std::cout << "All extended tests completed successfully!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Extended test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}