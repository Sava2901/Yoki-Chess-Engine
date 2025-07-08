#ifndef YOKI_SEARCH_H
#define YOKI_SEARCH_H

#include "board.h"
#include "movegen.h"
#include <chrono>
#include <unordered_map>

namespace yoki {

// Search result structure
struct SearchResult {
    Move best_move;
    int score;
    int depth;
    int nodes_searched;
    std::chrono::milliseconds time_taken;
    std::vector<Move> principal_variation;
    
    SearchResult() : score(0), depth(0), nodes_searched(0), time_taken(0) {}
};

// Transposition table entry
struct TTEntry {
    uint64_t hash;
    Move best_move;
    int score;
    int depth;
    enum NodeType { EXACT, LOWER_BOUND, UPPER_BOUND } node_type;
    
    TTEntry() : hash(0), score(0), depth(0), node_type(EXACT) {}
};

// Search engine class
class SearchEngine {
public:
    explicit SearchEngine(int max_depth = 8);
    
    // Main search interface
    SearchResult search(const Board& board, int max_time_ms = 5000);
    SearchResult search_depth(const Board& board, int depth);
    
    // Evaluation function
    int evaluate(const Board& board) const;
    
    // Search statistics
    void reset_stats();
    int get_nodes_searched() const { return nodes_searched_; }
    
    // Configuration
    void set_max_depth(int depth) { max_depth_ = depth; }
    void enable_transposition_table(bool enable) { use_tt_ = enable; }
    
private:
    int max_depth_;
    bool use_tt_;
    mutable int nodes_searched_;
    std::chrono::steady_clock::time_point search_start_time_;
    int max_search_time_ms_;
    
    // Transposition table
    std::unordered_map<uint64_t, TTEntry> transposition_table_;
    
    // Core search functions
    int alpha_beta(const Board& board, int depth, int alpha, int beta, bool maximizing_player);
    int quiescence_search(const Board& board, int alpha, int beta, bool maximizing_player);
    
    // Move ordering
    std::vector<Move> order_moves(const Board& board, const std::vector<Move>& moves) const;
    int get_move_score(const Board& board, const Move& move) const;
    
    // Evaluation components
    int evaluate_material(const Board& board) const;
    int evaluate_piece_square_tables(const Board& board) const;
    int evaluate_king_safety(const Board& board) const;
    int evaluate_mobility(const Board& board) const;
    
    // Utility functions
    bool is_time_up() const;
    bool is_capture(const Board& board, const Move& move) const;
    bool is_check(const Board& board, const Move& move) const;
    
    // Transposition table operations
    bool probe_tt(uint64_t hash, int depth, int alpha, int beta, TTEntry& entry) const;
    void store_tt(uint64_t hash, const Move& best_move, int score, int depth, TTEntry::NodeType node_type);
};

// Piece values for evaluation
namespace PieceValues {
    constexpr int PAWN_VALUE = 100;
    constexpr int KNIGHT_VALUE = 320;
    constexpr int BISHOP_VALUE = 330;
    constexpr int ROOK_VALUE = 500;
    constexpr int QUEEN_VALUE = 900;
    constexpr int KING_VALUE = 20000;
}

// Piece-square tables for positional evaluation
namespace PieceSquareTables {
    extern const int pawn_table[64];
    extern const int knight_table[64];
    extern const int bishop_table[64];
    extern const int rook_table[64];
    extern const int queen_table[64];
    extern const int king_middle_game_table[64];
    extern const int king_end_game_table[64];
}

} // namespace yoki

#endif // YOKI_SEARCH_H