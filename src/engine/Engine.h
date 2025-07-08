#ifndef ENGINE_H
#define ENGINE_H

#include <string>
#include "../board/Board.h"

class Engine {
public:
    Engine();
    ~Engine();
    
    // Top-level position management
    void set_position(const std::string& fen);                                       // Sets the board state using FEN notation
    const std::string get_current_position() const { return current_position; }      // Gets current position as FEN
    
    // AI logic
    std::string search_best_move(int depth);                                         // Finds the best move using search algorithm
    
    // Board access (for testing and debugging)
    const Board& get_board() const { return board; }
    
private:
    std::string current_position;
    Board board;  // Chess board representation
};

#endif // ENGINE_H