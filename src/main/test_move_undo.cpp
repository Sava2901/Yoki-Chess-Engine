#include "../board/Bitboard.h"
#include "../board/Board.h"
#include "../board/MoveGenerator.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>

class MoveUndoTester {
private:
    Board board;
    MoveGenerator generator;
    int tests_passed = 0;
    int tests_failed = 0;
    
    void assert_test(bool condition, const std::string& test_name) {
        if (condition) {
            std::cout << "✓ " << test_name << " PASSED\n";
            tests_passed++;
        } else {
            std::cout << "✗ " << test_name << " FAILED\n";
            tests_failed++;
        }
    }
    
    bool boards_equal(const Board& board1, const Board& board2) {
        return board1.to_fen() == board2.to_fen();
    }
    
    void print_board_state(const std::string& description) {
        std::cout << "\n" << description << ":\n";
        board.print();
        std::cout << std::endl;
    }
    
public:
    void run_all_tests() {
        std::cout << "=== Comprehensive Move/Undo Test Suite ===\n\n";
        
        BitboardUtils::init();
        
        test_basic_pawn_moves();
        test_pawn_captures();
        test_pawn_double_moves();
        test_en_passant();
        test_pawn_promotions();
        test_knight_moves();
        test_bishop_moves();
        test_rook_moves();
        test_queen_moves();
        test_king_moves();
        test_castling();
        test_captures(); // should be edge case
        test_complex_positions();
        test_edge_cases();
        test_game_state_preservation();
        test_move_sequences_after_undo();
        test_illegal_moves();
        
        print_summary();
    }
    
    void test_basic_pawn_moves() {
        std::cout << "\n--- Testing Basic Pawn Moves ---\n";
        
        // Test white pawn single move
        board.set_starting_position();
        Board original = board;
        
        Move pawn_move(1, 4, 2, 4, 'P'); // e2-e3
        print_board_state("Before pawn move e2-e3");
        BitboardMoveUndoData undo_data = board.make_move(pawn_move);
        print_board_state("After pawn move e2-e3");
        
        assert_test(board.get_piece(2, 4) == 'P', "White pawn moved to e3");
        assert_test(board.get_piece(1, 4) == '.', "White pawn left e2");
        assert_test(board.get_active_color() == Board::BLACK, "Turn switched to black");
        
        board.undo_move(undo_data);
        print_board_state("After undoing pawn move");
        assert_test(boards_equal(board, original), "Pawn move undo restores position");
        
        // Test black pawn single move
        board.set_starting_position();
        board.set_active_color(Board::BLACK);
        original = board;
        
        Move black_pawn_move(6, 3, 5, 3, 'p'); // d7-d6
        print_board_state("Before black pawn move d7-d6");
        undo_data = board.make_move(black_pawn_move);
        print_board_state("After black pawn move d7-d6");
        
        assert_test(board.get_piece(5, 3) == 'p', "Black pawn moved to d6");
        assert_test(board.get_piece(6, 3) == '.', "Black pawn left d7");
        
        board.undo_move(undo_data);
        print_board_state("After undoing black pawn move");
        assert_test(boards_equal(board, original), "Black pawn move undo restores position");
    }
    
    void test_pawn_captures() {
        std::cout << "\n--- Testing Pawn Captures ---\n";
        
        board.set_from_fen("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
        Board original = board;

        Move capture_move(3, 4, 4, 3, 'P', 'p'); // exd5

        print_board_state("Before pawn capture exd5");
        BitboardMoveUndoData undo_data = board.make_move(capture_move);
        print_board_state("After pawn capture exd5");
        
        assert_test(board.get_piece(4, 3) == 'P', "White pawn captured on d5");
        assert_test(board.get_piece(3, 4) == '.', "White pawn left e4");
        
        board.undo_move(undo_data);
        print_board_state("After undoing pawn capture");
        assert_test(boards_equal(board, original), "Pawn capture undo restores position");
        assert_test(board.get_piece(4, 3) == 'p', "Captured pawn restored");
    }
    
    void test_pawn_double_moves() {
        std::cout << "\n--- Testing Pawn Double Moves ---\n";
        
        board.set_starting_position();
        Board original = board;
        
        Move double_move(1, 4, 3, 4, 'P'); // e2-e4
        print_board_state("Before pawn double move e2-e4");
        BitboardMoveUndoData undo_data = board.make_move(double_move);
        print_board_state("After pawn double move e2-e4");
        
        assert_test(board.get_piece(3, 4) == 'P', "White pawn moved to e4");
        assert_test(board.get_en_passant_file() == 4, "En passant file set to e");
        
        board.undo_move(undo_data);
        print_board_state("After undoing pawn double move");
        assert_test(boards_equal(board, original), "Pawn double move undo restores position");
        assert_test(board.get_en_passant_file() == -1, "En passant file restored");
    }
    
    void test_en_passant() {
        std::cout << "\n--- Testing En Passant ---\n";
        
        board.set_from_fen("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
        Board original = board;
        
        Move en_passant_move(4, 4, 5, 5, 'P', 'p', '.', false, true); // exf6 e.p.
        print_board_state("Before en passant capture exf6");
        BitboardMoveUndoData undo_data = board.make_move(en_passant_move);
        print_board_state("After en passant capture exf6");
        
        assert_test(board.get_piece(5, 5) == 'P', "White pawn moved to f6");
        assert_test(board.get_piece(4, 5) == '.', "Captured pawn removed from f5");
        assert_test(board.get_piece(4, 4) == '.', "White pawn left e5");
        
        board.undo_move(undo_data);
        print_board_state("After undoing en passant capture");
        assert_test(boards_equal(board, original), "En passant undo restores position");
        assert_test(board.get_piece(4, 5) == 'p', "Captured pawn restored");
    }
    
    void test_pawn_promotions() {
        std::cout << "\n--- Testing Pawn Promotions ---\n";
        
        board.set_from_fen("rnbqkbn1/pppppppP/8/8/8/8/PPPPPPP1/RNBQKBNR w KQq - 0 1");
        Board original = board;
        
        Move promotion_move(6, 7, 7, 7, 'P', '.', 'Q'); // h7-h8=Q
        print_board_state("Before pawn promotion h7-h8=Q");
        BitboardMoveUndoData undo_data = board.make_move(promotion_move);
        print_board_state("After pawn promotion h7-h8=Q");
        
        assert_test(board.get_piece(7, 7) == 'Q', "Pawn promoted to queen");
        assert_test(board.get_piece(6, 7) == '.', "Pawn left h7");
        
        board.undo_move(undo_data);
        print_board_state("After undoing pawn promotion");
        assert_test(boards_equal(board, original), "Promotion undo restores position");
        assert_test(board.get_piece(6, 7) == 'P', "Pawn restored on h7");
        
        // Test promotion with capture
        board.set_from_fen("rnbqkbnr/pppppppP/8/8/8/8/PPPPPPP1/RNBQKBN1 w Qkq - 0 1");
        original = board;
        
        Move promotion_capture(6, 7, 7, 6, 'P', 'n', 'Q'); // hxg8=Q
        print_board_state("Before promotion capture hxg8=Q");
        undo_data = board.make_move(promotion_capture);
        print_board_state("After promotion capture hxg8=Q");
        
        assert_test(board.get_piece(7, 6) == 'Q', "Pawn promoted to queen with capture");
        
        board.undo_move(undo_data);
        print_board_state("After undoing promotion capture");
        assert_test(boards_equal(board, original), "Promotion capture undo restores position");
        assert_test(board.get_piece(7, 6) == 'n', "Captured knight restored");
    }
    
    void test_knight_moves() {
        std::cout << "\n--- Testing Knight Moves ---\n";
        
        board.set_starting_position();
        Board original = board;
        
        Move knight_move(0, 1, 2, 2, 'N'); // Nb1-c3
        print_board_state("Before knight move Nb1-c3");
        BitboardMoveUndoData undo_data = board.make_move(knight_move);
        print_board_state("After knight move Nb1-c3");
        
        assert_test(board.get_piece(2, 2) == 'N', "Knight moved to c3");
        assert_test(board.get_piece(0, 1) == '.', "Knight left b1");
        
        board.undo_move(undo_data);
        print_board_state("After undoing knight move");
        assert_test(boards_equal(board, original), "Knight move undo restores position");
    }
    
    void test_bishop_moves() {
        std::cout << "\n--- Testing Bishop Moves ---\n";
        
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPP1PPP/RNBQKB1R w KQkq - 0 1");
        Board original = board;
        
        Move bishop_move(0, 5, 3, 2, 'B'); // Bf1-c4
        print_board_state("Before bishop move Bf1-c4");
        BitboardMoveUndoData undo_data = board.make_move(bishop_move);
        print_board_state("After bishop move Bf1-c4");
        
        assert_test(board.get_piece(3, 2) == 'B', "Bishop moved to c4");
        assert_test(board.get_piece(0, 5) == '.', "Bishop left f1");
        
        board.undo_move(undo_data);
        print_board_state("After undoing bishop move");
        assert_test(boards_equal(board, original), "Bishop move undo restores position");
    }
    
    void test_rook_moves() {
        std::cout << "\n--- Testing Rook Moves ---\n";
        
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/1PPPPPPP/R1BQKBNR w KQkq - 0 1");
        Board original = board;
        
        Move rook_move(0, 0, 3, 0, 'R'); // Ra1-d1
        print_board_state("Before rook move Ra1-d1");
        BitboardMoveUndoData undo_data = board.make_move(rook_move);
        print_board_state("After rook move Ra1-d1");
        
        assert_test(board.get_piece(3, 0) == 'R', "Rook moved to d1");
        assert_test(board.get_piece(0, 0) == '.', "Rook left a1");
        
        board.undo_move(undo_data);
        print_board_state("After undoing rook move");
        assert_test(boards_equal(board, original), "Rook move undo restores position");
    }
    
    void test_queen_moves() {
        std::cout << "\n--- Testing Queen Moves ---\n";
        
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1");
        Board original = board;
        
        Move queen_move(0, 3, 4, 7, 'Q'); // Qd1-h5
        print_board_state("Before queen move Qd1-h5");
        BitboardMoveUndoData undo_data = board.make_move(queen_move);
        print_board_state("After queen move Qd1-h5");
        
        assert_test(board.get_piece(4, 7) == 'Q', "Queen moved to h5");
        assert_test(board.get_piece(0, 3) == '.', "Queen left d1");
        
        board.undo_move(undo_data);
        print_board_state("After undoing queen move");
        assert_test(boards_equal(board, original), "Queen move undo restores position");
    }
    
    void test_king_moves() {
        std::cout << "\n--- Testing King Moves ---\n";
        
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNB1KBNR w KQkq - 0 1");
        Board original = board;
        
        Move king_move(0, 4, 0, 3, 'K'); // Ke1-d1
        print_board_state("Before king move Ke1-d1");
        BitboardMoveUndoData undo_data = board.make_move(king_move);
        print_board_state("After king move Ke1-d1");
        
        assert_test(board.get_piece(0, 3) == 'K', "King moved to d1");
        assert_test(board.get_piece(0, 4) == '.', "King left e1");
        assert_test((board.get_castling_rights() & 0x03) == 0, "White castling rights removed");
        
        board.undo_move(undo_data);
        print_board_state("After undoing king move");
        assert_test(boards_equal(board, original), "King move undo restores position");
        assert_test((board.get_castling_rights() & 0x03) == 0x03, "Castling rights restored");
    }
    
    void test_castling() {
        std::cout << "\n--- Testing Castling ---\n";
        
        // Test white kingside castling
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQK2R w KQkq - 0 1");
        Board original = board;
        
        Move kingside_castle(0, 4, 0, 6, 'K', '.', '.', true); // O-O
        print_board_state("Before kingside castling O-O");
        BitboardMoveUndoData undo_data = board.make_move(kingside_castle);
        print_board_state("After kingside castling O-O");
        
        assert_test(board.get_piece(0, 6) == 'K', "King moved to g1");
        assert_test(board.get_piece(0, 5) == 'R', "Rook moved to f1");
        assert_test(board.get_piece(0, 4) == '.', "King left e1");
        assert_test(board.get_piece(0, 7) == '.', "Rook left h1");
        
        board.undo_move(undo_data);
        print_board_state("After undoing kingside castling");
        assert_test(boards_equal(board, original), "Kingside castling undo restores position");
        
        // Test white queenside castling
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/R3KBNR w KQkq - 0 1");
        original = board;
        
        Move queenside_castle(0, 4, 0, 2, 'K', '.', '.', true); // O-O-O
        print_board_state("Before queenside castling O-O-O");
        undo_data = board.make_move(queenside_castle);
        print_board_state("After queenside castling O-O-O");
        
        assert_test(board.get_piece(0, 2) == 'K', "King moved to c1");
        assert_test(board.get_piece(0, 3) == 'R', "Rook moved to d1");
        assert_test(board.get_piece(0, 4) == '.', "King left e1");
        assert_test(board.get_piece(0, 0) == '.', "Rook left a1");
        
        board.undo_move(undo_data);
        print_board_state("After undoing queenside castling");
        assert_test(boards_equal(board, original), "Queenside castling undo restores position");
    }
    
    void test_captures() {
        std::cout << "\n--- Testing Various Captures ---\n";
        
        board.set_from_fen("rnbqkbnr/ppp1pppp/8/3p4/3P4/8/PPP1PPPP/RNBQKBNR w KQkq - 0 2");
        Board original = board;
        
        Move capture_move(3, 3, 4, 3, 'P', 'p'); // dxd5
        print_board_state("Before capture dxd5");
        BitboardMoveUndoData undo_data = board.make_move(capture_move);
        print_board_state("After capture dxd5");
        
        assert_test(board.get_piece(4, 3) == 'P', "Capturing piece moved");
        assert_test(board.get_piece(3, 3) == '.', "Capturing piece left origin");
        
        board.undo_move(undo_data);
        print_board_state("After undoing capture");
        assert_test(boards_equal(board, original), "Capture undo restores position");
        assert_test(board.get_piece(4, 3) == 'p', "Captured piece restored");
    }
    
    void test_complex_positions() {
        std::cout << "\n--- Testing Complex Positions ---\n";
        
        // Test a complex middlegame position
        board.set_from_fen("r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4");
        Board original = board;

        MoveGenerator generator;
        std::vector<Move> legal_moves = generator.generate_legal_moves(board);
        for (auto move : legal_moves) {
            std::cout << move.to_algebraic() << std::endl;
        }

        // std::cout << board.is_square_attacked(18, Board::WHITE) << std::endl;

        Move complex_move(4, 1, 5, 2, 'B', 'n'); // Bxc6+
        print_board_state("Before complex move Bxc6+");
        BitboardMoveUndoData undo_data = board.make_move(complex_move);
        print_board_state("After complex move Bxc6+");
        
        assert_test(board.get_piece(5, 2) == 'B', "Bishop captured knight");
        
        board.undo_move(undo_data);
        print_board_state("After undoing complex move");
        assert_test(boards_equal(board, original), "Complex position undo restores state");
    }
    
    void test_edge_cases() {
        std::cout << "\n--- Testing Edge Cases ---\n";
        
        // Test move to same square (should be invalid but test undo anyway)
        board.set_starting_position();
        Board original = board;
        
        // Test multiple moves and undos in sequence
        std::vector<Move> moves = {
            Move(1, 4, 3, 4, 'P'), // e2-e4
            Move(6, 4, 4, 4, 'p'), // e7-e5
            Move(0, 6, 2, 5, 'N'), // Ng1-f3
            Move(7, 1, 5, 2, 'n')  // Nb8-c6
        };
        
        std::vector<BitboardMoveUndoData> undo_data_list;
        
        // Make all moves
        for (const auto& move : moves) {
            print_board_state("Before move " + move.to_algebraic());
            undo_data_list.push_back(board.make_move(move));
            print_board_state("After move " + move.to_algebraic());
        }
        
        // Undo all moves in reverse order
        for (int i = moves.size() - 1; i >= 0; i--) {
            print_board_state("Before undoing " + moves[i].to_algebraic());
            board.undo_move(undo_data_list[i]);
            print_board_state("After undoing " + moves[i].to_algebraic());
        }
        
        assert_test(boards_equal(board, original), "Multiple move/undo sequence restores position");
    }
    
    void test_game_state_preservation() {
        std::cout << "\n--- Testing Game State Preservation ---\n";
        
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
        Board original = board;
        
        uint8_t orig_castling = board.get_castling_rights();
        int8_t orig_ep = board.get_en_passant_file();
        int orig_halfmove = board.get_halfmove_clock();
        int orig_fullmove = board.get_fullmove_number();
        Board::Color orig_color = board.get_active_color();
        
        Move test_move(6, 3, 5, 3, 'p'); // d7-d6
        print_board_state("Before test move d7-d6");
        BitboardMoveUndoData undo_data = board.make_move(test_move);
        print_board_state("After test move d7-d6");
        
        board.undo_move(undo_data);
        print_board_state("After undoing test move");
        
        assert_test(board.get_castling_rights() == orig_castling, "Castling rights preserved");
        assert_test(board.get_en_passant_file() == orig_ep, "En passant file preserved");
        assert_test(board.get_halfmove_clock() == orig_halfmove, "Halfmove clock preserved");
        assert_test(board.get_fullmove_number() == orig_fullmove, "Fullmove number preserved");
        assert_test(board.get_active_color() == orig_color, "Active color preserved");
        assert_test(boards_equal(board, original), "Complete game state preserved");
    }
    
    void test_move_sequences_after_undo() {
        std::cout << "\n--- Testing Move Sequences After Undo ---\n";
        
        // Test making moves after undoing previous moves
        board.set_starting_position();
        Board original = board;
        print_board_state("Starting position");
        
        // Make a sequence of moves
        Move move1(1, 4, 3, 4, 'P'); // e2-e4
        Move move2(6, 4, 4, 4, 'p'); // e7-e5
        Move move3(0, 6, 2, 5, 'N'); // Ng1-f3
        
        print_board_state("Before first move e2-e4");
        BitboardMoveUndoData undo1 = board.make_move(move1);
        print_board_state("After e2-e4");
        
        print_board_state("Before second move e7-e5");
        BitboardMoveUndoData undo2 = board.make_move(move2);
        print_board_state("After e7-e5");
        
        print_board_state("Before third move Ng1-f3");
        BitboardMoveUndoData undo3 = board.make_move(move3);
        print_board_state("After Ng1-f3");
        
        // Undo the last move
        print_board_state("Before undoing Ng1-f3");
        board.undo_move(undo3);
        print_board_state("After undoing Ng1-f3");
        
        // Make a different move instead
        Move alternative_move(0, 1, 2, 2, 'N'); // Nb1-c3
        print_board_state("Before alternative move Nb1-c3");
        BitboardMoveUndoData undo_alt = board.make_move(alternative_move);
        print_board_state("After alternative move Nb1-c3");
        
        assert_test(board.get_piece(2, 2) == 'N', "Alternative knight move successful");
        assert_test(board.get_piece(0, 1) == '.', "Knight left original square");
        assert_test(board.get_piece(3, 4) == 'P', "Previous moves still intact");
        assert_test(board.get_piece(4, 4) == 'p', "Previous moves still intact");
        
        // Test undoing all moves to return to start
        board.undo_move(undo_alt);
        board.undo_move(undo2);
        board.undo_move(undo1);
        print_board_state("After undoing all moves");
        
        assert_test(boards_equal(board, original), "Returned to starting position");
        
        // Test complex undo/redo scenario
        std::cout << "\n--- Testing Complex Undo/Redo Scenario ---\n";
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
        print_board_state("Complex starting position");
        
        Move complex1(6, 3, 4, 3, 'p'); // d7-d5
        Move complex2(3, 4, 4, 3, 'P', 'p'); // exd5
        Move complex3(7, 3, 4, 3, 'q', 'P'); // Qxd5
        
        print_board_state("Before d7-d5");
        BitboardMoveUndoData complex_undo1 = board.make_move(complex1);
        print_board_state("After d7-d5");
        
        print_board_state("Before exd5");
        BitboardMoveUndoData complex_undo2 = board.make_move(complex2);
        print_board_state("After exd5");
        
        print_board_state("Before Qxd5");
        BitboardMoveUndoData complex_undo3 = board.make_move(complex3);
        print_board_state("After Qxd5");
        
        // Undo the queen capture
        print_board_state("Before undoing Qxd5");
        board.undo_move(complex_undo3);
        print_board_state("After undoing Qxd5");
        
        assert_test(board.get_piece(4, 3) == 'P', "White pawn restored on d5");
        assert_test(board.get_piece(7, 3) == 'q', "Black queen back on d8");
        
        std::cout << "\n--- Move Sequences After Undo Test Complete ---\n";
    }
    
    void test_illegal_moves() {
        std::cout << "\n--- Testing Illegal Moves ---\n";
        
        // Test friendly fire captures
        test_friendly_fire_captures();
        
        // Test moving pieces that don't exist
        test_nonexistent_piece_moves();
        
        // Test moving opponent's pieces
        test_wrong_color_moves();
        
        // Test invalid coordinates
        test_invalid_coordinates();
        
        // Test invalid en passant
        test_invalid_en_passant();
        
        // Test invalid castling
        test_invalid_castling();
        
        // Test moving to same square
        test_same_square_moves();
        
        std::cout << "\n--- Illegal Moves Test Complete ---\n";
    }
    
    void test_friendly_fire_captures() {
        std::cout << "\n--- Testing Friendly Fire Captures ---\n";
        
        board.set_starting_position();
        print_board_state("Starting position for friendly fire tests");
        
        // Test white pawn trying to capture white piece
        Move white_pawn_friendly_fire(1, 3, 2, 4, 'P', 'P'); // d2 pawn tries to "capture" e3 (if white pawn were there)
        board.set_piece(2, 4, 'P'); // Place white pawn on e3
        print_board_state("White pawn placed on e3");
        
        assert_test(!board.is_move_valid(white_pawn_friendly_fire), "White pawn cannot capture white pawn");
        
        // Test white knight trying to capture white piece
        Move white_knight_friendly_fire(0, 1, 2, 2, 'N', 'P'); // Nb1 tries to "capture" c3 white pawn
        board.set_piece(2, 2, 'P'); // Place white pawn on c3
        print_board_state("White pawn placed on c3");
        
        assert_test(!board.is_move_valid(white_knight_friendly_fire), "White knight cannot capture white pawn");
        
        // Test white bishop trying to capture white piece
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/2P5/PP1PPPPP/RNBQKBNR w KQkq - 0 1"); // c3 pawn
        print_board_state("Position with white pawn on c3");
        
        Move white_bishop_friendly_fire(0, 5, 3, 2, 'B', 'P'); // Bf1 tries to "capture" c4 white pawn
        board.set_piece(3, 2, 'P'); // Place white pawn on c4
        print_board_state("White pawn placed on c4");
        
        assert_test(!board.is_move_valid(white_bishop_friendly_fire), "White bishop cannot capture white pawn");
        
        // Test white rook trying to capture white piece
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/R1BQKBNR w KQkq - 0 1"); // Rook on a1, no knight on b1
        Move white_rook_friendly_fire(0, 0, 0, 1, 'R', 'B'); // Ra1 tries to "capture" b1 white bishop
        board.set_piece(0, 1, 'B'); // Place white bishop on b1
        print_board_state("White bishop placed on b1");
        
        assert_test(!board.is_move_valid(white_rook_friendly_fire), "White rook cannot capture white bishop");
        
        // Test white queen trying to capture white piece
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1"); // No pawn on e2
        Move white_queen_friendly_fire(0, 3, 1, 4, 'Q', 'P'); // Qd1 tries to "capture" e2 white pawn
        board.set_piece(1, 4, 'P'); // Place white pawn on e2
        print_board_state("White pawn placed on e2");
        
        assert_test(!board.is_move_valid(white_queen_friendly_fire), "White queen cannot capture white pawn");
        
        // Test white king trying to capture white piece
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1"); // No pawn on e2
        Move white_king_friendly_fire(0, 4, 1, 4, 'K', 'P'); // Ke1 tries to "capture" e2 white pawn
        board.set_piece(1, 4, 'P'); // Place white pawn on e2
        print_board_state("White pawn placed on e2");
        
        assert_test(!board.is_move_valid(white_king_friendly_fire), "White king cannot capture white pawn");
        
        // Test black pieces trying to capture black pieces
        board.set_starting_position();
        board.set_active_color(Board::BLACK);
        print_board_state("Starting position, black to move");
        
        // Black pawn tries to capture black piece
        Move black_pawn_friendly_fire(6, 3, 5, 4, 'p', 'p'); // d7 pawn tries to "capture" e6 black pawn
        board.set_piece(5, 4, 'p'); // Place black pawn on e6
        print_board_state("Black pawn placed on e6");
        
        assert_test(!board.is_move_valid(black_pawn_friendly_fire), "Black pawn cannot capture black pawn");
        
        // Black knight tries to capture black piece
        Move black_knight_friendly_fire(7, 1, 5, 2, 'n', 'p'); // Nb8 tries to "capture" c6 black pawn
        board.set_piece(5, 2, 'p'); // Place black pawn on c6
        print_board_state("Black pawn placed on c6");
        
        assert_test(!board.is_move_valid(black_knight_friendly_fire), "Black knight cannot capture black pawn");
    }
    
    void test_nonexistent_piece_moves() {
        std::cout << "\n--- Testing Moves with Nonexistent Pieces ---\n";
        
        board.set_starting_position();
        print_board_state("Starting position for nonexistent piece tests");
        
        // Try to move a piece from an empty square
        Move empty_square_move(3, 3, 4, 3, 'P'); // Try to move pawn from empty d4
        assert_test(!board.is_move_valid(empty_square_move), "Cannot move piece from empty square");
        
        // Try to move wrong piece type from occupied square
        Move wrong_piece_type(1, 4, 2, 4, 'N'); // Try to move knight from e2 (has pawn)
        assert_test(!board.is_move_valid(wrong_piece_type), "Cannot move wrong piece type");
        
        // Try to move piece that was captured
        board.set_from_fen("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
        board.make_move(Move(3, 4, 4, 3, 'P', 'p')); // exd5
        print_board_state("After white captures on d5");
        
        Move move_captured_pawn(4, 3, 5, 3, 'p'); // Try to move the captured black pawn
        assert_test(!board.is_move_valid(move_captured_pawn), "Cannot move captured piece");
    }
    
    void test_wrong_color_moves() {
        std::cout << "\n--- Testing Wrong Color Moves ---\n";
        
        board.set_starting_position();
        print_board_state("Starting position, white to move");
        
        // White to move, try to move black pieces
        Move move_black_pawn(6, 4, 5, 4, 'p'); // Try to move black e7 pawn
        assert_test(!board.is_move_valid(move_black_pawn), "White cannot move black pawn");
        
        Move move_black_knight(7, 1, 5, 2, 'n'); // Try to move black Nb8
        assert_test(!board.is_move_valid(move_black_knight), "White cannot move black knight");
        
        // Switch to black's turn
        board.set_active_color(Board::BLACK);
        print_board_state("Same position, black to move");
        
        // Black to move, try to move white pieces
        Move move_white_pawn(1, 4, 2, 4, 'P'); // Try to move white e2 pawn
        assert_test(!board.is_move_valid(move_white_pawn), "Black cannot move white pawn");
        
        Move move_white_knight(0, 1, 2, 2, 'N'); // Try to move white Nb1
        assert_test(!board.is_move_valid(move_white_knight), "Black cannot move white knight");
    }
    
    void test_invalid_coordinates() {
        std::cout << "\n--- Testing Invalid Coordinates ---\n";
        
        board.set_starting_position();
        print_board_state("Starting position for coordinate tests");
        
        // Test moves with coordinates outside the board
        Move off_board_from(-1, 4, 2, 4, 'P'); // From rank -1
        assert_test(!board.is_move_valid(off_board_from), "Cannot move from rank -1");
        
        Move off_board_to(1, 4, 8, 4, 'P'); // To rank 8
        assert_test(!board.is_move_valid(off_board_to), "Cannot move to rank 8");
        
        Move off_board_file(1, -1, 2, 4, 'P'); // From file -1
        assert_test(!board.is_move_valid(off_board_file), "Cannot move from file -1");
        
        Move off_board_file_to(1, 4, 2, 8, 'P'); // To file 8
        assert_test(!board.is_move_valid(off_board_file_to), "Cannot move to file 8");
    }
    
    void test_same_square_moves() {
        std::cout << "\n--- Testing Same Square Moves ---\n";
        
        board.set_starting_position();
        print_board_state("Starting position for same square tests");
        
        // Try to move piece to same square
        Move same_square(1, 4, 1, 4, 'P'); // e2 to e2
        assert_test(!board.is_move_valid(same_square), "Cannot move piece to same square");
        
        Move same_square_knight(0, 1, 0, 1, 'N'); // Nb1 to Nb1
        assert_test(!board.is_move_valid(same_square_knight), "Cannot move knight to same square");
    }
    
    void test_invalid_en_passant() {
        std::cout << "\n--- Testing Invalid En Passant ---\n";
        
        // Test en passant when no en passant is available
        board.set_starting_position();
        print_board_state("Starting position (no en passant available)");
        
        Move invalid_en_passant(3, 4, 4, 5, 'P', 'p', '.', false, true); // Try en passant when not available
        assert_test(!board.is_move_valid(invalid_en_passant), "Cannot do en passant when not available");
        
        // Test en passant to wrong file
        board.set_from_fen("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
        print_board_state("Position with en passant available on f6");
        
        Move wrong_file_en_passant(4, 4, 5, 3, 'P', 'p', '.', false, true); // Try en passant to d6 instead of f6
        assert_test(!board.is_move_valid(wrong_file_en_passant), "Cannot do en passant to wrong file");
        
        // Test en passant with non-pawn piece
        Move non_pawn_en_passant(4, 4, 5, 5, 'N', 'p', '.', false, true); // Try en passant with knight
        assert_test(!board.is_move_valid(non_pawn_en_passant), "Cannot do en passant with non-pawn");
    }
    
    void test_invalid_castling() {
        std::cout << "\n--- Testing Invalid Castling ---\n";
        
        board.set_starting_position();
        print_board_state("Starting position for castling tests");
        
        // Test castling with non-king piece
        Move non_king_castle(0, 0, 0, 2, 'R', '.', '.', true); // Try to castle with rook
        assert_test(!board.is_move_valid(non_king_castle), "Cannot castle with non-king piece");
        
        // Test castling when pieces are in the way
        Move blocked_castle(0, 4, 0, 6, 'K', '.', '.', true); // Try kingside castle with pieces in way
        assert_test(!board.is_move_valid(blocked_castle), "Cannot castle with pieces in the way");
        
        // Additional castling validation would require more complex setup
        // This is a basic test for the piece type validation
    }
    
    void print_summary() {
        std::cout << "\n=== Test Summary ===\n";
        std::cout << "Tests Passed: " << tests_passed << "\n";
        std::cout << "Tests Failed: " << tests_failed << "\n";
        std::cout << "Total Tests: " << (tests_passed + tests_failed) << "\n";
        
        if (tests_failed == 0) {
            std::cout << "\n🎉 ALL TESTS PASSED! 🎉\n";
        } else {
            std::cout << "\n❌ Some tests failed. Please review the implementation.\n";
        }
    }
};

int main() {
    std::cout << "Move/Undo Comprehensive Test Suite\n";
    std::cout << "==================================\n";
    
    try {
        MoveUndoTester tester;
        tester.run_all_tests();
    } catch (const std::exception& e) {
        std::cerr << "Error during testing: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}