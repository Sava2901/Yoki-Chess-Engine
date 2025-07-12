#include "../board/Board.h"
#include "../board/Move.h"
#include "../board/MoveGenerator.h"
#include <iostream>
#include <vector>
#include <cassert>

struct TestResult {
    std::string test_name;
    std::string category;
    bool passed;
    std::string description;
    std::string expected_outcome;
    std::string actual_outcome;
};

class PieceMovementTester {
private:
    Board board;
    std::vector<TestResult> test_results;
    int good_tests_passed = 0;
    int good_tests_failed = 0;
    int bad_tests_passed = 0;
    int bad_tests_failed = 0;
    
    void record_test(const std::string& test_name, const std::string& category, 
                    bool condition, const std::string& description,
                    const std::string& expected, const std::string& actual) {
        TestResult result;
        result.test_name = test_name;
        result.category = category;
        result.passed = condition;
        result.description = description;
        result.expected_outcome = expected;
        result.actual_outcome = actual;
        test_results.push_back(result);
        
        if (condition) {
            std::cout << "✓ PASS: " << test_name << std::endl;
            if (category == "GOOD_MOVE" || category == "VALID_BEHAVIOR") {
                good_tests_passed++;
            } else {
                bad_tests_passed++;
            }
        } else {
            std::cout << "✗ FAIL: " << test_name << std::endl;
            if (category == "GOOD_MOVE" || category == "VALID_BEHAVIOR") {
                good_tests_failed++;
            } else {
                bad_tests_failed++;
            }
        }
    }
    
    void print_board_state(const std::string& description) {
        std::cout << "\n--- " << description << " ---\n";
        board.print();
    }
    
    void test_move_with_state_display(const Move& move, const std::string& move_description, 
                                      bool should_be_valid, const std::string& test_category = "UNKNOWN") {
        std::cout << "\n=== Testing: " << move_description << " ===\n";
        print_board_state("Board state BEFORE move");
        
        bool is_legal = board.is_move_legal(move);
        std::cout << "\nMove validity: " << (is_legal ? "VALID" : "INVALID") << std::endl;
        
        if (is_legal) {
            // Make the move to show the result
            auto undo_data = board.make_move(move);
            print_board_state("Board state AFTER move");
            
            // Undo the move to restore original state for next test
            board.undo_move(undo_data);
            std::cout << "\n(Move undone for next test)\n";
        } else {
            std::cout << "\n(No board change - move was invalid)\n";
        }
        
        std::string expected = should_be_valid ? "VALID" : "INVALID";
        std::string actual = is_legal ? "VALID" : "INVALID";
        record_test(move_description, test_category, is_legal == should_be_valid,
                   move_description, expected, actual);
    }
    
public:
    void run_all_tests() {
        std::cout << "\n=== Testing Piece Movement and Blocking ===\n";
        
        test_pawn_blocking();
        test_rook_blocking();
        test_bishop_blocking();
        test_queen_blocking();
        test_knight_jumping();
        test_king_blocking();
        test_complex_scenarios();
        test_capture_vs_blocking();
        test_en_passant_blocking();
        test_castling_blocking();
        
        print_summary();
    }
    
    void test_pawn_blocking() {
        std::cout << "\n--- Testing Pawn Movement Blocking ---\n";
        
        // Test pawn blocked by piece in front
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/4P3/4P3/PPPP1PPP/RNBQKBNR w KQkq - 0 1");
        print_board_state("Pawn blocked by own piece");
        
        Move blocked_pawn_move(0, 1, 4, 4, 'P'); // e4 pawn tries to move to e5 (blocked by e3 pawn)
        test_move_with_state_display(blocked_pawn_move, "Pawn cannot move through own piece", false, "BAD_MOVE");
        
        // Test pawn blocked by opponent piece
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/4P3/4p3/PPPP1PPP/RNBQKBNR w KQkq - 0 1");
        print_board_state("Pawn blocked by opponent piece");
        
        Move blocked_by_opponent(3, 4, 4, 4, 'P'); // e4 pawn tries to move to e5 (blocked by e3 black pawn)
        test_move_with_state_display(blocked_by_opponent, "Pawn cannot move through opponent piece", false, "BAD_MOVE");
        
        // Test pawn two-square move blocked
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/4P3/PPPP1PPP/RNBQKBNR w KQkq - 0 1");
        print_board_state("Pawn two-square move blocked");
        
        Move blocked_two_square(1, 4, 3, 4, 'P'); // e2 pawn tries two-square move (blocked by e3 pawn)
        test_move_with_state_display(blocked_two_square, "Pawn cannot do two-square move when blocked", false, "BAD_MOVE");
    }
    
    void test_rook_blocking() {
        std::cout << "\n--- Testing Rook Movement Blocking ---\n";
        
        // Test rook blocked horizontally
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/3RPR2/8/PPPP1PPP/RNBQKBN1 w Qkq - 0 1");
        print_board_state("Rook blocked horizontally");
        
        Move blocked_rook_horizontal(3, 3, 3, 6, 'R'); // d4 rook tries to move to g4 (blocked by f4 rook)
        test_move_with_state_display(blocked_rook_horizontal, "Rook cannot move through piece horizontally", false, "BAD_MOVE");
        
        // Test rook blocked vertically
        board.set_from_fen("rnbqkbnr/pppppppp/8/4R3/4P3/4R3/PPPP1PPP/RNBQKBN1 w Qkq - 0 1");
        print_board_state("Rook blocked vertically");
        
        Move blocked_rook_vertical(2, 4, 6, 4, 'R'); // e3 rook tries to move to e7 (blocked by e4 pawn and e5 rook)
        test_move_with_state_display(blocked_rook_vertical, "Rook cannot move through pieces vertically", false, "BAD_MOVE");
        
        // Test rook can capture but not move beyond
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/3Rp3/8/PPPP1PPP/RNBQKBN1 w Qkq - 0 1");
        print_board_state("Rook can capture but not move beyond");
        
        Move rook_capture(3, 3, 3, 4, 'R', 'p'); // d4 rook captures e4 pawn
        test_move_with_state_display(rook_capture, "Rook can capture piece", true, "GOOD_MOVE");
        
        Move rook_beyond_capture(3, 3, 3, 5, 'R'); // d4 rook tries to move to f4 (beyond captured piece)
        test_move_with_state_display(rook_beyond_capture, "Rook cannot move beyond captured piece", false, "BAD_MOVE");
    }
    
    void test_bishop_blocking() {
        std::cout << "\n--- Testing Bishop Movement Blocking ---\n";
        
        // Test bishop blocked diagonally
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/3B4/2P5/P11P1PPP/1N1QKBNR w KQkq - 0 1");
        print_board_state("Bishop blocked diagonally");
        
        Move blocked_bishop(3, 3, 1, 1, 'B'); // d4 bishop tries to move to b2 (blocked by c3 pawn)
        test_move_with_state_display(blocked_bishop, "Bishop cannot move through piece diagonally", false, "BAD_MOVE");
        
        // Test bishop can capture but not move beyond
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/3B4/2p5/P11P1PPP/1N1QKBNR w KQkq - 0 1");
        print_board_state("Bishop can capture but not move beyond");
        
        Move bishop_capture(3, 3, 2, 2, 'B', 'p'); // d4 bishop captures c3 pawn
        test_move_with_state_display(bishop_capture, "Bishop can capture piece", true, "GOOD_MOVE");
        
        Move bishop_beyond_capture(3, 3, 1, 1, 'B'); // d4 bishop tries to move to b2 (beyond captured piece)
        test_move_with_state_display(bishop_beyond_capture, "Bishop cannot move beyond captured piece", false, "BAD_MOVE");
    }
    
    void test_queen_blocking() {
        std::cout << "\n--- Testing Queen Movement Blocking ---\n";
        
        // Test queen blocked like rook
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/3QP3/8/PPPP1PPP/RNB1KBNR w KQkq - 0 1");
        print_board_state("Queen blocked horizontally like rook");
        
        Move blocked_queen_horizontal(3, 3, 3, 5, 'Q'); // d4 queen tries to move to f4 (blocked by e4 pawn)
        test_move_with_state_display(blocked_queen_horizontal, "Queen cannot move through piece horizontally", false, "BAD_MOVE");
        
        // Test queen blocked like bishop
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/3Q4/2P5/PP1P1PPP/RNB1KBNR w KQkq - 0 1");
        print_board_state("Queen blocked diagonally like bishop");
        
        Move blocked_queen_diagonal(3, 3, 1, 1, 'Q'); // d4 queen tries to move to b2 (blocked by c3 pawn)
        test_move_with_state_display(blocked_queen_diagonal, "Queen cannot move through piece diagonally", false, "BAD_MOVE");
    }
    
    void test_knight_jumping() {
        std::cout << "\n--- Testing Knight Jumping Over Pieces ---\n";
        
        // Test knight can jump over pieces
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/3N4/2PPP3/PP3PPP/R1BQKB1R w KQkq - 0 1");
        print_board_state("Knight surrounded by pieces");
        
        Move knight_jump(3, 3, 5, 4, 'N'); // d4 knight jumps to e6 (over surrounding pieces)
        test_move_with_state_display(knight_jump, "Knight can jump over pieces", true, "GOOD_MOVE");
        
        Move knight_jump2(3, 3, 1, 2, 'N'); // d4 knight jumps to c2 (over surrounding pieces)
        test_move_with_state_display(knight_jump2, "Knight can jump over pieces in different direction", true, "GOOD_MOVE");
        
        // Test knight cannot capture own piece
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/3N4/2P1P3/PP3PPP/R1BQKB1R w KQkq - 0 1");
        print_board_state("Knight with own piece on target square");
        
        Move knight_friendly_fire(3, 3, 2, 1, 'N', 'P'); // d4 knight tries to "capture" b3 pawn
        board.set_piece(2, 1, 'P'); // Place white pawn on b3
        test_move_with_state_display(knight_friendly_fire, "Knight cannot capture own piece", false, "BAD_MOVE");
    }
    
    void test_king_blocking() {
        std::cout << "\n--- Testing King Movement Blocking ---\n";
        
        // Test king blocked by own pieces
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/3PPP2/PPP1K1PP/RNB1QB1R w kq - 0 1");
        print_board_state("King surrounded by own pieces");
        
        Move blocked_king(1, 4, 2, 4, 'K', 'P'); // e2 king tries to move to e3 (blocked by own pawn)
        test_move_with_state_display(blocked_king, "King cannot move to square occupied by own piece", false, "BAD_MOVE");
        
        // Test king can capture opponent piece
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/4p3/PPPPKPPP/RNB1QB1R w kq - 0 1");
        print_board_state("King can capture opponent piece");
        
        Move king_capture(1, 4, 2, 4, 'K', 'p'); // e2 king captures e3 pawn
        test_move_with_state_display(king_capture, "King can capture opponent piece", true, "GOOD_MOVE");
    }
    
    void test_complex_scenarios() {
        std::cout << "\n--- Testing Complex Blocking Scenarios ---\n";
        
        // Test multiple pieces blocking a long-range piece
        board.set_from_fen("r1bqkb1r/pppppppp/2n2n2/8/3Q4/2N2N2/PPPPPPPP/R1B1KB1R w KQkq - 0 1");
        print_board_state("Queen with multiple blocking pieces");
        
        Move queen_blocked_multiple(3, 3, 7, 7, 'Q'); // d4 queen tries to move to h8 (blocked by multiple pieces)
        test_move_with_state_display(queen_blocked_multiple, "Queen cannot move through multiple blocking pieces", false, "BAD_MOVE");
        
        // Test piece can move to adjacent square even when long path is blocked
        Move queen_adjacent(3, 3, 4, 4, 'Q'); // d4 queen moves to e5 (adjacent square)
        test_move_with_state_display(queen_adjacent, "Queen can move to adjacent square even when long path blocked", true, "GOOD_MOVE");
    }
    
    void test_capture_vs_blocking() {
        std::cout << "\n--- Testing Capture vs Blocking Scenarios ---\n";
        
        // Test that capturing stops movement
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/3Rp1p1/8/PPPP1PPP/RNBQKBN1 w Qkq - 0 1");
        print_board_state("Rook with capture opportunity and piece beyond");
        
        Move rook_capture_stop(3, 3, 3, 4, 'R', 'p'); // d4 rook captures e4 pawn
        test_move_with_state_display(rook_capture_stop, "Rook can capture first piece", true, "GOOD_MOVE");
        
        Move rook_through_capture(3, 3, 3, 6, 'R'); // d4 rook tries to move to g4 (through e4 and f4)
        test_move_with_state_display(rook_through_capture, "Rook cannot move through pieces to reach distant square", false, "BAD_MOVE");
    }
    
    void test_en_passant_blocking() {
        std::cout << "\n--- Testing En Passant Special Cases ---\n";
        
        // Test en passant when path is clear
        board.set_from_fen("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
        print_board_state("En passant available");
        
        Move valid_en_passant(4, 4, 5, 5, 'P', 'p', '.', false, true); // e5 pawn captures f6 en passant
        test_move_with_state_display(valid_en_passant, "Valid en passant move", true, "GOOD_MOVE");
        
        // Test en passant when target square is blocked (shouldn't happen in normal play)
        board.set_from_fen("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3");
        board.set_piece(5, 5, 'n'); // Place knight on f6
        print_board_state("En passant target square blocked");
        
        Move blocked_en_passant(4, 4, 5, 5, 'P', 'n', '.', false, false); // e5 pawn tries en passant to blocked f6
        test_move_with_state_display(blocked_en_passant, "En passant blocked by piece on target square", true, "BAD_MOVE");
    }
    
    void test_castling_blocking() {
        std::cout << "\n--- Testing Castling Blocking ---\n";
        
        // Test castling blocked by piece between king and rook
        board.set_from_fen("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3KB1R w KQkq - 0 1");
        print_board_state("Castling blocked by bishop");
        
        Move blocked_castle_kingside(0, 4, 0, 6, 'K', '.', '.', true); // King tries to castle kingside (blocked by bishop)
        test_move_with_state_display(blocked_castle_kingside, "Castling blocked by piece between king and rook", false, "BAD_MOVE");
        
        // Test castling when path is clear
        board.set_from_fen("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1");
        print_board_state("Castling path clear");
        
        Move valid_castle_kingside(0, 4, 0, 6, 'K', '.', '.', true); // King castles kingside
        test_move_with_state_display(valid_castle_kingside, "Valid castling when path is clear", true, "GOOD_MOVE");
        
        Move valid_castle_queenside(0, 4, 0, 2, 'K', '.', '.', true); // King castles queenside
        test_move_with_state_display(valid_castle_queenside, "Valid queenside castling when path is clear", true, "GOOD_MOVE");
    }
    
    void print_summary() {
        std::cout << "\n=== COMPREHENSIVE TEST SUMMARY ===\n";
        
        // Good moves (should be valid)
        std::cout << "\n--- GOOD MOVES (Should be Valid) ---\n";
        std::cout << "Passed: " << good_tests_passed << std::endl;
        std::cout << "Failed: " << good_tests_failed << std::endl;
        std::cout << "Total:  " << (good_tests_passed + good_tests_failed) << std::endl;
        
        // Bad moves (should be invalid)
        std::cout << "\n--- BAD MOVES (Should be Invalid) ---\n";
        std::cout << "Passed: " << bad_tests_passed << std::endl;
        std::cout << "Failed: " << bad_tests_failed << std::endl;
        std::cout << "Total:  " << (bad_tests_passed + bad_tests_failed) << std::endl;
        
        // Overall summary
        int total_tests = good_tests_passed + good_tests_failed + bad_tests_passed + bad_tests_failed;
        int total_passed = good_tests_passed + bad_tests_passed;
        int total_failed = good_tests_failed + bad_tests_failed;
        
        std::cout << "\n--- OVERALL RESULTS ---\n";
        std::cout << "Total Tests: " << total_tests << std::endl;
        std::cout << "Total Passed: " << total_passed << std::endl;
        std::cout << "Total Failed: " << total_failed << std::endl;
        std::cout << "Success Rate: " << (total_tests > 0 ? (total_passed * 100.0 / total_tests) : 0) << "%\n";
        
        // Detailed failure analysis
        if (total_failed > 0) {
            std::cout << "\n--- FAILED TESTS ANALYSIS ---\n";
            for (const auto& result : test_results) {
                if (!result.passed) {
                    std::cout << "❌ [" << result.category << "] " << result.test_name << std::endl;
                    std::cout << "   Expected: " << result.expected_outcome << ", Got: " << result.actual_outcome << std::endl;
                }
            }
        }
        
        // Final verdict
        if (total_failed == 0) {
            std::cout << "\n🎉 ALL TESTS PASSED! \n";
            std::cout << "✅ Valid moves are correctly accepted\n";
            std::cout << "✅ Invalid moves are correctly rejected\n";
            std::cout << "✅ Piece movement validation is working properly\n";
        } else {
            std::cout << "\n⚠️  SOME TESTS FAILED!\n";
            if (good_tests_failed > 0) {
                std::cout << "❌ " << good_tests_failed << " valid moves were incorrectly rejected\n";
            }
            if (bad_tests_failed > 0) {
                std::cout << "❌ " << bad_tests_failed << " invalid moves were incorrectly accepted\n";
            }
            std::cout << "⚠️  There may be issues with piece movement validation\n";
        }
    }
};

int main() {
    PieceMovementTester tester;
    tester.run_all_tests();
    return 0;
}