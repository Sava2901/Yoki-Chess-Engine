#include "Engine.h"
#include "Search.h"
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


    return "e2e4"; // Placeholder for the best move
    // // Use the search algorithm to find the best move
    // Move best_move = Search::find_best_move(board, depth);
    //
    // if (best_move.piece == '.') {
    //     std::cout << "No valid move found!" << std::endl;
    //     return "";
    // }
    //
    // std::string move_string = best_move.to_algebraic();
    // std::cout << "Best move found: " << move_string << std::endl;
    // return move_string;
}