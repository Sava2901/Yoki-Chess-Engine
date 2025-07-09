#include "../engine/Engine.h"
#include "../engine/Search.h"
#include "../engine/Evaluation.h"
#include <iostream>
#include <fstream>
#include <string>
#include "../board/Board.h"
#include "../board/Move.h"
#include "../board/MoveGenerator.h"

class TeeStreambuf : public std::streambuf {
public:
    TeeStreambuf(std::streambuf* sb1, std::streambuf* sb2) : sb1_(sb1), sb2_(sb2) {}
    
protected:
    int overflow(int c) override {
        if (c == EOF) {
            return !EOF;
        } else {
            int const r1 = sb1_->sputc(c);
            int const r2 = sb2_->sputc(c);
            return r1 == EOF || r2 == EOF ? EOF : c;
        }
    }
    
    int sync() override {
        int const r1 = sb1_->pubsync();
        int const r2 = sb2_->pubsync();
        return r1 == 0 && r2 == 0 ? 0 : -1;
    }
    
private:
    std::streambuf* sb1_;
    std::streambuf* sb2_;
};

int main() {
    // Open results file for output
    std::ofstream results("results.txt");
    if (!results.is_open()) {
        std::cerr << "Error: Could not open results.txt for writing" << std::endl;
        return 1;
    }
    
    // Create tee streambuf to output to both console and file
    std::streambuf* original_cout = std::cout.rdbuf();
    TeeStreambuf tee(std::cout.rdbuf(), results.rdbuf());
    std::cout.rdbuf(&tee);
    
    std::cout << "=== Yoki Chess Engine Test Results ===" << std::endl;
    std::cout << std::endl;
    
    try {
        // Create engine and board instances
        std::cout << "Creating engine and board instances..." << std::endl;
        Engine engine;
        Board board;
        
        // Test 0: Move Generation - Starting Position
        std::cout << "\n=== Test 0: Move Generation - Starting Position ===" << std::endl;
        board.set_position("8/8/8/8/8/6k1/8/6K1 w - - 0 1");

        MoveList pseudo_moves = MoveGenerator::generate_pseudo_legal_moves(board);
        MoveList legal_moves = MoveGenerator::generate_legal_moves(board);

        std::cout << "\nPseudo-legal moves: " << pseudo_moves.size() << std::endl;
        std::cout << "Legal moves: " << legal_moves.size() << std::endl;

        std::cout << "\nFirst 10 legal moves:" << std::endl;
        for (size_t i = 0; i < std::min(size_t(10), legal_moves.size()); i++) {
            std::cout << i + 1 << ". " << legal_moves[i].to_algebraic() << std::endl;
        }

        // Test 1: Move Generation - Starting Position
        std::cout << "\n=== Test 1: Move Generation - Starting Position ===" << std::endl;
        board.set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

        pseudo_moves = MoveGenerator::generate_pseudo_legal_moves(board);
        legal_moves = MoveGenerator::generate_legal_moves(board);

        std::cout << "\nPseudo-legal moves: " << pseudo_moves.size() << std::endl;
        std::cout << "Legal moves: " << legal_moves.size() << std::endl;

        std::cout << "\nFirst 10 legal moves:" << std::endl;
        for (size_t i = 0; i < std::min(size_t(10), legal_moves.size()); i++) {
            std::cout << i + 1 << ". " << legal_moves[i].to_algebraic() << std::endl;
        }
        
        // Test 2: Check Detection
        std::cout << "\n=== Test 2: Check Detection ===" << std::endl;
        bool white_in_check = MoveGenerator::is_in_check(board, 'w');
        bool black_in_check = MoveGenerator::is_in_check(board, 'b');
        std::cout << "White in check: " << (white_in_check ? "Yes" : "No") << std::endl;
        std::cout << "Black in check: " << (black_in_check ? "Yes" : "No") << std::endl;

        // Test 3: Move Generation After 1.e4
        std::cout << "\n=== Test 3: Move Generation After 1.e4 ===" << std::endl;
        board.set_position("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
        std::cout << "Board position after 1.e4:" << std::endl;

        legal_moves = MoveGenerator::generate_legal_moves(board);
        std::cout << "\nBlack has " << legal_moves.size() << " legal moves" << std::endl;

        std::cout << "\nFirst 10 legal moves for black:" << std::endl;
        for (size_t i = 0; i < std::min(size_t(10), legal_moves.size()); i++) {
            std::cout << i + 1 << ". " << legal_moves[i].to_algebraic() << std::endl;
        }
        
        // Test 4: En Passant Moves
        std::cout << "\n=== Test 4: En Passant Moves ===" << std::endl;
        board.set_position("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
        std::cout << "Position with en passant opportunity (f6):" << std::endl;
        
        legal_moves = MoveGenerator::generate_legal_moves(board);
        std::cout << "\nLegal moves (should include en passant): " << legal_moves.size() << std::endl;
        
        // Look for en passant moves
        int en_passant_count = 0;
        for (const Move& move : legal_moves) {
            if (move.is_en_passant) {
                std::cout << "En passant move found: " << move.to_algebraic() << std::endl;
                en_passant_count++;
            }
        }
        std::cout << "Total en passant moves: " << en_passant_count << std::endl;
        
        // Test 5: Castling Moves
        std::cout << "\n=== Test 5: Castling Moves ===" << std::endl;
        board.set_position("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1");
        std::cout << "Position with castling rights:" << std::endl;
        
        legal_moves = MoveGenerator::generate_legal_moves(board);
        std::cout << "\nLegal moves (should include castling): " << legal_moves.size() << std::endl;
        
        // Look for castling moves
        int castling_count = 0;
        for (const Move& move : legal_moves) {
            if (move.is_castling) {
                std::cout << "Castling move found: " << move.to_algebraic() << std::endl;
                castling_count++;
            }
        }
        std::cout << "Total castling moves: " << castling_count << std::endl;
        
        // Test 6: Pawn Promotion
        std::cout << "\n=== Test 6: Pawn Promotion ===" << std::endl;
        board.set_position("8/P7/8/8/8/8/7p/8 w - - 0 1");
        std::cout << "Position with promotion opportunity:" << std::endl;
        
        legal_moves = MoveGenerator::generate_legal_moves(board);
        std::cout << "\nLegal moves (should include promotions): " << legal_moves.size() << std::endl;
        
        // Look for promotion moves
        int promotion_count = 0;
        for (const Move& move : legal_moves) {
            if (move.promotion_piece != '.') {
                std::cout << "Promotion move found: " << move.to_algebraic() << std::endl;
                promotion_count++;
            }
        }
        std::cout << "Total promotion moves: " << promotion_count << std::endl;
        
        // Test 7: Check Position
        std::cout << "\n=== Test 7: King in Check ===" << std::endl;
        board.set_position("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3");
        
        white_in_check = MoveGenerator::is_in_check(board, 'w');
        black_in_check = MoveGenerator::is_in_check(board, 'b');
        std::cout << "\nWhite in check: " << (white_in_check ? "Yes" : "No") << std::endl;
        std::cout << "Black in check: " << (black_in_check ? "Yes" : "No") << std::endl;
        
        legal_moves = MoveGenerator::generate_legal_moves(board);
        std::cout << "Legal moves for white (in check): " << legal_moves.size() << std::endl;
        
        std::cout << "\nAll legal moves for white:" << std::endl;
        for (size_t i = 0; i < legal_moves.size(); i++) {
            std::cout << i + 1 << ". " << legal_moves[i].to_algebraic() << std::endl;
        }
        
        // Test 8: Engine Integration
        std::cout << "\n=== Test 8: Engine Integration ===" << std::endl;
        std::cout << "Testing engine with starting position..." << std::endl;
        engine.set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        std::string best_move = engine.search_best_move(3);
        std::cout << "Best move from engine: " << best_move << std::endl;
        
        // Test 9: Move Validation
        std::cout << "\n=== Test 9: Move Validation ===" << std::endl;
        board.set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        std::cout << "Testing move validation from starting position:" << std::endl;
        
        // Test valid moves
        Move valid_move1(6, 4, 4, 4, 'P', '.', '.', false, false); // e2-e4
        Move valid_move2(7, 1, 5, 2, 'N', '.', '.', false, false); // Nb1-c3
        
        std::cout << "Valid move e2-e4: " << (board.is_valid_move(valid_move1) ? "Yes" : "No") << std::endl;
        std::cout << "Valid move Nb1-c3: " << (board.is_valid_move(valid_move2) ? "Yes" : "No") << std::endl;
        
        // Test invalid moves
        Move invalid_move1(6, 4, 3, 4, 'P', '.', '.', false, false); // e2-e5 (too far)
        Move invalid_move2(7, 0, 5, 0, 'R', '.', '.', false, false); // Ra1-a6 (blocked)
        
        std::cout << "Invalid move e2-e5: " << (board.is_valid_move(invalid_move1) ? "Yes" : "No") << std::endl;
        std::cout << "Invalid move Ra1-a6: " << (board.is_valid_move(invalid_move2) ? "Yes" : "No") << std::endl;
        
        // Test 10: Make/Undo Move Functionality
        std::cout << "\n=== Test 10: Make/Undo Move Functionality ===" << std::endl;
        board.set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        
        // Make a move and capture state
        Board::BoardState state;
        Move test_move(6, 4, 4, 4, 'P', '.', '.', false, false); // e2-e4
        
        bool move_success = board.make_move(test_move, state);
        std::cout << "\nMade move e2-e4: " << (move_success ? "Success" : "Failed") << std::endl;
        std::cout << "Position after e2-e4: " << std::endl;
        board.print();
        
        // Undo the move
        board.undo_move(test_move, state);
        std::cout << "\nAfter undo: " << std::endl;
        board.print();
        std::cout << "Position restored correctly: " << (board.to_fen() == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" ? "Yes" : "No") << std::endl;
        
        // Test 11: Make/Undo with Capture
        std::cout << "\n=== Test 11: Make/Undo with Capture ===" << std::endl;
        board.set_position("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
        std::string original_fen = board.to_fen();
        
        // Make a capture move
        Move capture_move(4, 4, 3, 3, 'P', 'p', '.', false, false); // exd5
        Board::BoardState capture_state;
        
        move_success = board.make_move(capture_move, capture_state);
        std::cout << "\nMade capture exd5: " << (move_success ? "Success" : "Failed") << std::endl;
        std::cout << "Position after capture: " << std::endl;
        board.print();
        
        // Undo the capture
        board.undo_move(capture_move, capture_state);
        std::cout << "\nAfter undo capture: " << std::endl;
        board.print();
        std::cout << "Capture undone correctly: " << (board.to_fen() == original_fen ? "Yes" : "No") << std::endl;
        
        // Test 12: Make/Undo Castling
        std::cout << "\n=== Test 12: Make/Undo Castling ===" << std::endl;
        board.set_position("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1");
        original_fen = board.to_fen();
        
        // Make kingside castling
        Move castling_move(7, 4, 7, 6, 'K', '.', '.', true, false); // O-O
        Board::BoardState castling_state;
        
        move_success = board.make_move(castling_move, castling_state);
        std::cout << "\nMade kingside castling: " << (move_success ? "Success" : "Failed") << std::endl;
        std::cout << "Position after castling: " << std::endl;
        board.print();
        
        // Undo castling
        board.undo_move(castling_move, castling_state);
        std::cout << "\nAfter undo castling: " << std::endl;
        board.print();
        std::cout << "Castling undone correctly: " << (board.to_fen() == original_fen ? "Yes" : "No") << std::endl;
        
        // Test 13: Make/Undo En Passant
        std::cout << "\n=== Test 13: Make/Undo En Passant ===" << std::endl;
        board.set_position("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
        original_fen = board.to_fen();

        legal_moves = MoveGenerator::generate_legal_moves(board);
        std::cout << "\nLegal moves (should include en passant): " << legal_moves.size() << std::endl;

        // Look for en passant moves
        en_passant_count = 0;
        Move en_passant_move; // exf6

        for (const Move& move : legal_moves) {
            if (move.is_en_passant) {
                en_passant_move = move;
                std::cout << "En passant move found: " << move.to_algebraic() << std::endl;
                en_passant_count++;
            }
        }

        // Make en passant capture
        Board::BoardState en_passant_state;
        
        move_success = board.make_move(en_passant_move, en_passant_state);
        std::cout << "\nMade en passant exf6: " << (move_success ? "Success" : "Failed") << std::endl;
        std::cout << "Position after en passant: " << std::endl;
        board.print();
        
        // Undo en passant
        board.undo_move(en_passant_move, en_passant_state);
        std::cout << "\nAfter undo en passant: " << std::endl;
        board.print();
        std::cout << "En passant undone correctly: " << (board.to_fen() == original_fen ? "Yes" : "No") << std::endl;
        
        // Test 14: Make/Undo Promotion
        std::cout << "\n=== Test 14: Make/Undo Promotion ===" << std::endl;
        board.set_position("8/P7/8/8/8/8/7p/8 w - - 0 1");
        original_fen = board.to_fen();
        std::cout << "Position with promotion opportunity: " << original_fen << std::endl;
        
        // Make promotion move
        Move promotion_move(1, 0, 0, 0, 'P', '.', 'Q', false, false); // a7-a8=Q
        Board::BoardState promotion_state;
        
        move_success = board.make_move(promotion_move, promotion_state);
        std::cout << "\nMade promotion a7-a8=Q: " << (move_success ? "Success" : "Failed") << std::endl;
        std::cout << "Position after promotion: " << std::endl;
        board.print();
        
        // Undo promotion
        board.undo_move(promotion_move, promotion_state);
        std::cout << "\nAfter undo promotion: " << std::endl;
        board.print();
        std::cout << "Promotion undone correctly: " << (board.to_fen() == original_fen ? "Yes" : "No") << std::endl;
        
        // Test 15: Multiple Move Sequence
        std::cout << "\n=== Test 15: Multiple Move Sequence ===" << std::endl;
        board.set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        std::cout << "Testing sequence: 1.e4 e5 2.Nf3 Nc6" << std::endl;
        
        std::vector<Move> move_sequence = {
            Move(6, 4, 4, 4, 'P', '.', '.', false, false), // e2-e4
            Move(1, 4, 3, 4, 'p', '.', '.', false, false), // e7-e5
            Move(7, 6, 5, 5, 'N', '.', '.', false, false), // Ng1-f3
            Move(0, 1, 2, 2, 'n', '.', '.', false, false)  // Nb8-c6
        };
        
        std::vector<Board::BoardState> states;
        states.resize(move_sequence.size());
        
        // Make all moves
        for (size_t i = 0; i < move_sequence.size(); i++) {
            bool success = board.make_move(move_sequence[i], states[i]);
            board.print();
            std::cout << "Move " << (i + 1) << " (" << move_sequence[i].to_algebraic() << "): " << (success ? "Success" : "Failed") << std::endl;
        }
        
        std::cout << "\nFinal position: " << std::endl;
        board.print();
        
        // Undo all moves in reverse order
        std::cout << "\nUndoing moves in reverse order:" << std::endl;
        for (int i = move_sequence.size() - 1; i >= 0; i--) {
            board.undo_move(move_sequence[i], states[i]);
            board.print();
            std::cout << "Undid move " << (i + 1) << " (" << move_sequence[i].to_algebraic() << ")" << std::endl;
        }
        
        std::cout << "\nPosition after undoing all moves: " << std::endl;
        board.print();
        std::cout << "Returned to starting position: " << (board.to_fen() == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" ? "Yes" : "No") << std::endl;
        
        // Test 16: Invalid FEN handling
        std::cout << "\n=== Test 16: Invalid FEN Handling ===" << std::endl;
        engine.set_position("invalid_fen");
        engine.set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP"); // Missing parts
        engine.set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x KQkq - 0 1"); // Invalid color
        
        std::cout << "\n=== All move generation and validation tests completed successfully ===" << std::endl;
        
        // // Test 17: Search and Evaluation Tests
        // std::cout << "\n=== Test 17: Search and Evaluation Tests ===" << std::endl;
        //
        // // Test basic evaluation
        // std::cout << "\n--- Testing Basic Evaluation ---" << std::endl;
        // board.set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        // int starting_eval = Evaluation::evaluate_position(board);
        // std::cout << "Starting position evaluation: " << starting_eval << " centipawns" << std::endl;
        //
        // // Test material advantage
        // board.set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKB1R w KQkq - 0 1"); // Missing white knight
        // int material_disadvantage = Evaluation::evaluate_position(board);
        // std::cout << "Position missing white knight: " << material_disadvantage << " centipawns" << std::endl;
        //
        // board.set_position("rnbqkb1r/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); // Missing black knight
        // int material_advantage = Evaluation::evaluate_position(board);
        // std::cout << "Position missing black knight: " << material_advantage << " centipawns" << std::endl;
        //
        // // Test search functionality
        // std::cout << "\n--- Testing Search Functionality ---" << std::endl;
        // board.set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        //
        // std::cout << "\nSearching at depth 1:" << std::endl;
        // Move best_move_d1 = Search::find_best_move(board, 1);
        // std::cout << "Best move at depth 1: " << best_move_d1.to_algebraic() << std::endl;
        //
        // std::cout << "\nSearching at depth 2:" << std::endl;
        // Move best_move_d2 = Search::find_best_move(board, 2);
        // std::cout << "Best move at depth 2: " << best_move_d2.to_algebraic() << std::endl;
        //
        // std::cout << "\nSearching at depth 3:" << std::endl;
        // Move best_move_d3 = Search::find_best_move(board, 3);
        // std::cout << "Best move at depth 3: " << best_move_d3.to_algebraic() << std::endl;
        //
        // // Test tactical position
        // std::cout << "\n--- Testing Tactical Position ---" << std::endl;
        // board.set_position("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 1"); // Italian Game
        // std::cout << "Italian Game position:" << std::endl;
        // board.print();
        //
        // int tactical_eval = Evaluation::evaluate_position(board);
        // std::cout << "Tactical position evaluation: " << tactical_eval << " centipawns" << std::endl;
        //
        // Move tactical_move = Search::find_best_move(board, 3);
        // std::cout << "Best tactical move: " << tactical_move.to_algebraic() << std::endl;
        //
        // // Test endgame position
        // std::cout << "\n--- Testing Endgame Position ---" << std::endl;
        // board.set_position("8/8/8/8/8/3k4/3P4/3K4 w - - 0 1"); // King and pawn endgame
        // std::cout << "King and pawn endgame:" << std::endl;
        // board.print();
        //
        // int endgame_eval = Evaluation::evaluate_position(board);
        // std::cout << "Endgame evaluation: " << endgame_eval << " centipawns" << std::endl;
        // std::cout << "Is endgame: " << (Evaluation::is_endgame(board) ? "Yes" : "No") << std::endl;
        //
        // Move endgame_move = Search::find_best_move(board, 4);
        // std::cout << "Best endgame move: " << endgame_move.to_algebraic() << std::endl;
        //
        // // Test Engine integration
        // std::cout << "\n--- Testing Engine Integration ---" << std::endl;
        // engine.set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        //
        // std::string engine_move_d2 = engine.search_best_move(2);
        // std::cout << "Engine best move at depth 2: " << engine_move_d2 << std::endl;
        //
        // std::string engine_move_d3 = engine.search_best_move(3);
        // std::cout << "Engine best move at depth 3: " << engine_move_d3 << std::endl;
        //
        // // Test piece values
        // std::cout << "\n--- Testing Piece Values ---" << std::endl;
        // std::cout << "Pawn value: " << Evaluation::get_piece_value('P') << std::endl;
        // std::cout << "Knight value: " << Evaluation::get_piece_value('N') << std::endl;
        // std::cout << "Bishop value: " << Evaluation::get_piece_value('B') << std::endl;
        // std::cout << "Rook value: " << Evaluation::get_piece_value('R') << std::endl;
        // std::cout << "Queen value: " << Evaluation::get_piece_value('Q') << std::endl;
        // std::cout << "King value: " << Evaluation::get_piece_value('K') << std::endl;
        //
        // // Test checkmate detection
        // std::cout << "\n--- Testing Checkmate Detection ---" << std::endl;
        // board.set_position("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3"); // Scholar's mate
        // std::cout << "Scholar's mate position:" << std::endl;
        // board.print();
        //
        // legal_moves = MoveGenerator::generate_legal_moves(board);
        // std::cout << "Number of legal moves: " << legal_moves.size() << std::endl;
        //
        // if (legal_moves.empty()) {
        //     bool in_check = MoveGenerator::is_in_check(board, board.get_active_color());
        //     std::cout << "Position is: " << (in_check ? "Checkmate" : "Stalemate") << std::endl;
        // }
        //
        // int mate_eval = Evaluation::evaluate_position(board);
        // std::cout << "Mate position evaluation: " << mate_eval << " centipawns" << std::endl;
        //
        // std::cout << "\n=== Search and Evaluation tests completed successfully ===" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "Error during testing: " << e.what() << std::endl;
        return 1;
    }
    
    // Restore cout
    std::cout.rdbuf(original_cout);
    results.close();
    std::cout << "Tests completed. Results written to results.txt" << std::endl;
    
    return 0;
}