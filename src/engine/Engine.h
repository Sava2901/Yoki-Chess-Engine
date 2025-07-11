#ifndef ENGINE_H
#define ENGINE_H

#include <string>
#include <string_view>
#include "../board/Board.h"

class Engine {
public:
    Engine();
    ~Engine();
    
    // Top-level position management
    void set_position(std::string_view fen);                                       // Sets the board state using FEN notation
    const std::string get_current_position() const { return current_position; }      // Gets current position as FEN
    
    // AI logic
    std::string get_best_move(int depth);                                         // Finds the best move using search algorithm
    
    // Board access (for testing and debugging)
    const Board& get_board() const { return board; }
    
private:
    std::string current_position;
    Board board;  // Chess board representation
};

#endif // ENGINE_H