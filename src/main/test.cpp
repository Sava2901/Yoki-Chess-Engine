#include "../engine/Engine.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "../engine/Engine.h"
#include "../board/Board.h"
#include "../board/Move.h"
#include "../board/MoveGenerator.h"

class TeeStreambuf : public std::streambuf {
public:
    TeeStreambuf(std::streambuf* sb1, std::streambuf* sb2) : sb1_(sb1), sb2_(sb2) {}
    
protected:
    virtual int overflow(int c) {
        if (c == EOF) {
            return !EOF;
        } else {
            int const r1 = sb1_->sputc(c);
            int const r2 = sb2_->sputc(c);
            return r1 == EOF || r2 == EOF ? EOF : c;
        }
    }
    
    virtual int sync() {
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
        
        // Test 1: Move Generation - Starting Position
        std::cout << "\n=== Test 1: Move Generation - Starting Position ===" << std::endl;
        board.set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        std::cout << "Board position:" << std::endl;
        board.print();
        
        MoveList pseudo_moves = MoveGenerator::generate_pseudo_legal_moves(board);
        MoveList legal_moves = MoveGenerator::generate_legal_moves(board);
        
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
        board.print();
        
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
        board.print();
        
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
        board.print();
        
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
        board.print();
        
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
        std::cout << "Position with white king in check:" << std::endl;
        board.print();
        
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
        
        // Test 9: Invalid FEN handling
        std::cout << "\n=== Test 9: Invalid FEN Handling ===" << std::endl;
        engine.set_position("invalid_fen");
        engine.set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP"); // Missing parts
        engine.set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x KQkq - 0 1"); // Invalid color
        
        std::cout << "\n=== All move generation tests completed successfully ===" << std::endl;
        
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