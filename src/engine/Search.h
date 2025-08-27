#ifndef SEARCH_H
#define SEARCH_H

#include "../board/Board.h"
#include "../board/Move.h"
#include "../board/MoveGenerator.h"
#include "Evaluation.h"
#include <vector>
#include <chrono>

/**
 * @brief Chess search engine implementing minimax with alpha-beta pruning
 * 
 * This class provides a complete chess search implementation using the minimax
 * algorithm with alpha-beta pruning for move selection. It supports iterative
 * deepening, time-limited searches, and basic move ordering for improved performance.
 */
class Search {
public:
    /**
     * @brief Statistics collected during search operations
     */
    struct SearchStats {
        int nodes_searched = 0;          ///< Total number of nodes evaluated
        int beta_cutoffs = 0;            ///< Number of beta cutoffs (pruning events)
        std::chrono::milliseconds time_elapsed{0};  ///< Total search time
        
        /**
         * @brief Reset all statistics to zero
         */
        void reset() {
            nodes_searched = 0;
            beta_cutoffs = 0;
            time_elapsed = std::chrono::milliseconds(0);
        }
    };
    
    /**
     * @brief Complete result of a search operation
     */
    struct SearchResult {
        Move best_move;          ///< The best move found
        int score = 0;           ///< Evaluation score of the best move
        int depth = 0;           ///< Actual search depth reached
        SearchStats stats;       ///< Search statistics
        bool is_mate = false;    ///< Whether the result is a forced mate
        int mate_in = 0;         ///< Number of moves until mate (if is_mate is true)
    };
    
private:
    MoveGenerator move_generator;     ///< Move generation engine
    Evaluation* evaluation = nullptr; ///< Position evaluation function
    SearchStats current_stats;        ///< Current search statistics
    
    // Search parameters
    static constexpr int MAX_DEPTH = 10;
    static constexpr int MATE_SCORE = 30000;
    static constexpr int ALPHA_INIT = -31000;
    static constexpr int BETA_INIT = 31000;
    
public:
    /**
     * @brief Constructor - initializes search engine
     */
    Search();
    
    /**
     * @brief Default destructor
     */
    ~Search() = default;
    
    // Main search functions - 4 well-documented functions with clear purposes
    
    /**
     * @brief Find the best move using depth-limited search
     * 
     * Simple search function that returns only the best move found within the specified depth.
     * Uses iterative deepening for better move ordering and early termination on mate.
     * 
     * @param board The current board position to search from
     * @param depth Maximum search depth in plies (half-moves)
     * @return The best move found, or invalid move if no legal moves exist
     */
    Move find_best_move(Board& board, int depth);
    
    /**
     * @brief Find the best move using time-limited search
     * 
     * Simple search function that returns only the best move found within the time limit.
     * Uses iterative deepening and stops when time runs out, returning the best move from
     * the last completed depth.
     * 
     * @param board The current board position to search from
     * @param time_limit Maximum search time allowed
     * @return The best move found, or invalid move if no legal moves exist
     */
    Move find_best_move_timed(Board& board, std::chrono::milliseconds time_limit);
    
    /**
     * @brief Perform depth-limited search with comprehensive statistics
     * 
     * Advanced search function that returns complete search results including the best move,
     * evaluation score, search statistics, and mate detection. Ideal for analysis and
     * debugging purposes.
     * 
     * @param board The current board position to search from
     * @param depth Maximum search depth in plies (half-moves)
     * @return Complete search results with move, score, statistics, and mate information
     */
    SearchResult search_with_stats(Board& board, int depth);
    
    /**
     * @brief Perform time and depth limited search with comprehensive statistics
     * 
     * Most advanced search function that combines time and depth limits with full statistics.
     * Returns complete search results including the best move, evaluation score, actual depth
     * reached, search statistics, and mate detection. Perfect for tournament play and analysis.
     * 
     * @param board The current board position to search from
     * @param depth Maximum search depth in plies (half-moves)
     * @param time_limit Maximum search time allowed
     * @return Complete search results with move, score, statistics, and mate information
     */
    SearchResult search_with_stats_timed(Board& board, int depth, std::chrono::milliseconds time_limit);
    
    // Configuration
    /**
     * @brief Set the evaluation function to use
     * 
     * @param eval Pointer to evaluation function (can be nullptr)
     */
    void set_evaluation(Evaluation* eval) { evaluation = eval; }
    
    /**
     * @brief Get current search statistics
     * 
     * @return Copy of current search statistics
     */
    SearchStats get_stats() const { return current_stats; }
    
    /**
     * @brief Reset search statistics to zero
     */
    void reset_stats() { current_stats.reset(); }
    
private:
    /**
     * @brief Core minimax algorithm with alpha-beta pruning
     * 
     * @param board The current board position
     * @param depth Remaining search depth
     * @param alpha Alpha value for pruning
     * @param beta Beta value for pruning
     * @param start_time Search start time for time management
     * @param time_limit Maximum search time allowed
     * @return Best evaluation score found
     */
    int minimax(Board& board, int depth, int alpha, int beta, 
                std::chrono::steady_clock::time_point start_time, 
                std::chrono::milliseconds time_limit);
    
    // Helper functions
    /**
     * @brief Check if search time limit has been exceeded
     * 
     * @param start_time Search start time
     * @param time_limit Maximum allowed time
     * @return true if time limit exceeded
     */
    bool is_time_up(std::chrono::steady_clock::time_point start_time, 
                    std::chrono::milliseconds time_limit) const;
    
    /**
     * @brief Check if score represents a mate position
     * 
     * @param score Evaluation score to check
     * @return true if score indicates mate
     */
    bool is_mate_score(int score) const;
    
    /**
     * @brief Calculate distance to mate from mate score
     * 
     * @param score Mate evaluation score
     * @return Number of moves until mate
     */
    int mate_distance(int score) const;
    
    /**
     * @brief Check if position is a draw
     * 
     * @param board Board position to evaluate
     * @return true if position is drawn
     */
    bool is_draw(const Board& board) const;
    
    // Move ordering for better alpha-beta pruning
    /**
     * @brief Order moves for better search efficiency
     * 
     * @param moves Vector of moves to sort
     * @param board Current board position
     */
    void order_moves(std::vector<Move>& moves, const Board& board);
    
    /**
     * @brief Calculate heuristic score for move ordering
     * 
     * @param move Move to evaluate
     * @param board Current board position
     * @return Heuristic score for move ordering
     */
    int get_move_score(const Move& move, const Board& board) const;
};

#endif // SEARCH_H