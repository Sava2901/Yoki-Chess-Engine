#include "Search.h"
#include "Evaluation.h"
#include <algorithm>
#include <iostream>
#include <chrono>

// Initialize static members
int Search::nodes_searched = 0;
int Search::cutoffs = 0;
Search::TTEntry Search::transposition_table[Search::TT_SIZE] = {};

Move Search::find_best_move(Board& board, int depth) {
    nodes_searched = 0;
    cutoffs = 0;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    MoveList legal_moves = MoveGenerator::generate_legal_moves(board);
    
    if (legal_moves.empty()) {
        std::cout << "No legal moves available!" << std::endl;
        return Move(); // Return invalid move
    }
    
    Move best_move = legal_moves[0];
    int best_score = -INFINITY_SCORE;
    
    // Order moves for better alpha-beta pruning
    order_moves(board, legal_moves);
    
    std::cout << "Searching " << legal_moves.size() << " moves at depth " << depth << std::endl;
    
    for (const Move& move : legal_moves) {
        // Make the move
        Board::BoardState state;
        board.make_move(move,  state);
        
        // Search this move
        int score = -minimax(board, depth - 1, -INFINITY_SCORE, INFINITY_SCORE, false);
        
        // Undo the move
        board.undo_move(move, state);
        
        std::cout << "Move " << move.to_algebraic() << " scored " << score << std::endl;
        
        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Best move: " << best_move.to_algebraic() << " (score: " << best_score << ")" << std::endl;
    std::cout << "Nodes searched: " << nodes_searched << std::endl;
    std::cout << "Cutoffs: " << cutoffs << std::endl;
    std::cout << "Search time: " << duration.count() << "ms" << std::endl;
    
    return best_move;
}

int Search::minimax(Board& board, int depth, int alpha, int beta, bool maximizing_player) {
    nodes_searched++;
    
    // Terminal node - evaluate position
    if (depth == 0) {
        return quiescence_search(board, alpha, beta, maximizing_player);
    }
    
    MoveList legal_moves = MoveGenerator::generate_legal_moves(board);
    
    // Check for checkmate or stalemate
    if (legal_moves.empty()) {
        if (MoveGenerator::is_in_check(board, board.get_active_color())) {
            // Checkmate - return mate score adjusted for depth
            return maximizing_player ? -MATE_SCORE + depth : MATE_SCORE - depth;
        } else {
            // Stalemate
            return 0;
        }
    }
    
    // Order moves for better pruning
    order_moves(board, legal_moves);
    
    if (maximizing_player) {
        int max_eval = -INFINITY_SCORE;
        
        for (const Move& move : legal_moves) {
            Board::BoardState state;
            board.make_move(move, state);
            
            int eval = minimax(board, depth - 1, alpha, beta, false);
            
            board.undo_move(move, state);
            
            max_eval = std::max(max_eval, eval);
            alpha = std::max(alpha, eval);
            
            if (beta <= alpha) {
                cutoffs++;
                break; // Beta cutoff
            }
        }
        
        return max_eval;
    } else {
        int min_eval = INFINITY_SCORE;
        
        for (const Move& move : legal_moves) {
            Board::BoardState state;
            board.make_move(move, state);
            
            int eval = minimax(board, depth - 1, alpha, beta, true);
            
            board.undo_move(move, state);
            
            min_eval = std::min(min_eval, eval);
            beta = std::min(beta, eval);
            
            if (beta <= alpha) {
                cutoffs++;
                break; // Alpha cutoff
            }
        }
        
        return min_eval;
    }
}

int Search::quiescence_search(Board& board, int alpha, int beta, bool maximizing_player) {
    nodes_searched++;
    
    // Stand pat - evaluate current position
    int stand_pat = Evaluation::evaluate_position(board);
    
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
    MoveList captures;
    MoveList all_moves = MoveGenerator::generate_legal_moves(board);
    
    for (const Move& move : all_moves) {
        if (move.captured_piece != '.' || 
            MoveGenerator::is_in_check(board, board.get_active_color() == 'w' ? 'b' : 'w')) {
            captures.push_back(move);
        }
    }
    
    // Order captures by MVV-LVA (Most Valuable Victim - Least Valuable Attacker)
    order_moves(board, captures);
    
    for (const Move& move : captures) {
        Board::BoardState state;
        board.make_move(move, state);
        
        int score = quiescence_search(board, alpha, beta, !maximizing_player);
        
        board.undo_move(move, state);
        
        if (maximizing_player) {
            alpha = std::max(alpha, score);
            if (beta <= alpha) {
                cutoffs++;
                break;
            }
        } else {
            beta = std::min(beta, score);
            if (beta <= alpha) {
                cutoffs++;
                break;
            }
        }
    }
    
    return maximizing_player ? alpha : beta;
}

void Search::order_moves(Board& board, MoveList& moves) {
    // Sort moves by score (highest first)
    std::sort(moves.begin(), moves.end(), [&board](const Move& a, const Move& b) {
        return get_move_score(board, a) > get_move_score(board, b);
    });
}

int Search::get_move_score(Board& board, const Move& move) {
    int score = 0;
    
    // Prioritize captures
    if (move.captured_piece != '.') {
        int victim_value = Evaluation::get_piece_value(move.captured_piece);
        int attacker_value = Evaluation::get_piece_value(move.piece);
        score += 1000 + victim_value - attacker_value; // MVV-LVA
    }
    
    // Prioritize promotions
    if (move.promotion_piece != '.') {
        score += 900 + Evaluation::get_piece_value(move.promotion_piece);
    }
    
    // Prioritize castling
    if (move.is_castling) {
        score += 50;
    }
    
    // Prioritize center control
    int center_bonus = 0;
    if ((move.to_rank >= 3 && move.to_rank <= 4) && (move.to_file >= 3 && move.to_file <= 4)) {
        center_bonus = 20;
    } else if ((move.to_rank >= 2 && move.to_rank <= 5) && (move.to_file >= 2 && move.to_file <= 5)) {
        center_bonus = 10;
    }
    score += center_bonus;
    
    return score;
}

bool Search::is_quiet_position(Board& board) {
    // Check if king is in check
    if (MoveGenerator::is_in_check(board, board.get_active_color())) {
        return false;
    }
    
    // Check for immediate captures
    MoveList moves = MoveGenerator::generate_legal_moves(board);
    for (const Move& move : moves) {
        if (move.captured_piece != '.') {
            return false;
        }
    }
    
    return true;
}

uint64_t Search::hash_position(const Board& board) {
    // Simple hash function - in a real engine, use Zobrist hashing
    uint64_t hash = 0;
    
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            char piece = board.get_piece(rank, file);
            if (piece != '.') {
                hash ^= (uint64_t(piece) << (rank * 8 + file));
            }
        }
    }
    
    hash ^= (board.get_active_color() == 'w' ? 1 : 0);
    
    return hash;
}