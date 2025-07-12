#include "../board/Bitboard.h"
#include "../board/Board.h"
#include "../board/MoveGenerator.h"
#include <iostream>
#include <vector>
#include <string>

void test_knight_attacks() {
    std::cout << "=== KNIGHT ATTACK TESTS ===\n";
    
    // Test 1: Knight in center
    std::cout << "Test 1: Knight on e4 (square 28):\n";
    Bitboard knight_attacks = BitboardUtils::knight_attacks(28);
    BitboardUtils::print_bitboard(knight_attacks);
    std::cout << "Expected 8 squares attacked\n\n";
    
    // Test 2: Knight in corner
    std::cout << "Test 2: Knight on a1 (square 0):\n";
    knight_attacks = BitboardUtils::knight_attacks(0);
    BitboardUtils::print_bitboard(knight_attacks);
    std::cout << "Expected 2 squares attacked (b3, c2)\n\n";
    
    // Test 3: Knight on edge
    std::cout << "Test 3: Knight on a4 (square 24):\n";
    knight_attacks = BitboardUtils::knight_attacks(24);
    BitboardUtils::print_bitboard(knight_attacks);
    std::cout << "Expected 4 squares attacked\n\n";
    
    // Test 4: Knight on h8
    std::cout << "Test 4: Knight on h8 (square 63):\n";
    knight_attacks = BitboardUtils::knight_attacks(63);
    BitboardUtils::print_bitboard(knight_attacks);
    std::cout << "Expected 2 squares attacked (f7, g6)\n\n";
}

void test_rook_attacks() {
    std::cout << "=== ROOK ATTACK TESTS ===\n";
    
    // Test 1: Rook with no blockers
    std::cout << "Test 1: Rook on e4 (square 28) - empty board:\n";
    Bitboard rook_attacks = BitboardUtils::rook_attacks(28, 0);
    BitboardUtils::print_bitboard(rook_attacks);
    std::cout << "Expected 14 squares attacked (entire rank and file)\n\n";
    
    // Test 2: Rook with multiple blockers
    std::cout << "Test 2: Rook on e4 with blockers on e2, e6, c4, g4:\n";
    Bitboard occupancy = 0;
    BitboardUtils::set_bit(occupancy, 12); // e2
    BitboardUtils::set_bit(occupancy, 44); // e6
    BitboardUtils::set_bit(occupancy, 26); // c4
    BitboardUtils::set_bit(occupancy, 30); // g4
    rook_attacks = BitboardUtils::rook_attacks(28, occupancy);
    BitboardUtils::print_bitboard(rook_attacks);
    std::cout << "Should attack blockers but not beyond them\n\n";
    
    // Test 3: Rook in corner
    std::cout << "Test 3: Rook on a1 (square 0) - empty board:\n";
    rook_attacks = BitboardUtils::rook_attacks(0, 0);
    BitboardUtils::print_bitboard(rook_attacks);
    std::cout << "Expected 14 squares attacked\n\n";
    
    // Test 4: Rook completely blocked
    std::cout << "Test 4: Rook on e4 completely surrounded:\n";
    occupancy = 0;
    BitboardUtils::set_bit(occupancy, 27); // d4
    BitboardUtils::set_bit(occupancy, 29); // f4
    BitboardUtils::set_bit(occupancy, 20); // e3
    BitboardUtils::set_bit(occupancy, 36); // e5
    rook_attacks = BitboardUtils::rook_attacks(28, occupancy);
    BitboardUtils::print_bitboard(rook_attacks);
    std::cout << "Should only attack the 4 adjacent squares\n\n";
}

void test_bishop_attacks() {
    std::cout << "=== BISHOP ATTACK TESTS ===\n";
    
    // Test 1: Bishop with no blockers
    std::cout << "Test 1: Bishop on e4 (square 28) - empty board:\n";
    Bitboard bishop_attacks = BitboardUtils::bishop_attacks(28, 0);
    BitboardUtils::print_bitboard(bishop_attacks);
    std::cout << "Expected 13 squares attacked (all diagonals)\n\n";
    
    // Test 2: Bishop with blockers
    std::cout << "Test 2: Bishop on e4 with blockers on c2, g6, c6, g2:\n";
    Bitboard occupancy = 0;
    BitboardUtils::set_bit(occupancy, 10); // c2
    BitboardUtils::set_bit(occupancy, 46); // g6
    BitboardUtils::set_bit(occupancy, 42); // c6
    BitboardUtils::set_bit(occupancy, 14); // g2
    bishop_attacks = BitboardUtils::bishop_attacks(28, occupancy);
    BitboardUtils::print_bitboard(bishop_attacks);
    std::cout << "Should attack blockers but not beyond them\n\n";
    
    // Test 3: Bishop in corner
    std::cout << "Test 3: Bishop on a1 (square 0) - empty board:\n";
    bishop_attacks = BitboardUtils::bishop_attacks(0, 0);
    BitboardUtils::print_bitboard(bishop_attacks);
    std::cout << "Expected 7 squares attacked (one diagonal)\n\n";
    
    // Test 4: Bishop on light square vs dark square
    std::cout << "Test 4: Bishop on d1 (square 3) - light square:\n";
    bishop_attacks = BitboardUtils::bishop_attacks(3, 0);
    BitboardUtils::print_bitboard(bishop_attacks);
    std::cout << "Should only attack light squares\n\n";
}

void test_queen_attacks() {
    std::cout << "=== QUEEN ATTACK TESTS ===\n";
    
    // Test 1: Queen with no blockers
    std::cout << "Test 1: Queen on e4 (square 28) - empty board:\n";
    Bitboard queen_attacks = BitboardUtils::queen_attacks(28, 0);
    BitboardUtils::print_bitboard(queen_attacks);
    std::cout << "Expected 27 squares attacked (rook + bishop)\n\n";
    
    // Test 2: Queen with mixed blockers
    std::cout << "Test 2: Queen on e4 with various blockers:\n";
    Bitboard occupancy = 0;
    BitboardUtils::set_bit(occupancy, 20); // e3
    BitboardUtils::set_bit(occupancy, 29); // f4
    BitboardUtils::set_bit(occupancy, 35); // d5
    BitboardUtils::set_bit(occupancy, 46); // g6
    queen_attacks = BitboardUtils::queen_attacks(28, occupancy);
    BitboardUtils::print_bitboard(queen_attacks);
    std::cout << "Should combine rook and bishop attack patterns\n\n";
}

void test_king_attacks() {
    std::cout << "=== KING ATTACK TESTS ===\n";
    
    // Test 1: King in center
    std::cout << "Test 1: King on e4 (square 28):\n";
    Bitboard king_attacks = BitboardUtils::king_attacks(28);
    BitboardUtils::print_bitboard(king_attacks);
    std::cout << "Expected 8 squares attacked\n\n";
    
    // Test 2: King in corner
    std::cout << "Test 2: King on a1 (square 0):\n";
    king_attacks = BitboardUtils::king_attacks(0);
    BitboardUtils::print_bitboard(king_attacks);
    std::cout << "Expected 3 squares attacked\n\n";
    
    // Test 3: King on edge
    std::cout << "Test 3: King on e1 (square 4):\n";
    king_attacks = BitboardUtils::king_attacks(4);
    BitboardUtils::print_bitboard(king_attacks);
    std::cout << "Expected 5 squares attacked\n\n";
    
    // Test 4: King on h8
    std::cout << "Test 4: King on h8 (square 63):\n";
    king_attacks = BitboardUtils::king_attacks(63);
    BitboardUtils::print_bitboard(king_attacks);
    std::cout << "Expected 3 squares attacked\n\n";
}

void test_pawn_attacks() {
    std::cout << "=== PAWN ATTACK TESTS ===\n";
    
    // Test 1: White pawn in center
    std::cout << "Test 1: White pawn on e4 (square 28):\n";
    Bitboard pawn_attacks = BitboardUtils::pawn_attacks(28, true);
    BitboardUtils::print_bitboard(pawn_attacks);
    std::cout << "Expected 2 squares attacked (d5, f5)\n\n";
    
    // Test 2: Black pawn in center
    std::cout << "Test 2: Black pawn on e5 (square 36):\n";
    pawn_attacks = BitboardUtils::pawn_attacks(36, false);
    BitboardUtils::print_bitboard(pawn_attacks);
    std::cout << "Expected 2 squares attacked (d4, f4)\n\n";
    
    // Test 3: White pawn on edge
    std::cout << "Test 3: White pawn on a4 (square 24):\n";
    pawn_attacks = BitboardUtils::pawn_attacks(24, true);
    BitboardUtils::print_bitboard(pawn_attacks);
    std::cout << "Expected 1 square attacked (b5)\n\n";
    
    // Test 4: Black pawn on edge
    std::cout << "Test 4: Black pawn on h5 (square 39):\n";
    pawn_attacks = BitboardUtils::pawn_attacks(39, false);
    BitboardUtils::print_bitboard(pawn_attacks);
    std::cout << "Expected 1 square attacked (g4)\n\n";
    
    // Test 5: White pawn on 8th rank (shouldn't happen but test anyway)
    std::cout << "Test 5: White pawn on e8 (square 60):\n";
    pawn_attacks = BitboardUtils::pawn_attacks(60, true);
    BitboardUtils::print_bitboard(pawn_attacks);
    std::cout << "Expected 0 squares attacked (off board)\n\n";
    
    // Test 6: Black pawn on 1st rank
    std::cout << "Test 6: Black pawn on e1 (square 4):\n";
    pawn_attacks = BitboardUtils::pawn_attacks(4, false);
    BitboardUtils::print_bitboard(pawn_attacks);
    std::cout << "Expected 0 squares attacked (off board)\n\n";
}

void test_edge_cases() {
    std::cout << "=== EDGE CASE TESTS ===\n";
    
    // Test 1: All pieces don't attack their own square
    std::cout << "Test 1: Pieces don't attack their own square:\n";
    int test_square = 28; // e4
    
    bool knight_self = BitboardUtils::get_bit(BitboardUtils::knight_attacks(test_square), test_square);
    bool rook_self = BitboardUtils::get_bit(BitboardUtils::rook_attacks(test_square, 0), test_square);
    bool bishop_self = BitboardUtils::get_bit(BitboardUtils::bishop_attacks(test_square, 0), test_square);
    bool queen_self = BitboardUtils::get_bit(BitboardUtils::queen_attacks(test_square, 0), test_square);
    bool king_self = BitboardUtils::get_bit(BitboardUtils::king_attacks(test_square), test_square);
    bool pawn_self_w = BitboardUtils::get_bit(BitboardUtils::pawn_attacks(test_square, true), test_square);
    bool pawn_self_b = BitboardUtils::get_bit(BitboardUtils::pawn_attacks(test_square, false), test_square);
    
    std::cout << "Knight attacks own square: " << (knight_self ? "YES (ERROR)" : "NO (CORRECT)") << "\n";
    std::cout << "Rook attacks own square: " << (rook_self ? "YES (ERROR)" : "NO (CORRECT)") << "\n";
    std::cout << "Bishop attacks own square: " << (bishop_self ? "YES (ERROR)" : "NO (CORRECT)") << "\n";
    std::cout << "Queen attacks own square: " << (queen_self ? "YES (ERROR)" : "NO (CORRECT)") << "\n";
    std::cout << "King attacks own square: " << (king_self ? "YES (ERROR)" : "NO (CORRECT)") << "\n";
    std::cout << "White pawn attacks own square: " << (pawn_self_w ? "YES (ERROR)" : "NO (CORRECT)") << "\n";
    std::cout << "Black pawn attacks own square: " << (pawn_self_b ? "YES (ERROR)" : "NO (CORRECT)") << "\n\n";
    
    // Test 2: Sliding pieces with self-blocking
    std::cout << "Test 2: Sliding pieces with self on occupancy (shouldn't happen but test):\n";
    Bitboard self_occupancy = 0;
    BitboardUtils::set_bit(self_occupancy, test_square);
    
    Bitboard rook_self_blocked = BitboardUtils::rook_attacks(test_square, self_occupancy);
    Bitboard bishop_self_blocked = BitboardUtils::bishop_attacks(test_square, self_occupancy);
    
    std::cout << "Rook with self in occupancy attacks: " << BitboardUtils::popcount(rook_self_blocked) << " squares\n";
    std::cout << "Bishop with self in occupancy attacks: " << BitboardUtils::popcount(bishop_self_blocked) << " squares\n\n";
}

void test_symmetry() {
    std::cout << "=== SYMMETRY TESTS ===\n";
    
    // Test knight symmetry
    std::cout << "Test 1: Knight attack symmetry:\n";
    Bitboard knight_e4 = BitboardUtils::knight_attacks(28); // e4
    Bitboard knight_e5 = BitboardUtils::knight_attacks(36); // e5
    Bitboard knight_d4 = BitboardUtils::knight_attacks(27); // d4
    Bitboard knight_f4 = BitboardUtils::knight_attacks(29); // f4
    
    std::cout << "Knight on e4 attacks: " << BitboardUtils::popcount(knight_e4) << " squares\n";
    std::cout << "Knight on e5 attacks: " << BitboardUtils::popcount(knight_e5) << " squares\n";
    std::cout << "Knight on d4 attacks: " << BitboardUtils::popcount(knight_d4) << " squares\n";
    std::cout << "Knight on f4 attacks: " << BitboardUtils::popcount(knight_f4) << " squares\n\n";
    
    // Test king symmetry
    std::cout << "Test 2: King attack symmetry:\n";
    Bitboard king_e4 = BitboardUtils::king_attacks(28); // e4
    Bitboard king_e5 = BitboardUtils::king_attacks(36); // e5
    Bitboard king_d4 = BitboardUtils::king_attacks(27); // d4
    Bitboard king_f4 = BitboardUtils::king_attacks(29); // f4
    
    std::cout << "King on e4 attacks: " << BitboardUtils::popcount(king_e4) << " squares\n";
    std::cout << "King on e5 attacks: " << BitboardUtils::popcount(king_e5) << " squares\n";
    std::cout << "King on d4 attacks: " << BitboardUtils::popcount(king_d4) << " squares\n";
    std::cout << "King on f4 attacks: " << BitboardUtils::popcount(king_f4) << " squares\n\n";
}

int main() {
    BitboardUtils::init();
    
    std::cout << "=== COMPREHENSIVE ATTACK PATTERN TESTS ===\n\n";
    
    test_knight_attacks();
    test_rook_attacks();
    test_bishop_attacks();
    test_queen_attacks();
    test_king_attacks();
    test_pawn_attacks();
    test_edge_cases();
    test_symmetry();

    std::cout << "=== ALL TESTS COMPLETED ===\n";
    
    return 0;
}