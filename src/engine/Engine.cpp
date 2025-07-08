#include "Engine.h"
#include <iostream>

Engine::Engine() {
    // Initialize with starting position
    current_position = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    
    // Board initializes itself with starting position
    std::cout << "Engine initialized with starting position" << std::endl;
}

Engine::~Engine() {
    std::cout << "Engine destroyed" << std::endl;
}

void Engine::set_position(const std::string& fen) {
    board.set_position(fen);
    current_position = board.to_fen();
    std::cout << "Position set to: " << current_position << std::endl;
}

std::string Engine::search_best_move(int depth) {
    std::cout << "Searching for best move at depth " << depth << std::endl;
    std::cout << "Current position: " << current_position << std::endl;
    
    // Simple placeholder implementation - returns a common opening move
    // This will be replaced with actual search algorithm later
    std::string best_move = "e2e4";
    
    std::cout << "Best move found: " << best_move << std::endl;
    return best_move;
}