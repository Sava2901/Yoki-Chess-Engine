#include "Engine.h"
#include "Search.h"

Engine::Engine() : board(), current_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") {}

Engine::~Engine() = default;

void Engine::set_position(std::string_view fen) {
    board.set_position(fen);
    current_position = fen;
}

std::string Engine::get_best_move(int depth) {
    return "e2e4"; // Placeholder for the best move

    // std::cout << "Searching for best move at depth " << depth << std::endl;
    // std::cout << "Current position: " << current_position << std::endl;
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