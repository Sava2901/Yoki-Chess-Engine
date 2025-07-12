#include "../board/Bitboard.h"
#include "../board/Board.h"
#include "../board/MoveGenerator.h"
#include <iostream>
#include <chrono>
#include <iomanip>

void test_bitboard_utils() {
    std::cout << "=== Testing Bitboard Utils ===\n";
    
    // Initialize bitboards
    BitboardUtils::init();
    
    // Test basic operations
    Bitboard bb = 0;
    BitboardUtils::set_bit(bb, 0);  // a1
    BitboardUtils::set_bit(bb, 7);  // h1
    BitboardUtils::set_bit(bb, 56); // a8
    BitboardUtils::set_bit(bb, 63); // h8
    
    std::cout << "Corner squares bitboard:\n";
    BitboardUtils::print_bitboard(bb);
    
    std::cout << "Population count: " << BitboardUtils::popcount(bb) << "\n";
    
    // Test knight attacks
    std::cout << "\nKnight attacks from e4 (square 28):\n";
    Bitboard knight_attacks = BitboardUtils::knight_attacks(28);
    BitboardUtils::print_bitboard(knight_attacks);
    
    // Test rook attacks
    std::cout << "\nRook attacks from e4 with some blockers:\n";
    Bitboard blockers = 0;
    BitboardUtils::set_bit(blockers, 20); // e3
    BitboardUtils::set_bit(blockers, 30); // g4
    Bitboard rook_attacks = BitboardUtils::rook_attacks(28, blockers);
    BitboardUtils::print_bitboard(rook_attacks);
}

void test_bitboard_board() {
    std::cout << "\n=== Testing Bitboard Board ===\n";
    
    Board board;
    board.set_starting_position();
    
    std::cout << "Starting position:\n";
    board.print();
    
    // Test piece access
    std::cout << "\nPiece at e2: " << board.get_piece(1, 4) << "\n";
    std::cout << "Piece at e7: " << board.get_piece(6, 4) << "\n";
    
    // Test bitboard access
    std::cout << "\nWhite pawns bitboard:\n";
    Bitboard white_pawns = board.get_piece_bitboard(Board::PAWN, Board::WHITE);
    BitboardUtils::print_bitboard(white_pawns);
    
    std::cout << "\nBlack pieces bitboard:\n";
    Bitboard black_pieces = board.get_color_bitboard(Board::BLACK);
    BitboardUtils::print_bitboard(black_pieces);
}

void test_move_generation() {
    std::cout << "\n=== Testing Move Generation ===\n";
    
    Board board;
    MoveGenerator generator;
    
    board.set_starting_position();
    
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<Move> moves = generator.generate_all_moves(board);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Generated " << moves.size() << " moves from starting position\n";
    std::cout << "Time taken: " << duration.count() << " microseconds\n";
    
    std::cout << "\nFirst 10 moves:\n";
    for (size_t i = 0; i < std::min(moves.size(), static_cast<size_t>(10)); i++) {
        const Move& move = moves[i];
        char from_file = 'a' + move.from_file;
        char to_file = 'a' + move.to_file;
        std::cout << move.piece << ": " << from_file << (move.from_rank + 1) 
                  << " -> " << to_file << (move.to_rank + 1);
        if (move.promotion_piece != '.') {
            std::cout << "=" << move.promotion_piece;
        }
        if (move.is_castling) {
            std::cout << " (castling)";
        }
        if (move.is_en_passant) {
            std::cout << " (en passant)";
        }
        std::cout << "\n";
    }
}

void test_legal_moves() {
    std::cout << "\n=== Testing Legal Move Generation ===\n";
    
    Board board;
    MoveGenerator generator;
    
    board.set_starting_position();
    
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<Move> legal_moves = generator.generate_legal_moves(board);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Generated " << legal_moves.size() << " legal moves from starting position\n";
    std::cout << "Time taken: " << duration.count() << " microseconds\n";
    
    // Test a position with check
    board.set_from_fen("rnbqkb1r/pppp1ppp/5n2/4p3/2B1P3/8/PPPP1PPP/RNBQK1NR w KQkq - 2 3");
    std::cout << "\nPosition after 1.e4 e5 2.Bc4 Nf6:\n";
    board.print();
    
    legal_moves = generator.generate_legal_moves(board);
    std::cout << "Legal moves: " << legal_moves.size() << "\n";
}

void performance_test() {
    std::cout << "\n=== Performance Test ===\n";
    
    Board board;
    MoveGenerator generator;
    
    board.set_starting_position();
    
    const int iterations = 10000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    long long total_moves = 0;
    for (int i = 0; i < iterations; i++) {
        std::vector<Move> moves = generator.generate_all_moves(board);
        total_moves += moves.size();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Generated " << total_moves << " total moves in " << iterations << " iterations\n";
    std::cout << "Total time: " << duration.count() << " microseconds\n";
    std::cout << "Average time per move generation: " 
              << std::fixed << std::setprecision(2) 
              << (double)duration.count() / iterations << " microseconds\n";
    std::cout << "Moves per second: " 
              << std::fixed << std::setprecision(0)
              << (double)total_moves / duration.count() * 1000000 << "\n";
}

void test_attack_detection() {
    std::cout << "\n=== Testing Attack Detection ===\n";
    
    Board board;
    MoveGenerator generator;
    
    // Test position where white king is in check
    board.set_from_fen("rnbqkbnr/pppp1ppp/8/4p3/6P1/5P2/PPPPP2P/RNBQKBNR b KQkq - 0 2");
    
    std::cout << "Test position:\n";
    board.print();
    
    bool white_in_check = generator.is_in_check(board, Board::WHITE);
    bool black_in_check = generator.is_in_check(board, Board::BLACK);
    
    std::cout << "White in check: " << (white_in_check ? "Yes" : "No") << "\n";
    std::cout << "Black in check: " << (black_in_check ? "Yes" : "No") << "\n";
    
    // Test attacked squares
    std::cout << "\nSquares attacked by white:\n";
    // board.set_starting_position();
    Bitboard white_attacks = generator.get_attacked_squares(board, Board::WHITE);
    BitboardUtils::print_bitboard(white_attacks);
}

int main() {
    std::cout << "Bitboard Chess Engine Test Suite\n";
    std::cout << "================================\n";
    
    try {
        test_bitboard_utils();
        test_bitboard_board();
        test_move_generation();
        test_legal_moves();
        test_attack_detection();
        performance_test();
        
        std::cout << "\n=== All Tests Completed Successfully! ===\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}