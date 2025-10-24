#include "../include/board/Bitboard.h"
#include "../include/board/Board.h"
#include "../include/board/MoveGenerator.h"
#include <gtest/gtest.h>
#include <vector>
#include <string>

class MoveUndoTest : public ::testing::Test {
protected:
    Board board;
    MoveGenerator generator;
    
    void SetUp() override {
        BitboardUtils::init();
    }
    
    bool boards_equal(const Board& board1, const Board& board2) {
        return board1.to_fen() == board2.to_fen();
    }
};

TEST_F(MoveUndoTest, BasicPawnMoves) {
    // Test white pawn single move
    board.set_starting_position();
    Board original = board;
    
    Move pawn_move(1, 4, 2, 4, 'P'); // e2-e3
    BitboardMoveUndoData undo_data = board.make_move(pawn_move);
    
    EXPECT_EQ(board.get_piece(2, 4), 'P') << "White pawn moved to e3";
    EXPECT_EQ(board.get_piece(1, 4), '.') << "White pawn left e2";
    EXPECT_EQ(board.get_active_color(), Board::BLACK) << "Turn switched to black";
    
    board.undo_move(undo_data);
    EXPECT_TRUE(boards_equal(board, original)) << "Pawn move undo restores position";
    
    // Test black pawn single move
    board.set_starting_position();
    board.set_active_color(Board::BLACK);
    original = board;
    
    Move black_pawn_move(6, 3, 5, 3, 'p'); // d7-d6
    undo_data = board.make_move(black_pawn_move);
    
    EXPECT_EQ(board.get_piece(5, 3), 'p') << "Black pawn moved to d6";
    EXPECT_EQ(board.get_piece(6, 3), '.') << "Black pawn left d7";
    
    board.undo_move(undo_data);
    EXPECT_TRUE(boards_equal(board, original)) << "Black pawn move undo restores position";
}

TEST_F(MoveUndoTest, PawnCaptures) {
    board.set_from_fen("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
    Board original = board;

    Move capture_move(3, 4, 4, 3, 'P', 'p'); // exd5
    BitboardMoveUndoData undo_data = board.make_move(capture_move);
    
    EXPECT_EQ(board.get_piece(4, 3), 'P') << "White pawn captured on d5";
    EXPECT_EQ(board.get_piece(3, 4), '.') << "White pawn left e4";
    
    board.undo_move(undo_data);
    EXPECT_TRUE(boards_equal(board, original)) << "Pawn capture undo restores position";
    EXPECT_EQ(board.get_piece(4, 3), 'p') << "Captured pawn restored";
}

TEST_F(MoveUndoTest, PawnDoubleMoves) {
    board.set_starting_position();
    Board original = board;
    
    Move double_move(1, 4, 3, 4, 'P'); // e2-e4
    BitboardMoveUndoData undo_data = board.make_move(double_move);
    
    EXPECT_EQ(board.get_piece(3, 4), 'P') << "White pawn moved to e4";
    EXPECT_EQ(board.get_en_passant_file(), 4) << "En passant file set to e";
    
    board.undo_move(undo_data);
    EXPECT_TRUE(boards_equal(board, original)) << "Pawn double move undo restores position";
    EXPECT_EQ(board.get_en_passant_file(), -1) << "En passant file restored";
}

TEST_F(MoveUndoTest, EnPassant) {
    board.set_from_fen("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
    Board original = board;
    
    Move en_passant_move(4, 4, 5, 5, 'P', 'p', '.', false, true); // exf6 e.p.
    BitboardMoveUndoData undo_data = board.make_move(en_passant_move);
    
    EXPECT_EQ(board.get_piece(5, 5), 'P') << "White pawn moved to f6";
    EXPECT_EQ(board.get_piece(4, 5), '.') << "Captured pawn removed from f5";
    EXPECT_EQ(board.get_piece(4, 4), '.') << "White pawn left e5";
    
    board.undo_move(undo_data);
    EXPECT_TRUE(boards_equal(board, original)) << "En passant undo restores position";
    EXPECT_EQ(board.get_piece(4, 5), 'p') << "Captured pawn restored";
}

TEST_F(MoveUndoTest, PawnPromotions) {
    board.set_from_fen("rnbqkbn1/pppppppP/8/8/8/8/PPPPPPP1/RNBQKBNR w KQq - 0 1");
    Board original = board;
    
    Move promotion_move(6, 7, 7, 7, 'P', '.', 'Q'); // h7-h8=Q
    BitboardMoveUndoData undo_data = board.make_move(promotion_move);
    
    EXPECT_EQ(board.get_piece(7, 7), 'Q') << "Pawn promoted to queen";
    EXPECT_EQ(board.get_piece(6, 7), '.') << "Pawn left h7";
    
    board.undo_move(undo_data);
    EXPECT_TRUE(boards_equal(board, original)) << "Promotion undo restores position";
    EXPECT_EQ(board.get_piece(6, 7), 'P') << "Pawn restored on h7";
    
    // Test promotion with capture
    board.set_from_fen("rnbqkbnr/pppppppP/8/8/8/8/PPPPPPP1/RNBQKBN1 w Qkq - 0 1");
    original = board;
    
    Move promotion_capture(6, 7, 7, 6, 'P', 'n', 'Q'); // hxg8=Q
    undo_data = board.make_move(promotion_capture);
    
    EXPECT_EQ(board.get_piece(7, 6), 'Q') << "Pawn promoted to queen with capture";
    
    board.undo_move(undo_data);
    EXPECT_TRUE(boards_equal(board, original)) << "Promotion capture undo restores position";
    EXPECT_EQ(board.get_piece(7, 6), 'n') << "Captured knight restored";
}

TEST_F(MoveUndoTest, KnightMoves) {
    board.set_starting_position();
    Board original = board;
    
    Move knight_move(0, 1, 2, 2, 'N'); // Nb1-c3
    BitboardMoveUndoData undo_data = board.make_move(knight_move);
    
    EXPECT_EQ(board.get_piece(2, 2), 'N') << "Knight moved to c3";
    EXPECT_EQ(board.get_piece(0, 1), '.') << "Knight left b1";
    
    board.undo_move(undo_data);
    EXPECT_TRUE(boards_equal(board, original)) << "Knight move undo restores position";
}

TEST_F(MoveUndoTest, BishopMoves) {
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPP1PPP/RNBQKB1R w KQkq - 0 1");
    Board original = board;
    
    Move bishop_move(0, 5, 3, 2, 'B'); // Bf1-c4
    BitboardMoveUndoData undo_data = board.make_move(bishop_move);
    
    EXPECT_EQ(board.get_piece(3, 2), 'B') << "Bishop moved to c4";
    EXPECT_EQ(board.get_piece(0, 5), '.') << "Bishop left f1";
    
    board.undo_move(undo_data);
    EXPECT_TRUE(boards_equal(board, original)) << "Bishop move undo restores position";
}

TEST_F(MoveUndoTest, RookMoves) {
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/1PPPPPPP/R1BQKBNR w KQkq - 0 1");
    Board original = board;
    
    Move rook_move(0, 0, 3, 0, 'R'); // Ra1-d1
    BitboardMoveUndoData undo_data = board.make_move(rook_move);
    
    EXPECT_EQ(board.get_piece(3, 0), 'R') << "Rook moved to d1";
    EXPECT_EQ(board.get_piece(0, 0), '.') << "Rook left a1";
    
    board.undo_move(undo_data);
    EXPECT_TRUE(boards_equal(board, original)) << "Rook move undo restores position";
}

TEST_F(MoveUndoTest, QueenMoves) {
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1");
    Board original = board;
    
    Move queen_move(0, 3, 4, 7, 'Q'); // Qd1-h5
    BitboardMoveUndoData undo_data = board.make_move(queen_move);
    
    EXPECT_EQ(board.get_piece(4, 7), 'Q') << "Queen moved to h5";
    EXPECT_EQ(board.get_piece(0, 3), '.') << "Queen left d1";
    
    board.undo_move(undo_data);
    EXPECT_TRUE(boards_equal(board, original)) << "Queen move undo restores position";
}

TEST_F(MoveUndoTest, KingMoves) {
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNB1KBNR w KQkq - 0 1");
    Board original = board;
    
    Move king_move(0, 4, 0, 3, 'K'); // Ke1-d1
    BitboardMoveUndoData undo_data = board.make_move(king_move);
    
    EXPECT_EQ(board.get_piece(0, 3), 'K') << "King moved to d1";
    EXPECT_EQ(board.get_piece(0, 4), '.') << "King left e1";
    EXPECT_EQ((board.get_castling_rights() & 0x03), 0) << "White castling rights removed";
    
    board.undo_move(undo_data);
    EXPECT_TRUE(boards_equal(board, original)) << "King move undo restores position";
    EXPECT_EQ((board.get_castling_rights() & 0x03), 0x03) << "Castling rights restored";
}

TEST_F(MoveUndoTest, Castling) {
    // Test white kingside castling
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQK2R w KQkq - 0 1");
    Board original = board;
    
    Move kingside_castle(0, 4, 0, 6, 'K', '.', '.', true); // O-O
    BitboardMoveUndoData undo_data = board.make_move(kingside_castle);
    
    EXPECT_EQ(board.get_piece(0, 6), 'K') << "King moved to g1";
    EXPECT_EQ(board.get_piece(0, 5), 'R') << "Rook moved to f1";
    EXPECT_EQ(board.get_piece(0, 4), '.') << "King left e1";
    EXPECT_EQ(board.get_piece(0, 7), '.') << "Rook left h1";
    
    board.undo_move(undo_data);
    EXPECT_TRUE(boards_equal(board, original)) << "Kingside castling undo restores position";
    
    // Test white queenside castling
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/R3KBNR w KQkq - 0 1");
    original = board;
    
    Move queenside_castle(0, 4, 0, 2, 'K', '.', '.', true); // O-O-O
    undo_data = board.make_move(queenside_castle);
    
    EXPECT_EQ(board.get_piece(0, 2), 'K') << "King moved to c1";
    EXPECT_EQ(board.get_piece(0, 3), 'R') << "Rook moved to d1";
    EXPECT_EQ(board.get_piece(0, 4), '.') << "King left e1";
    EXPECT_EQ(board.get_piece(0, 0), '.') << "Rook left a1";
    
    board.undo_move(undo_data);
    EXPECT_TRUE(boards_equal(board, original)) << "Queenside castling undo restores position";
}

TEST_F(MoveUndoTest, Captures) {
    board.set_from_fen("rnbqkbnr/ppp1pppp/8/3p4/3P4/8/PPP1PPPP/RNBQKBNR w KQkq - 0 2");
    Board original = board;
    
    Move capture_move(3, 3, 4, 3, 'P', 'p'); // dxd5
    BitboardMoveUndoData undo_data = board.make_move(capture_move);
    
    EXPECT_EQ(board.get_piece(4, 3), 'P') << "Capturing piece moved";
    EXPECT_EQ(board.get_piece(3, 3), '.') << "Capturing piece left origin";
    
    board.undo_move(undo_data);
    EXPECT_TRUE(boards_equal(board, original)) << "Capture undo restores position";
    EXPECT_EQ(board.get_piece(4, 3), 'p') << "Captured piece restored";
}

TEST_F(MoveUndoTest, ComplexPositions) {
    // Test a complex middlegame position
    board.set_from_fen("r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4");
    Board original = board;

    Move complex_move(4, 1, 5, 2, 'B', 'n'); // Bxc6+
    BitboardMoveUndoData undo_data = board.make_move(complex_move);
    
    EXPECT_EQ(board.get_piece(5, 2), 'B') << "Bishop captured knight";
    
    board.undo_move(undo_data);
    EXPECT_TRUE(boards_equal(board, original)) << "Complex position undo restores state";
}

TEST_F(MoveUndoTest, EdgeCases) {
    board.set_starting_position();
    Board original = board;
    
    // Test multiple moves and undos in sequence
    MoveList moves;
    moves.push_back(Move(1, 4, 3, 4, 'P')); // e2-e4
    moves.push_back(Move(6, 4, 4, 4, 'p')); // e7-e5
    moves.push_back(Move(0, 6, 2, 5, 'N')); // Ng1-f3
    moves.push_back(Move(7, 1, 5, 2, 'n')); // Nb8-c6
    
    std::vector<BitboardMoveUndoData> undo_data_list;
    
    // Make all moves
    for (const auto& move : moves) {
        undo_data_list.push_back(board.make_move(move));
    }
    
    // Undo all moves in reverse order
    for (int i = moves.size() - 1; i >= 0; i--) {
        board.undo_move(undo_data_list[i]);
    }
    
    EXPECT_TRUE(boards_equal(board, original)) << "Multiple move/undo sequence restores position";
}

TEST_F(MoveUndoTest, GameStatePreservation) {
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    Board original = board;
    
    uint8_t orig_castling = board.get_castling_rights();
    int8_t orig_ep = board.get_en_passant_file();
    int orig_halfmove = board.get_halfmove_clock();
    int orig_fullmove = board.get_fullmove_number();
    Board::Color orig_color = board.get_active_color();
    
    Move test_move(6, 3, 5, 3, 'p'); // d7-d6
    BitboardMoveUndoData undo_data = board.make_move(test_move);
    
    board.undo_move(undo_data);
    
    EXPECT_EQ(board.get_castling_rights(), orig_castling) << "Castling rights preserved";
    EXPECT_EQ(board.get_en_passant_file(), orig_ep) << "En passant file preserved";
    EXPECT_EQ(board.get_halfmove_clock(), orig_halfmove) << "Halfmove clock preserved";
    EXPECT_EQ(board.get_fullmove_number(), orig_fullmove) << "Fullmove number preserved";
    EXPECT_EQ(board.get_active_color(), orig_color) << "Active color preserved";
    EXPECT_TRUE(boards_equal(board, original)) << "Complete game state preserved";
}

TEST_F(MoveUndoTest, MoveSequencesAfterUndo) {
    // Test making moves after undoing previous moves
    board.set_starting_position();
    Board original = board;
    
    // Make a sequence of moves
    Move move1(1, 4, 3, 4, 'P'); // e2-e4
    Move move2(6, 4, 4, 4, 'p'); // e7-e5
    Move move3(0, 6, 2, 5, 'N'); // Ng1-f3
    
    BitboardMoveUndoData undo1 = board.make_move(move1);
    BitboardMoveUndoData undo2 = board.make_move(move2);
    BitboardMoveUndoData undo3 = board.make_move(move3);
    
    // Undo the last move
    board.undo_move(undo3);
    
    // Make a different move instead
    Move alternative_move(0, 1, 2, 2, 'N'); // Nb1-c3
    BitboardMoveUndoData undo_alt = board.make_move(alternative_move);
    
    EXPECT_EQ(board.get_piece(2, 2), 'N') << "Alternative knight move successful";
    EXPECT_EQ(board.get_piece(0, 1), '.') << "Knight left original square";
    EXPECT_EQ(board.get_piece(3, 4), 'P') << "Previous moves still intact";
    EXPECT_EQ(board.get_piece(4, 4), 'p') << "Previous moves still intact";
    
    // Test undoing all moves to return to start
    board.undo_move(undo_alt);
    board.undo_move(undo2);
    board.undo_move(undo1);
    
    EXPECT_TRUE(boards_equal(board, original)) << "Returned to starting position";
    
    // Test complex undo/redo scenario
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    
    Move complex1(6, 3, 4, 3, 'p'); // d7-d5
    Move complex2(3, 4, 4, 3, 'P', 'p'); // exd5
    Move complex3(7, 3, 4, 3, 'q', 'P'); // Qxd5
    
    BitboardMoveUndoData complex_undo1 = board.make_move(complex1);
    BitboardMoveUndoData complex_undo2 = board.make_move(complex2);
    BitboardMoveUndoData complex_undo3 = board.make_move(complex3);
    
    // Undo the queen capture
    board.undo_move(complex_undo3);
    
    EXPECT_EQ(board.get_piece(4, 3), 'P') << "White pawn restored on d5";
    EXPECT_EQ(board.get_piece(7, 3), 'q') << "Black queen back on d8";
}

TEST_F(MoveUndoTest, FriendlyFireCaptures) {
    board.set_starting_position();
    
    // Test white pawn trying to capture white piece
    Move white_pawn_friendly_fire(1, 3, 2, 4, 'P', 'P'); // d2 pawn tries to "capture" e3
    board.set_piece(2, 4, 'P'); // Place white pawn on e3
    
    EXPECT_FALSE(board.is_move_valid(white_pawn_friendly_fire)) << "White pawn cannot capture white pawn";
    
    // Test white knight trying to capture white piece
    Move white_knight_friendly_fire(0, 1, 2, 2, 'N', 'P'); // Nb1 tries to "capture" c3 white pawn
    board.set_piece(2, 2, 'P'); // Place white pawn on c3
    
    EXPECT_FALSE(board.is_move_valid(white_knight_friendly_fire)) << "White knight cannot capture white pawn";
    
    // Test black pieces trying to capture black pieces
    board.set_starting_position();
    board.set_active_color(Board::BLACK);
    
    // Black pawn tries to capture black piece
    Move black_pawn_friendly_fire(6, 3, 5, 4, 'p', 'p'); // d7 pawn tries to "capture" e6 black pawn
    board.set_piece(5, 4, 'p'); // Place black pawn on e6
    
    EXPECT_FALSE(board.is_move_valid(black_pawn_friendly_fire)) << "Black pawn cannot capture black pawn";
    
    // Black knight tries to capture black piece
    Move black_knight_friendly_fire(7, 1, 5, 2, 'n', 'p'); // Nb8 tries to "capture" c6 black pawn
    board.set_piece(5, 2, 'p'); // Place black pawn on c6
    
    EXPECT_FALSE(board.is_move_valid(black_knight_friendly_fire)) << "Black knight cannot capture black pawn";
}

TEST_F(MoveUndoTest, NonexistentPieceMoves) {
    board.set_starting_position();
    
    // Try to move a piece from an empty square
    Move empty_square_move(3, 3, 4, 3, 'P'); // Try to move pawn from empty d4
    EXPECT_FALSE(board.is_move_valid(empty_square_move)) << "Cannot move piece from empty square";
    
    // Try to move wrong piece type from occupied square
    Move wrong_piece_type(1, 4, 2, 4, 'N'); // Try to move knight from e2 (has pawn)
    EXPECT_FALSE(board.is_move_valid(wrong_piece_type)) << "Cannot move wrong piece type";
    
    // Try to move piece that was captured
    board.set_from_fen("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
    board.make_move(Move(3, 4, 4, 3, 'P', 'p')); // exd5
    
    Move move_captured_pawn(4, 3, 5, 3, 'p'); // Try to move the captured black pawn
    EXPECT_FALSE(board.is_move_valid(move_captured_pawn)) << "Cannot move captured piece";
}

TEST_F(MoveUndoTest, WrongColorMoves) {
    board.set_starting_position();
    
    // White to move, try to move black pieces
    Move move_black_pawn(6, 4, 5, 4, 'p'); // Try to move black e7 pawn
    EXPECT_FALSE(board.is_move_valid(move_black_pawn)) << "White cannot move black pawn";
    
    Move move_black_knight(7, 1, 5, 2, 'n'); // Try to move black Nb8
    EXPECT_FALSE(board.is_move_valid(move_black_knight)) << "White cannot move black knight";
    
    // Switch to black's turn
    board.set_active_color(Board::BLACK);
    
    // Black to move, try to move white pieces
    Move move_white_pawn(1, 4, 2, 4, 'P'); // Try to move white e2 pawn
    EXPECT_FALSE(board.is_move_valid(move_white_pawn)) << "Black cannot move white pawn";
    
    Move move_white_knight(0, 1, 2, 2, 'N'); // Try to move white Nb1
    EXPECT_FALSE(board.is_move_valid(move_white_knight)) << "Black cannot move white knight";
}

TEST_F(MoveUndoTest, InvalidCoordinates) {
    board.set_starting_position();
    
    // Test moves with coordinates outside the board
    Move off_board_from(-1, 4, 2, 4, 'P'); // From rank -1
    EXPECT_FALSE(board.is_move_valid(off_board_from)) << "Cannot move from rank -1";
    
    Move off_board_to(1, 4, 8, 4, 'P'); // To rank 8
    EXPECT_FALSE(board.is_move_valid(off_board_to)) << "Cannot move to rank 8";
    
    Move off_board_file(1, -1, 2, 4, 'P'); // From file -1
    EXPECT_FALSE(board.is_move_valid(off_board_file)) << "Cannot move from file -1";
    
    Move off_board_file_to(1, 4, 2, 8, 'P'); // To file 8
    EXPECT_FALSE(board.is_move_valid(off_board_file_to)) << "Cannot move to file 8";
}

TEST_F(MoveUndoTest, SameSquareMoves) {
    board.set_starting_position();
    
    // Try to move piece to same square
    Move same_square(1, 4, 1, 4, 'P'); // e2 to e2
    EXPECT_FALSE(board.is_move_valid(same_square)) << "Cannot move piece to same square";
    
    Move same_square_knight(0, 1, 0, 1, 'N'); // Nb1 to Nb1
    EXPECT_FALSE(board.is_move_valid(same_square_knight)) << "Cannot move knight to same square";
}

TEST_F(MoveUndoTest, InvalidEnPassant) {
    // Test en passant when no en passant is available
    board.set_starting_position();
    
    Move invalid_en_passant(3, 4, 4, 5, 'P', 'p', '.', false, true); // Try en passant when not available
    EXPECT_FALSE(board.is_move_valid(invalid_en_passant)) << "Cannot do en passant when not available";
    
    // Test en passant to wrong file
    board.set_from_fen("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
    
    Move wrong_file_en_passant(4, 4, 5, 3, 'P', 'p', '.', false, true); // Try en passant to d6 instead of f6
    EXPECT_FALSE(board.is_move_valid(wrong_file_en_passant)) << "Cannot do en passant to wrong file";
    
    // Test en passant with non-pawn piece
    Move non_pawn_en_passant(4, 4, 5, 5, 'N', 'p', '.', false, true); // Try en passant with knight
    EXPECT_FALSE(board.is_move_valid(non_pawn_en_passant)) << "Cannot do en passant with non-pawn";
}

TEST_F(MoveUndoTest, InvalidCastling) {
    board.set_starting_position();
    
    // Test castling with non-king piece
    Move non_king_castle(0, 0, 0, 2, 'R', '.', '.', true); // Try to castle with rook
    EXPECT_FALSE(board.is_move_valid(non_king_castle)) << "Cannot castle with non-king piece";
    
    // Test castling when pieces are in the way
    Move blocked_castle(0, 4, 0, 6, 'K', '.', '.', true); // Try kingside castle with pieces in way
    EXPECT_FALSE(board.is_move_valid(blocked_castle)) << "Cannot castle with pieces in the way";
}
