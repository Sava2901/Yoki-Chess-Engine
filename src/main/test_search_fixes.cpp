#include <iostream>
#include <vector>
#include <chrono>
#include "../board/Board.h"
#include "../engine/Search.h"
#include "../engine/Evaluation.h"
#include "../board/MoveGenerator.h"

int main() {
    std::cout << "Testing Search Engine PV Fixes..." << std::endl;
    
    // Initialize board
    Board board;
    board.set_starting_position();
    
    // Initialize search engine
    Search search;
    
    std::cout << "Board initialized successfully." << std::endl;
    
    // Test searches at different depths
    for (int depth = 1; depth <= 8; depth++) {
        std::cout << "\n--- Testing depth " << depth << " ---" << std::endl;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        try {
            SearchResult result = search.search(board, std::chrono::milliseconds(5000), depth);
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            std::cout << "Search completed successfully!" << std::endl;
            std::cout << "Best score: " << result.score << std::endl;
            std::cout << "Nodes searched: " << result.stats.nodes_searched << std::endl;
            std::cout << "Time: " << duration.count() << "ms" << std::endl;
            
            // Check principal variation validity
            std::cout << "Principal variation length: " << result.principal_variation.size() << std::endl;
            
            // Display and validate the principal variation
            if (!result.principal_variation.empty()) {
                std::cout << "Principal variation (score: " << result.score << "): ";
                
                Board temp_board = board;
                Board::Color expected_color = board.get_active_color();
                
                bool pv_valid = true;
                for (size_t i = 0; i < result.principal_variation.size(); i++) {
                    const Move& move = result.principal_variation[i];
                    Board::Color move_color = Board::char_to_color(move.piece);
                    
                    std::cout << move.piece << char('a' + move.from_file) << (move.from_rank + 1) 
                              << char('a' + move.to_file) << (move.to_rank + 1) << " ";
                    
                    if (move_color != expected_color) {
                        std::cout << "\n*** ERROR: Move " << i << " has wrong color! Expected " 
                                  << (expected_color == Board::WHITE ? "WHITE" : "BLACK") 
                                  << " but got " << (move_color == Board::WHITE ? "WHITE" : "BLACK") << std::endl;
                        pv_valid = false;
                        break;
                    }
                    
                    // Make the move and switch expected color
                    if (temp_board.is_move_legal(move)) {
                        temp_board.make_move(move);
                        expected_color = (expected_color == Board::WHITE) ? Board::BLACK : Board::WHITE;
                    } else {
                        std::cout << "\n*** ERROR: Move " << i << " is illegal!" << std::endl;
                        pv_valid = false;
                        break;
                    }
                }
                
                std::cout << std::endl;
                if (pv_valid) {
                    std::cout << "✓ Principal variation is valid with proper color alternation" << std::endl;
                } else {
                    std::cout << "✗ Principal variation has errors!" << std::endl;
                }
            } else {
                std::cout << "No principal variation found" << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cout << "*** CRASH: " << e.what() << std::endl;
            return 1;
        } catch (...) {
            std::cout << "*** UNKNOWN CRASH occurred!" << std::endl;
            return 1;
        }
    }
    
    std::cout << "\n=== All tests completed successfully! ===" << std::endl;
    return 0;
}