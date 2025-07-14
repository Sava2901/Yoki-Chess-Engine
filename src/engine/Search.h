#ifndef SEARCH_H
#define SEARCH_H

#include "../board/Board.h"
#include "../board/Move.h"
#include "../board/MoveGenerator.h"
#include "Evaluation.h"
#include <vector>
#include <chrono>

/**
 * Simple Minimax Search Implementation
 * This class implements the basic minimax algorithm with alpha-beta pruning
 * for chess move searching using existing board and evaluation functions.
 */
class Search {
public:
    // Search statistics
    struct SearchStats {
        int nodes_searched = 0;
        int beta_cutoffs = 0;
        std::chrono::milliseconds time_elapsed{0};
        
        void reset() {
            nodes_searched = 0;
            beta_cutoffs = 0;
            time_elapsed = std::chrono::milliseconds(0);
        }
    };
    
    // Search result
    struct SearchResult {
        Move best_move;
        int score = 0;
        int depth = 0;
        SearchStats stats;
        bool is_mate = false;
        int mate_in = 0;
    };
    
private:
    MoveGenerator move_generator;
    Evaluation* evaluation = nullptr;
    SearchStats current_stats;
    
    // Search parameters
    static constexpr int MAX_DEPTH = 10;
    static constexpr int MATE_SCORE = 30000;
    static constexpr int ALPHA_INIT = -31000;
    static constexpr int BETA_INIT = 31000;
    
public:
    Search();
    ~Search() = default;
    
    // Main search functions
    SearchResult find_best_move(Board& board, int max_depth, std::chrono::milliseconds time_limit = std::chrono::milliseconds(0));
    SearchResult find_best_move_timed(Board& board, std::chrono::milliseconds time_limit);
    
    // Configuration
    void set_evaluation(Evaluation* eval) { evaluation = eval; }
    SearchStats get_stats() const { return current_stats; }
    void reset_stats() { current_stats.reset(); }
    
private:
    // Core minimax algorithm
    int minimax(Board& board, int depth, int alpha, int beta, 
                std::chrono::steady_clock::time_point start_time, 
                std::chrono::milliseconds time_limit);
    
    // Helper functions
    bool is_time_up(std::chrono::steady_clock::time_point start_time, 
                    std::chrono::milliseconds time_limit) const;
    bool is_mate_score(int score) const;
    int mate_distance(int score) const;
    bool is_draw(const Board& board) const;
    
    // Move ordering for better alpha-beta pruning
    void order_moves(std::vector<Move>& moves, const Board& board);
    int get_move_score(const Move& move, const Board& board) const;
};

#endif // SEARCH_H