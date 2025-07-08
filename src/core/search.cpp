#include "search.h"
#include <algorithm>
#include <limits>
#include <cmath>

namespace yoki {

// Piece-square tables (from white's perspective)
namespace PieceSquareTables {
    const int pawn_table[64] = {
         0,  0,  0,  0,  0,  0,  0,  0,
        50, 50, 50, 50, 50, 50, 50, 50,
        10, 10, 20, 30, 30, 20, 10, 10,
         5,  5, 10, 25, 25, 10,  5,  5,
         0,  0,  0, 20, 20,  0,  0,  0,
         5, -5,-10,  0,  0,-10, -5,  5,
         5, 10, 10,-20,-20, 10, 10,  5,
         0,  0,  0,  0,  0,  0,  0,  0
    };
    
    const int knight_table[64] = {
        -50,-40,-30,-30,-30,-30,-40,-50,
        -40,-20,  0,  0,  0,  0,-20,-40,
        -30,  0, 10, 15, 15, 10,  0,-30,
        -30,  5, 15, 20, 20, 15,  5,-30,
        -30,  0, 15, 20, 20, 15,  0,-30,
        -30,  5, 10, 15, 15, 10,  5,-30,
        -40,-20,  0,  5,  5,  0,-20,-40,
        -50,-40,-30,-30,-30,-30,-40,-50
    };
    
    const int bishop_table[64] = {
        -20,-10,-10,-10,-10,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5, 10, 10,  5,  0,-10,
        -10,  5,  5, 10, 10,  5,  5,-10,
        -10,  0, 10, 10, 10, 10,  0,-10,
        -10, 10, 10, 10, 10, 10, 10,-10,
        -10,  5,  0,  0,  0,  0,  5,-10,
        -20,-10,-10,-10,-10,-10,-10,-20
    };
    
    const int rook_table[64] = {
         0,  0,  0,  0,  0,  0,  0,  0,
         5, 10, 10, 10, 10, 10, 10,  5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
         0,  0,  0,  5,  5,  0,  0,  0
    };
    
    const int queen_table[64] = {
        -20,-10,-10, -5, -5,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5,  5,  5,  5,  0,-10,
         -5,  0,  5,  5,  5,  5,  0, -5,
          0,  0,  5,  5,  5,  5,  0, -5,
        -10,  5,  5,  5,  5,  5,  0,-10,
        -10,  0,  5,  0,  0,  0,  0,-10,
        -20,-10,-10, -5, -5,-10,-10,-20
    };
    
    const int king_middle_game_table[64] = {
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -20,-30,-30,-40,-40,-30,-30,-20,
        -10,-20,-20,-20,-20,-20,-20,-10,
         20, 20,  0,  0,  0,  0, 20, 20,
         20, 30, 10,  0,  0, 10, 30, 20
    };
    
    const int king_end_game_table[64] = {
        -50,-40,-30,-20,-20,-30,-40,-50,
        -30,-20,-10,  0,  0,-10,-20,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-30,  0,  0,  0,  0,-30,-30,
        -50,-30,-30,-30,-30,-30,-30,-50
    };
}

SearchEngine::SearchEngine(int max_depth) 
    : max_depth_(max_depth), use_tt_(true), nodes_searched_(0), max_search_time_ms_(5000) {
}

SearchResult SearchEngine::search(const Board& board, int max_time_ms) {
    max_search_time_ms_ = max_time_ms;
    search_start_time_ = std::chrono::steady_clock::now();
    
    SearchResult result;
    reset_stats();
    
    MoveGenerator movegen(board);
    std::vector<Move> legal_moves = movegen.generate_legal_moves();
    
    if (legal_moves.empty()) {
        return result; // No legal moves
    }
    
    if (legal_moves.size() == 1) {
        result.best_move = legal_moves[0];
        result.depth = 1;
        return result;
    }
    
    // Iterative deepening
    for (int depth = 1; depth <= max_depth_; ++depth) {
        if (is_time_up()) break;
        
        int best_score = std::numeric_limits<int>::lowest();
        Move best_move;
        
        // Order moves for better alpha-beta pruning
        std::vector<Move> ordered_moves = order_moves(board, legal_moves);
        
        int alpha = std::numeric_limits<int>::lowest();
        int beta = std::numeric_limits<int>::max();
        
        for (const Move& move : ordered_moves) {
            if (is_time_up()) break;
            
            Board temp_board = board;
            if (!temp_board.make_move(move)) continue;
            
            int score = -alpha_beta(temp_board, depth - 1, -beta, -alpha, false);
            
            if (score > best_score) {
                best_score = score;
                best_move = move;
                alpha = std::max(alpha, score);
            }
        }
        
        result.best_move = best_move;
        result.score = best_score;
        result.depth = depth;
        
        // If we found a mate, no need to search deeper
        if (std::abs(best_score) > PieceValues::KING_VALUE - 1000) {
            break;
        }
    }
    
    result.nodes_searched = nodes_searched_;
    result.time_taken = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - search_start_time_);
    
    return result;
}

SearchResult SearchEngine::search_depth(const Board& board, int depth) {
    max_search_time_ms_ = std::numeric_limits<int>::max();
    search_start_time_ = std::chrono::steady_clock::now();
    
    SearchResult result;
    reset_stats();
    
    MoveGenerator movegen(board);
    std::vector<Move> legal_moves = movegen.generate_legal_moves();
    
    if (legal_moves.empty()) {
        return result;
    }
    
    int best_score = std::numeric_limits<int>::lowest();
    Move best_move;
    
    std::vector<Move> ordered_moves = order_moves(board, legal_moves);
    
    int alpha = std::numeric_limits<int>::lowest();
    int beta = std::numeric_limits<int>::max();
    
    for (const Move& move : ordered_moves) {
        Board temp_board = board;
        if (!temp_board.make_move(move)) continue;
        
        int score = -alpha_beta(temp_board, depth - 1, -beta, -alpha, false);
        
        if (score > best_score) {
            best_score = score;
            best_move = move;
            alpha = std::max(alpha, score);
        }
    }
    
    result.best_move = best_move;
    result.score = best_score;
    result.depth = depth;
    result.nodes_searched = nodes_searched_;
    result.time_taken = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - search_start_time_);
    
    return result;
}

int SearchEngine::alpha_beta(const Board& board, int depth, int alpha, int beta, bool maximizing_player) {
    nodes_searched_++;
    
    if (is_time_up()) {
        return evaluate(board);
    }
    
    // Check transposition table
    TTEntry tt_entry;
    if (use_tt_ && probe_tt(board.get_hash(), depth, alpha, beta, tt_entry)) {
        return tt_entry.score;
    }
    
    if (depth == 0) {
        return quiescence_search(board, alpha, beta, maximizing_player);
    }
    
    MoveGenerator movegen(board);
    std::vector<Move> legal_moves = movegen.generate_legal_moves();
    
    if (legal_moves.empty()) {
        if (board.is_in_check(board.get_side_to_move())) {
            // Checkmate
            return maximizing_player ? -PieceValues::KING_VALUE + depth : PieceValues::KING_VALUE - depth;
        } else {
            // Stalemate
            return 0;
        }
    }
    
    std::vector<Move> ordered_moves = order_moves(board, legal_moves);
    
    Move best_move;
    TTEntry::NodeType node_type = TTEntry::UPPER_BOUND;
    
    if (maximizing_player) {
        int max_eval = std::numeric_limits<int>::lowest();
        
        for (const Move& move : ordered_moves) {
            Board temp_board = board;
            if (!temp_board.make_move(move)) continue;
            
            int eval = alpha_beta(temp_board, depth - 1, alpha, beta, false);
            
            if (eval > max_eval) {
                max_eval = eval;
                best_move = move;
            }
            
            alpha = std::max(alpha, eval);
            if (beta <= alpha) {
                node_type = TTEntry::LOWER_BOUND;
                break; // Beta cutoff
            }
        }
        
        if (alpha > std::numeric_limits<int>::lowest()) {
            node_type = TTEntry::EXACT;
        }
        
        if (use_tt_) {
            store_tt(board.get_hash(), best_move, max_eval, depth, node_type);
        }
        
        return max_eval;
    } else {
        int min_eval = std::numeric_limits<int>::max();
        
        for (const Move& move : ordered_moves) {
            Board temp_board = board;
            if (!temp_board.make_move(move)) continue;
            
            int eval = alpha_beta(temp_board, depth - 1, alpha, beta, true);
            
            if (eval < min_eval) {
                min_eval = eval;
                best_move = move;
            }
            
            beta = std::min(beta, eval);
            if (beta <= alpha) {
                node_type = TTEntry::UPPER_BOUND;
                break; // Alpha cutoff
            }
        }
        
        if (beta < std::numeric_limits<int>::max()) {
            node_type = TTEntry::EXACT;
        }
        
        if (use_tt_) {
            store_tt(board.get_hash(), best_move, min_eval, depth, node_type);
        }
        
        return min_eval;
    }
}

int SearchEngine::quiescence_search(const Board& board, int alpha, int beta, bool maximizing_player) {
    nodes_searched_++;
    
    int stand_pat = evaluate(board);
    
    if (maximizing_player) {
        if (stand_pat >= beta) {
            return beta;
        }
        alpha = std::max(alpha, stand_pat);
    } else {
        if (stand_pat <= alpha) {
            return alpha;
        }
        beta = std::min(beta, stand_pat);
    }
    
    // Only search captures and checks in quiescence
    MoveGenerator movegen(board);
    std::vector<Move> legal_moves = movegen.generate_legal_moves();
    
    std::vector<Move> tactical_moves;
    for (const Move& move : legal_moves) {
        if (is_capture(board, move) || is_check(board, move)) {
            tactical_moves.push_back(move);
        }
    }
    
    std::vector<Move> ordered_moves = order_moves(board, tactical_moves);
    
    for (const Move& move : ordered_moves) {
        Board temp_board = board;
        if (!temp_board.make_move(move)) continue;
        
        int score = quiescence_search(temp_board, alpha, beta, !maximizing_player);
        
        if (maximizing_player) {
            alpha = std::max(alpha, score);
            if (beta <= alpha) {
                break;
            }
        } else {
            beta = std::min(beta, score);
            if (beta <= alpha) {
                break;
            }
        }
    }
    
    return maximizing_player ? alpha : beta;
}

int SearchEngine::evaluate(const Board& board) const {
    int score = 0;
    
    score += evaluate_material(board);
    score += evaluate_piece_square_tables(board);
    score += evaluate_king_safety(board);
    score += evaluate_mobility(board);
    
    // Return score from the perspective of the side to move
    return (board.get_side_to_move() == WHITE) ? score : -score;
}

int SearchEngine::evaluate_material(const Board& board) const {
    int score = 0;
    
    for (int sq = 0; sq < 64; ++sq) {
        if (board.is_empty(sq)) continue;
        
        PieceType piece = board.get_piece_type(sq);
        Color color = board.get_piece_color(sq);
        
        int piece_value = 0;
        switch (piece) {
            case PAWN: piece_value = PieceValues::PAWN_VALUE; break;
            case KNIGHT: piece_value = PieceValues::KNIGHT_VALUE; break;
            case BISHOP: piece_value = PieceValues::BISHOP_VALUE; break;
            case ROOK: piece_value = PieceValues::ROOK_VALUE; break;
            case QUEEN: piece_value = PieceValues::QUEEN_VALUE; break;
            case KING: piece_value = PieceValues::KING_VALUE; break;
            default: break;
        }
        
        score += (color == WHITE) ? piece_value : -piece_value;
    }
    
    return score;
}

int SearchEngine::evaluate_piece_square_tables(const Board& board) const {
    int score = 0;
    
    for (int sq = 0; sq < 64; ++sq) {
        if (board.is_empty(sq)) continue;
        
        PieceType piece = board.get_piece_type(sq);
        Color color = board.get_piece_color(sq);
        
        // Flip square for black pieces
        int table_sq = (color == WHITE) ? sq : (56 - (sq / 8) * 8 + sq % 8);
        
        int positional_value = 0;
        switch (piece) {
            case PAWN: positional_value = PieceSquareTables::pawn_table[table_sq]; break;
            case KNIGHT: positional_value = PieceSquareTables::knight_table[table_sq]; break;
            case BISHOP: positional_value = PieceSquareTables::bishop_table[table_sq]; break;
            case ROOK: positional_value = PieceSquareTables::rook_table[table_sq]; break;
            case QUEEN: positional_value = PieceSquareTables::queen_table[table_sq]; break;
            case KING: positional_value = PieceSquareTables::king_middle_game_table[table_sq]; break;
            default: break;
        }
        
        score += (color == WHITE) ? positional_value : -positional_value;
    }
    
    return score;
}

int SearchEngine::evaluate_king_safety(const Board& board) const {
    // TODO: Implement king safety evaluation
    return 0;
}

int SearchEngine::evaluate_mobility(const Board& board) const {
    // TODO: Implement mobility evaluation
    return 0;
}

std::vector<Move> SearchEngine::order_moves(const Board& board, const std::vector<Move>& moves) const {
    std::vector<std::pair<Move, int>> scored_moves;
    
    for (const Move& move : moves) {
        int score = get_move_score(board, move);
        scored_moves.emplace_back(move, score);
    }
    
    // Sort by score (highest first)
    std::sort(scored_moves.begin(), scored_moves.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<Move> ordered_moves;
    for (const auto& pair : scored_moves) {
        ordered_moves.push_back(pair.first);
    }
    
    return ordered_moves;
}

int SearchEngine::get_move_score(const Board& board, const Move& move) const {
    int score = 0;
    
    // Prioritize captures
    if (is_capture(board, move)) {
        PieceType captured = board.get_piece_type(move.to);
        PieceType attacker = board.get_piece_type(move.from);
        
        // MVV-LVA (Most Valuable Victim - Least Valuable Attacker)
        int victim_value = 0;
        int attacker_value = 0;
        
        switch (captured) {
            case PAWN: victim_value = 1; break;
            case KNIGHT: case BISHOP: victim_value = 3; break;
            case ROOK: victim_value = 5; break;
            case QUEEN: victim_value = 9; break;
            default: break;
        }
        
        switch (attacker) {
            case PAWN: attacker_value = 1; break;
            case KNIGHT: case BISHOP: attacker_value = 3; break;
            case ROOK: attacker_value = 5; break;
            case QUEEN: attacker_value = 9; break;
            default: break;
        }
        
        score += victim_value * 10 - attacker_value;
    }
    
    // Prioritize promotions
    if (move.promotion != EMPTY) {
        score += 8;
    }
    
    return score;
}

void SearchEngine::reset_stats() {
    nodes_searched_ = 0;
}

bool SearchEngine::is_time_up() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - search_start_time_);
    return elapsed.count() >= max_search_time_ms_;
}

bool SearchEngine::is_capture(const Board& board, const Move& move) const {
    return !board.is_empty(move.to);
}

bool SearchEngine::is_check(const Board& board, const Move& move) const {
    Board temp_board = board;
    if (!temp_board.make_move(move)) return false;
    
    Color opponent = (board.get_side_to_move() == WHITE) ? BLACK : WHITE;
    return temp_board.is_in_check(opponent);
}

bool SearchEngine::probe_tt(uint64_t hash, int depth, int alpha, int beta, TTEntry& entry) const {
    auto it = transposition_table_.find(hash);
    if (it == transposition_table_.end()) {
        return false;
    }
    
    entry = it->second;
    
    if (entry.depth >= depth) {
        switch (entry.node_type) {
            case TTEntry::EXACT:
                return true;
            case TTEntry::LOWER_BOUND:
                if (entry.score >= beta) return true;
                break;
            case TTEntry::UPPER_BOUND:
                if (entry.score <= alpha) return true;
                break;
        }
    }
    
    return false;
}

void SearchEngine::store_tt(uint64_t hash, const Move& best_move, int score, int depth, TTEntry::NodeType node_type) {
    TTEntry entry;
    entry.hash = hash;
    entry.best_move = best_move;
    entry.score = score;
    entry.depth = depth;
    entry.node_type = node_type;
    
    transposition_table_[hash] = entry;
}

} // namespace yoki