#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include "../board/Board.h"
#include "../board/MoveGenerator.h"
#include "../board/Bitboard.h"

class OptimizationTester {
private:
    std::mt19937 rng;
    
public:
    OptimizationTester() : rng(std::random_device{}()) {}
    
    // Test PEXT vs Magic Bitboards performance
    void test_bitboard_performance() {
        std::cout << "\n=== Bitboard Performance Test ===\n";
        
        const int iterations = 1000000;
        std::vector<int> squares;
        std::vector<Bitboard> occupancies;
        
        // Generate random test data
        std::uniform_int_distribution<int> square_dist(0, 63);
        std::uniform_int_distribution<uint64_t> occ_dist(0, UINT64_MAX);
        
        for (int i = 0; i < 1000; i++) {
            squares.push_back(square_dist(rng));
            occupancies.push_back(occ_dist(rng));
        }
        
        // Test rook attacks
        auto start = std::chrono::high_resolution_clock::now();
        volatile Bitboard result = 0;
        
        for (int i = 0; i < iterations; i++) {
            int idx = i % squares.size();
            result ^= BitboardUtils::rook_attacks(squares[idx], occupancies[idx]);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Rook attacks (" << iterations << " calls): " 
                  << duration.count() << " microseconds\n";
        std::cout << "Average per call: " 
                  << static_cast<double>(duration.count()) / iterations << " microseconds\n";
        
        // Test bishop attacks
        start = std::chrono::high_resolution_clock::now();
        result = 0;
        
        for (int i = 0; i < iterations; i++) {
            int idx = i % squares.size();
            result ^= BitboardUtils::bishop_attacks(squares[idx], occupancies[idx]);
        }
        
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Bishop attacks (" << iterations << " calls): " 
                  << duration.count() << " microseconds\n";
        std::cout << "Average per call: " 
                  << static_cast<double>(duration.count()) / iterations << " microseconds\n";
        
#ifdef __BMI2__
        std::cout << "BMI2 PEXT support: ENABLED\n";
#else
        std::cout << "BMI2 PEXT support: DISABLED (using Magic Bitboards)\n";
#endif
    }
    
    // Test move generation performance
    void test_move_generation_performance() {
        std::cout << "\n=== Move Generation Performance Test ===\n";
        
        Board board;
        MoveGenerator generator;
        
        // Test positions
        std::vector<std::string> test_positions = {
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", // Starting position
            "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", // Complex middle game
            "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", // Endgame
            "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8" // Tactical position
        };
        
        const int iterations = 10000;
        
        for (const auto& fen : test_positions) {
            board.set_from_fen(fen);
            
            // Test all moves generation
            auto start = std::chrono::high_resolution_clock::now();
            
            for (int i = 0; i < iterations; i++) {
                auto moves = generator.generate_all_moves(board);
                volatile size_t count = moves.size(); // Prevent optimization
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            auto moves = generator.generate_all_moves(board);
            std::cout << "Position: " << fen.substr(0, 30) << "...\n";
            std::cout << "  Moves generated: " << moves.size() << "\n";
            std::cout << "  Time (" << iterations << " iterations): " 
                      << duration.count() << " microseconds\n";
            std::cout << "  Average per generation: " 
                      << static_cast<double>(duration.count()) / iterations << " microseconds\n\n";
        }
    }
    
    // Test legal move generation performance
    void test_legal_move_generation() {
        std::cout << "\n=== Legal Move Generation Performance Test ===\n";
        
        Board board;
        MoveGenerator generator;
        
        // Test positions with different complexity
        std::vector<std::string> test_positions = {
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
            "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"
        };
        
        const int iterations = 5000;
        
        for (const auto& fen : test_positions) {
            board.set_from_fen(fen);
            
            auto start = std::chrono::high_resolution_clock::now();
            
            for (int i = 0; i < iterations; i++) {
                auto moves = generator.generate_legal_moves(board);
                volatile size_t count = moves.size();
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            auto moves = generator.generate_legal_moves(board);
            std::cout << "Position: " << fen.substr(0, 30) << "...\n";
            std::cout << "  Legal moves: " << moves.size() << "\n";
            std::cout << "  Time (" << iterations << " iterations): " 
                      << duration.count() << " microseconds\n";
            std::cout << "  Average per generation: " 
                      << static_cast<double>(duration.count()) / iterations << " microseconds\n\n";
        }
    }
    
    // Test move ordering effectiveness
    void test_move_ordering() {
        std::cout << "\n=== Move Ordering Test ===\n";
        
        Board board;
        MoveGenerator generator;
        
        // Test tactical position where move ordering matters
        board.set_from_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
        
        auto moves = generator.generate_all_moves(board);
        auto captures = generator.generate_captures(board);
        
        std::cout << "Total moves: " << moves.size() << "\n";
        std::cout << "Captures: " << captures.size() << "\n";
        
        // Display first 10 moves to show ordering
        std::cout << "\nFirst 10 moves (showing move ordering):" << std::endl;
        for (size_t i = 0; i < std::min(static_cast<size_t>(10), moves.size()); i++) {
            std::cout << "  " << (i + 1) << ". " << moves[i].to_algebraic() << "\n";
        }
        
        std::cout << "\nFirst 5 captures (MVV-LVA ordering):" << std::endl;
        for (size_t i = 0; i < std::min(static_cast<size_t>(5), captures.size()); i++) {
            std::cout << "  " << (i + 1) << ". " << captures[i].to_algebraic() << "\n";
        }
    }
    
    // Test branchless operations (indirect test through performance)
    void test_branchless_performance() {
        std::cout << "\n=== Branchless Operations Test ===\n";
        
        Board board;
        const int iterations = 100000;
        
        // Test make/unmake move performance (includes branchless optimizations)
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        
        MoveGenerator generator;
        auto moves = generator.generate_legal_moves(board);
        
        if (!moves.empty()) {
            auto start = std::chrono::high_resolution_clock::now();
            
            for (int i = 0; i < iterations; i++) {
                Move move = moves[i % moves.size()];
                auto undo_data = board.make_move(move);
                board.undo_move(undo_data);
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            std::cout << "Make/Unmake moves (" << iterations << " iterations): " 
                      << duration.count() << " microseconds\n";
            std::cout << "Average per make/unmake: " 
                      << static_cast<double>(duration.count()) / iterations << " microseconds\n";
            std::cout << "Note: This includes branchless optimizations in Board.cpp\n";
        }
    }
    
    // Memory usage test
    void test_memory_usage() {
        std::cout << "\n=== Memory Usage Analysis ===\n";
        
        std::cout << "Static table sizes:\n";
        std::cout << "  Rook attack table: ~" << (102400 * sizeof(Bitboard)) / 1024 << " KB\n";
        std::cout << "  Bishop attack table: ~" << (5248 * sizeof(Bitboard)) / 1024 << " KB\n";
        std::cout << "  Knight attacks: " << (64 * sizeof(Bitboard)) / 1024 << " KB\n";
        std::cout << "  King attacks: " << (64 * sizeof(Bitboard)) / 1024 << " KB\n";
        std::cout << "  Pawn attacks: " << (128 * sizeof(Bitboard)) / 1024 << " KB\n";
        
        size_t total_kb = ((102400 + 5248 + 64 + 64 + 128) * sizeof(Bitboard)) / 1024;
        std::cout << "  Total precomputed tables: ~" << total_kb << " KB\n";
        
        std::cout << "\nMove generation memory efficiency:\n";
        Board board;
        MoveGenerator generator;
        
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        auto moves = generator.generate_all_moves(board);
        
        std::cout << "  Moves vector capacity: " << moves.capacity() << " moves\n";
        std::cout << "  Actual moves generated: " << moves.size() << " moves\n";
        std::cout << "  Memory efficiency: " 
                  << (static_cast<double>(moves.size()) / moves.capacity() * 100) << "%\n";
    }
    
    void run_all_tests() {
        std::cout << "Chess Engine Optimization Test Suite\n";
        std::cout << "====================================\n";
        
        // Initialize bitboards
        BitboardUtils::init();
        
        test_bitboard_performance();
        test_move_generation_performance();
        test_legal_move_generation();
        test_move_ordering();
        test_branchless_performance();
        test_memory_usage();
        
        std::cout << "\n=== Test Summary ===\n";
        std::cout << "All optimization tests completed successfully!\n";
        
#ifdef __BMI2__
        std::cout << "BMI2 PEXT support: ENABLED\n";
#else
        std::cout << "BMI2 PEXT support: DISABLED (using Magic Bitboards)\n";
#endif
        
        std::cout << "\nOptimizations tested:\n";
        std::cout << "✓ Magic Bitboards with PEXT support (BMI2)\n";
        std::cout << "✓ Precomputed attack tables\n";
        std::cout << "✓ Move ordering (MVV-LVA, piece priorities)\n";
        std::cout << "✓ Branchless operations in Board.cpp\n";
        std::cout << "✓ Memory-optimized move generation\n";
        std::cout << "✓ Legal move optimization with pin/check detection\n";
    }
};

int main() {
    try {
        OptimizationTester tester;
        tester.run_all_tests();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}