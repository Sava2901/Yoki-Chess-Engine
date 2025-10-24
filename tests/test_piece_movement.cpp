#include <gtest/gtest.h>
#include "../include/board/Board.h"
#include "../include/board/Move.h"
#include "../include/board/MoveGenerator.h"
#include <iostream>
#include <vector>

class PieceMovementTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset board for each test
    }
    
    void print_board_state(const Board& board, const std::string& description) {
        std::cout << "\n--- " << description << " ---\n";
        board.print();
    }
    
    void test_move_validity(Board& board, const Move& move, const std::string& move_description, 
                           bool should_be_valid) {
        std::cout << "\n=== Testing: " << move_description << " ===\n";
        print_board_state(board, "Board state BEFORE move");
        
        bool is_legal = board.is_move_legal(move);
        std::cout << "\nMove validity: " << (is_legal ? "VALID" : "INVALID") << std::endl;
        
        if (is_legal) {
            auto undo_data = board.make_move(move);
            print_board_state(board, "Board state AFTER move");
            board.undo_move(undo_data);
            std::cout << "\n(Move undone for next test)\n";
        } else {
            std::cout << "\n(No board change - move was invalid)\n";
        }
        
        EXPECT_EQ(is_legal, should_be_valid) << move_description;
    }
    
    Board board;
};

TEST_F(PieceMovementTest, PawnBlocking) {
    std::cout << "\n--- Testing Pawn Movement Blocking ---\n";
    
    // Test pawn blocked by piece in front
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/4P3/4P3/PPPP1PPP/RNBQKBNR w KQkq - 0 1");
    print_board_state(board, "Pawn blocked by own piece");
    
    Move blocked_pawn_move(0, 1, 4, 4, 'P');
    test_move_validity(board, blocked_pawn_move, "Pawn cannot move through own piece", false);
    
    // Test pawn blocked by opponent piece
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/4P3/4p3/PPPP1PPP/RNBQKBNR w KQkq - 0 1");
    print_board_state(board, "Pawn blocked by opponent piece");
    
    Move blocked_by_opponent(3, 4, 4, 4, 'P');
    test_move_validity(board, blocked_by_opponent, "Pawn cannot move through opponent piece", false);
    
    // Test pawn two-square move blocked
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/4P3/PPPP1PPP/RNBQKBNR w KQkq - 0 1");
    print_board_state(board, "Pawn two-square move blocked");
    
    Move blocked_two_square(1, 4, 3, 4, 'P');
    test_move_validity(board, blocked_two_square, "Pawn cannot do two-square move when blocked", false);
}

TEST_F(PieceMovementTest, RookBlocking) {
    std::cout << "\n--- Testing Rook Movement Blocking ---\n";
    
    // Test rook blocked horizontally
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/3RPR2/8/PPPP1PPP/RNBQKBN1 w Qkq - 0 1");
    print_board_state(board, "Rook blocked horizontally");
    
    Move blocked_rook_horizontal(3, 3, 3, 6, 'R');
    test_move_validity(board, blocked_rook_horizontal, "Rook cannot move through piece horizontally", false);
    
    // Test rook blocked vertically
    board.set_from_fen("rnbqkbnr/pppppppp/8/4R3/4P3/4R3/PPPP1PPP/RNBQKBN1 w Qkq - 0 1");
    print_board_state(board, "Rook blocked vertically");
    
    Move blocked_rook_vertical(2, 4, 6, 4, 'R');
    test_move_validity(board, blocked_rook_vertical, "Rook cannot move through pieces vertically", false);
    
    // Test rook can capture but not move beyond
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/3Rp3/8/PPPP1PPP/RNBQKBN1 w Qkq - 0 1");
    print_board_state(board, "Rook can capture but not move beyond");
    
    Move rook_capture(3, 3, 3, 4, 'R', 'p');
    test_move_validity(board, rook_capture, "Rook can capture piece", true);
    
    Move rook_beyond_capture(3, 3, 3, 5, 'R');
    test_move_validity(board, rook_beyond_capture, "Rook cannot move beyond captured piece", false);
}

TEST_F(PieceMovementTest, BishopBlocking) {
    std::cout << "\n--- Testing Bishop Movement Blocking ---\n";
    
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/3B4/2P5/P11P1PPP/1N1QKBNR w KQkq - 0 1");
    print_board_state(board, "Bishop blocked diagonally");
    
    Move blocked_bishop(3, 3, 1, 1, 'B');
    test_move_validity(board, blocked_bishop, "Bishop cannot move through piece diagonally", false);
    
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/3B4/2p5/P11P1PPP/1N1QKBNR w KQkq - 0 1");
    print_board_state(board, "Bishop can capture but not move beyond");
    
    Move bishop_capture(3, 3, 2, 2, 'B', 'p');
    test_move_validity(board, bishop_capture, "Bishop can capture piece", true);
    
    Move bishop_beyond_capture(3, 3, 1, 1, 'B');
    test_move_validity(board, bishop_beyond_capture, "Bishop cannot move beyond captured piece", false);
}

TEST_F(PieceMovementTest, QueenBlocking) {
    std::cout << "\n--- Testing Queen Movement Blocking ---\n";
    
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/3QP3/8/PPPP1PPP/RNB1KBNR w KQkq - 0 1");
    print_board_state(board, "Queen blocked horizontally like rook");
    
    Move blocked_queen_horizontal(3, 3, 3, 5, 'Q');
    test_move_validity(board, blocked_queen_horizontal, "Queen cannot move through piece horizontally", false);
    
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/3Q4/2P5/PP1P1PPP/RNB1KBNR w KQkq - 0 1");
    print_board_state(board, "Queen blocked diagonally like bishop");
    
    Move blocked_queen_diagonal(3, 3, 1, 1, 'Q');
    test_move_validity(board, blocked_queen_diagonal, "Queen cannot move through piece diagonally", false);
}

TEST_F(PieceMovementTest, KnightJumping) {
    std::cout << "\n--- Testing Knight Jumping Over Pieces ---\n";
    
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/3N4/2PPP3/PP3PPP/R1BQKB1R w KQkq - 0 1");
    print_board_state(board, "Knight surrounded by pieces");
    
    Move knight_jump(3, 3, 5, 4, 'N');
    test_move_validity(board, knight_jump, "Knight can jump over pieces", true);
    
    Move knight_jump2(3, 3, 1, 2, 'N');
    test_move_validity(board, knight_jump2, "Knight can jump over pieces in different direction", true);
    
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/3N4/2P1P3/PP3PPP/R1BQKB1R w KQkq - 0 1");
    print_board_state(board, "Knight with own piece on target square");
    board.set_piece(2, 1, 'P');
    
    Move knight_friendly_fire(3, 3, 2, 1, 'N', 'P');
    test_move_validity(board, knight_friendly_fire, "Knight cannot capture own piece", false);
}

TEST_F(PieceMovementTest, KingBlocking) {
    std::cout << "\n--- Testing King Movement Blocking ---\n";
    
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/3PPP2/PPP1K1PP/RNB1QB1R w kq - 0 1");
    print_board_state(board, "King surrounded by own pieces");
    
    Move blocked_king(1, 4, 2, 4, 'K', 'P');
    test_move_validity(board, blocked_king, "King cannot move to square occupied by own piece", false);
    
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/4p3/PPPPKPPP/RNB1QB1R w kq - 0 1");
    print_board_state(board, "King can capture opponent piece");
    
    Move king_capture(1, 4, 2, 4, 'K', 'p');
    test_move_validity(board, king_capture, "King can capture opponent piece", true);
}

TEST_F(PieceMovementTest, ComplexScenarios) {
    std::cout << "\n--- Testing Complex Blocking Scenarios ---\n";
    
    board.set_from_fen("r1bqkb1r/pppppppp/2n2n2/8/3Q4/2N2N2/PPPPPPPP/R1B1KB1R w KQkq - 0 1");
    print_board_state(board, "Queen with multiple blocking pieces");
    
    Move queen_blocked_multiple(3, 3, 7, 7, 'Q');
    test_move_validity(board, queen_blocked_multiple, "Queen cannot move through multiple blocking pieces", false);
    
    Move queen_adjacent(3, 3, 4, 4, 'Q');
    test_move_validity(board, queen_adjacent, "Queen can move to adjacent square even when long path blocked", true);
}

TEST_F(PieceMovementTest, CaptureVsBlocking) {
    std::cout << "\n--- Testing Capture vs Blocking Scenarios ---\n";
    
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/3Rp1p1/8/PPPP1PPP/RNBQKBN1 w Qkq - 0 1");
    print_board_state(board, "Rook with capture opportunity and piece beyond");
    
    Move rook_capture_stop(3, 3, 3, 4, 'R', 'p');
    test_move_validity(board, rook_capture_stop, "Rook can capture first piece", true);
    
    Move rook_through_capture(3, 3, 3, 6, 'R');
    test_move_validity(board, rook_through_capture, "Rook cannot move through pieces to reach distant square", false);
}

TEST_F(PieceMovementTest, EnPassant) {
    std::cout << "\n--- Testing En Passant Special Cases ---\n";
    
    board.set_from_fen("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
    print_board_state(board, "En passant available");
    
    Move valid_en_passant(4, 4, 5, 5, 'P', 'p', '.', false, true);
    test_move_validity(board, valid_en_passant, "Valid en passant move", true);
    
    board.set_from_fen("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3");
    board.set_piece(5, 5, 'n');
    print_board_state(board, "En passant target square blocked");
    
    Move blocked_en_passant(4, 4, 5, 5, 'P', 'n', '.', false, false);
    test_move_validity(board, blocked_en_passant, "En passant blocked by piece on target square", true);
}

TEST_F(PieceMovementTest, CastlingBlocking) {
    std::cout << "\n--- Testing Castling Blocking ---\n";
    
    board.set_from_fen("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3KB1R w KQkq - 0 1");
    print_board_state(board, "Castling blocked by bishop");
    
    Move blocked_castle_kingside(0, 4, 0, 6, 'K', '.', '.', true);
    test_move_validity(board, blocked_castle_kingside, "Castling blocked by piece between king and rook", false);
    
    board.set_from_fen("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1");
    print_board_state(board, "Castling path clear");
    
    Move valid_castle_kingside(0, 4, 0, 6, 'K', '.', '.', true);
    test_move_validity(board, valid_castle_kingside, "Valid castling when path is clear", true);
    
    Move valid_castle_queenside(0, 4, 0, 2, 'K', '.', '.', true);
    test_move_validity(board, valid_castle_queenside, "Valid queenside castling when path is clear", true);
}
