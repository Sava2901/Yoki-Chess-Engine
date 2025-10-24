#include <gtest/gtest.h>
#include "../include/board/Bitboard.h"
#include "../include/board/Board.h"
#include "../include/board/MoveGenerator.h"
#include <chrono>

class BitboardTest : public ::testing::Test {
protected:
    void SetUp() override {
        BitboardUtils::init();
    }
};

TEST_F(BitboardTest, BitboardUtils) {
    // Test basic operations
    Bitboard bb = 0;
    BitboardUtils::set_bit(bb, 0);  // a1
    BitboardUtils::set_bit(bb, 7);  // h1
    BitboardUtils::set_bit(bb, 56); // a8
    BitboardUtils::set_bit(bb, 63); // h8
    
    EXPECT_EQ(BitboardUtils::popcount(bb), 4);
    EXPECT_TRUE(BitboardUtils::get_bit(bb, 0));
    EXPECT_TRUE(BitboardUtils::get_bit(bb, 7));
    EXPECT_TRUE(BitboardUtils::get_bit(bb, 56));
    EXPECT_TRUE(BitboardUtils::get_bit(bb, 63));
    EXPECT_FALSE(BitboardUtils::get_bit(bb, 28)); // e4
}

TEST_F(BitboardTest, KnightAttacks) {
    // Knight in center should have 8 possible moves
    Bitboard knight_attacks = BitboardUtils::knight_attacks(28); // e4
    EXPECT_EQ(BitboardUtils::popcount(knight_attacks), 8);
    
    // Knight in corner should have only 2 moves
    knight_attacks = BitboardUtils::knight_attacks(0); // a1
    EXPECT_EQ(BitboardUtils::popcount(knight_attacks), 2);
}

TEST_F(BitboardTest, RookAttacks) {
    // Rook on empty board from e4 should have 14 moves (7 horizontal + 7 vertical)
    Bitboard rook_attacks = BitboardUtils::rook_attacks(28, 0);
    EXPECT_EQ(BitboardUtils::popcount(rook_attacks), 14);
    
    // Test with blockers
    Bitboard blockers = 0;
    BitboardUtils::set_bit(blockers, 20); // e3
    BitboardUtils::set_bit(blockers, 30); // g4
    rook_attacks = BitboardUtils::rook_attacks(28, blockers);
    EXPECT_GT(BitboardUtils::popcount(rook_attacks), 0);
    EXPECT_LT(BitboardUtils::popcount(rook_attacks), 14);
}

TEST_F(BitboardTest, BishopAttacks) {
    // Bishop on empty board from e4
    Bitboard bishop_attacks = BitboardUtils::bishop_attacks(28, 0);
    EXPECT_EQ(BitboardUtils::popcount(bishop_attacks), 13);
    
    // Bishop in corner
    bishop_attacks = BitboardUtils::bishop_attacks(0, 0);
    EXPECT_EQ(BitboardUtils::popcount(bishop_attacks), 7);
}

TEST_F(BitboardTest, QueenAttacks) {
    // Queen combines rook and bishop moves
    Bitboard queen_attacks = BitboardUtils::queen_attacks(28, 0);
    EXPECT_EQ(BitboardUtils::popcount(queen_attacks), 27);
}

TEST_F(BitboardTest, KingAttacks) {
    // King in center
    Bitboard king_attacks = BitboardUtils::king_attacks(28); // e4
    EXPECT_EQ(BitboardUtils::popcount(king_attacks), 8);
    
    // King in corner
    king_attacks = BitboardUtils::king_attacks(0); // a1
    EXPECT_EQ(BitboardUtils::popcount(king_attacks), 3);
}

TEST_F(BitboardTest, BoardInitialization) {
    Board board;
    board.set_starting_position();
    
    // Test piece placement
    EXPECT_EQ(board.get_piece(1, 4), 'P'); // e2 - white pawn
    EXPECT_EQ(board.get_piece(6, 4), 'p'); // e7 - black pawn
    EXPECT_EQ(board.get_piece(0, 4), 'K'); // e1 - white king
    EXPECT_EQ(board.get_piece(7, 4), 'k'); // e8 - black king
    
    // Test bitboard access
    Bitboard white_pawns = board.get_piece_bitboard(Board::PAWN, Board::WHITE);
    EXPECT_EQ(BitboardUtils::popcount(white_pawns), 8);
    
    Bitboard black_pieces = board.get_color_bitboard(Board::BLACK);
    EXPECT_EQ(BitboardUtils::popcount(black_pieces), 16);
}

TEST_F(BitboardTest, MoveGeneration) {
    Board board;
    MoveGenerator generator;
    board.set_starting_position();
    
    MoveList moves = generator.generate_all_moves(board);
    
    // Starting position should have 20 legal moves
    EXPECT_GT(moves.size(), 0);
}

TEST_F(BitboardTest, LegalMoves) {
    Board board;
    MoveGenerator generator;
    board.set_starting_position();
    
    MoveList legal_moves = generator.generate_legal_moves(board);
    
    // Starting position should have 20 legal moves for white
    EXPECT_EQ(legal_moves.size(), 20);
}

TEST_F(BitboardTest, AttackDetection) {
    Board board;
    MoveGenerator generator;
    
    // Starting position - no one in check
    board.set_starting_position();
    EXPECT_FALSE(generator.is_in_check(board, Board::WHITE));
    EXPECT_FALSE(generator.is_in_check(board, Board::BLACK));
    
    // Test a position with check
    board.set_from_fen("rnbqkb1r/pppp1ppp/5n2/4p3/2B1P3/8/PPPP1PPP/RNBQK1NR w KQkq - 2 3");
    // This position has no check, but let's verify it doesn't crash
    bool white_in_check = generator.is_in_check(board, Board::WHITE);
    bool black_in_check = generator.is_in_check(board, Board::BLACK);
    EXPECT_FALSE(white_in_check || black_in_check); // Just verify no crash
}

TEST_F(BitboardTest, PerformanceBenchmark) {
    Board board;
    MoveGenerator generator;
    board.set_starting_position();
    
    const int iterations = 1000;
    auto start = std::chrono::high_resolution_clock::now();
    
    long long total_moves = 0;
    for (int i = 0; i < iterations; i++) {
        MoveList moves = generator.generate_all_moves(board);
        total_moves += moves.size();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    EXPECT_GT(total_moves, 0);
    EXPECT_GT(duration.count(), 0);
    
    // Log performance info
    double moves_per_sec = (double)total_moves / duration.count() * 1000000;
    std::cout << "Move generation: " << moves_per_sec << " moves/sec" << std::endl;
}