#ifndef ENGINE_H
#define ENGINE_H

#include <string>
#include <string_view>
#include "../board/Board.h"

/**
 * @brief Main chess engine class
 * 
 * The Engine class provides the main interface for the chess engine,
 * handling position management and move calculation.
 */
class Engine {
public:
    /**
     * @brief Default constructor
     * 
     * Initializes the engine with the standard starting position.
     */
    Engine();
    
    /**
     * @brief Destructor
     */
    ~Engine();
    
    /**
     * @brief Set the board position from FEN notation
     * 
     * Updates the internal board state using the provided FEN string.
     * 
     * @param fen FEN notation string representing the position
     */
    void set_position(std::string_view fen);
    
    /**
     * @brief Get the current position as FEN string
     * 
     * Returns the current board position in FEN notation.
     * 
     * @return Current position as FEN string
     */
    const std::string get_current_position() const { return current_position; }
    
    /**
     * @brief Find the best move using search algorithm
     * 
     * Analyzes the current position to find the best move using
     * the search algorithm with the specified depth.
     * 
     * @param depth Search depth for move calculation
     * @return Best move in algebraic notation
     */
    std::string get_best_move(int depth);
    
    /**
     * @brief Get read-only access to the internal board
     * 
     * Provides access to the internal board representation
     * for testing and debugging purposes.
     * 
     * @return Const reference to the internal board
     */
    const Board& get_board() const { return board; }
    
private:
    std::string current_position;  ///< Current position in FEN notation
    Board board;                   ///< Internal chess board representation
};

#endif // ENGINE_H