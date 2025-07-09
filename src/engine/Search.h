#ifndef SEARCH_H
#define SEARCH_H

#include "../board/Board.h"
#include "../board/Move.h"
#include "../board/MoveGenerator.h"
#include <limits>
#include <vector>
#include <cstdint>

class Search {
public:
    // Search configuration
    static const int MAX_DEPTH = 10;
    static const int INFINITY_SCORE = 999999;
    static const int MATE_SCORE = 100000;
    
    // Main search function
    static Move find_best_move(Board& board, int depth);
    
    // Minimax with alpha-beta pruning
    static int minimax(Board& board, int depth, int alpha, int beta, bool maximizing_player);
    
    // Quiescence search for tactical positions
    static int quiescence_search(Board& board, int alpha, int beta, bool maximizing_player);
    
    // Move ordering for better alpha-beta pruning
    static void order_moves(Board& board, MoveList& moves);
    
    // Check if position is quiet (no captures/checks)
    static bool is_quiet_position(Board& board);
    
    // Get move score for ordering
    static int get_move_score(Board& board, const Move& move);
    
private:
    // Search statistics
    static int nodes_searched;
    static int cutoffs;
    
    // Transposition table entry
    struct TTEntry {
        uint64_t hash;
        int depth;
        int score;
        Move best_move;
        enum NodeType { EXACT, LOWER_BOUND, UPPER_BOUND } type;
    };
    
    // Simple transposition table
    static const int TT_SIZE = 1024 * 1024; // 1M entries
    static TTEntry transposition_table[TT_SIZE];
    
    // Hash function for positions
    static uint64_t hash_position(const Board& board);
};

#endif // SEARCH_H