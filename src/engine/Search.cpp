#include "Search.h"
#include <algorithm>
#include <iostream>
Search::Search() {
    current_stats.reset();
}

Search::SearchResult Search::find_best_move(Board& board, int max_depth, std::chrono::milliseconds time_limit) {
    SearchResult result;
    current_stats.reset();
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Generate all legal moves for the current player
    std::vector<Move> legal_moves = move_generator.generate_legal_moves(board);
    
    if (legal_moves.empty()) {
        // No legal moves - checkmate or stalemate
        if (move_generator.is_in_check(board, board.get_active_color())) {
            result.is_mate = true;
            result.mate_in = 0;
            result.score = -MATE_SCORE;
        } else {
            result.score = 0; // Stalemate
        }
        return result;
    }
    
    // Order moves for better alpha-beta pruning
    // order_moves(legal_moves, board);
    
    Move best_move = legal_moves[0];
    int best_score = ALPHA_INIT;
    
    // Search up to the specified max_depth with optional time failsafe
    for (int depth = 1; depth <= max_depth; ++depth) {
        // Check time limit only if specified (time_limit > 0)
        if (time_limit.count() > 0 && is_time_up(start_time, time_limit)) {
            break;
        }
        
        int alpha = ALPHA_INIT;
        int beta = BETA_INIT;
        Move current_best_move = legal_moves[0];
        int current_best_score = ALPHA_INIT;
        
        for (const Move& move : legal_moves) {
            // Check time limit only if specified
            if (time_limit.count() > 0 && is_time_up(start_time, time_limit)) {
                break;
            }
            
            // Make the move
            auto undo_data = board.make_move(move);
            
            // Search with minimax - now the opponent is to move
            int score = -minimax(board, depth - 1, -beta, -alpha, start_time, time_limit);
            
            // Undo the move immediately
            board.undo_move(undo_data);
            
            if (score > current_best_score) {
                current_best_score = score;
                current_best_move = move;
            }
            
            alpha = std::max(alpha, score);
            if (alpha >= beta) {
                current_stats.beta_cutoffs++;
                break; // Beta cutoff
            }
        }
        
        // Update best move and score if we completed the depth or no time limit
        if (time_limit.count() == 0 || !is_time_up(start_time, time_limit)) {
            best_move = current_best_move;
            best_score = current_best_score;
            result.depth = depth;
            
            // Check for mate
            if (is_mate_score(best_score)) {
                result.is_mate = true;
                result.mate_in = mate_distance(best_score);
                break; // Found mate, no need to search deeper
            }
        }
    }
    
    result.best_move = best_move;
    result.score = best_score;
    result.stats = current_stats;
    
    auto end_time = std::chrono::steady_clock::now();
    result.stats.time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    return result;
}

Search::SearchResult Search::find_best_move_timed(Board& board, std::chrono::milliseconds time_limit) {
    SearchResult result;
    current_stats.reset();
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Generate all legal moves for the current player
    std::vector<Move> legal_moves = move_generator.generate_legal_moves(board);
    
    if (legal_moves.empty()) {
        // No legal moves - checkmate or stalemate
        if (move_generator.is_in_check(board, board.get_active_color())) {
            result.is_mate = true;
            result.mate_in = 0;
            result.score = -MATE_SCORE;
        } else {
            result.score = 0; // Stalemate
        }
        return result;
    }
    
    // Order moves for better alpha-beta pruning
    order_moves(legal_moves, board);
    
    Move best_move = legal_moves[0];
    int best_score = ALPHA_INIT;
    
    // Iterative deepening from depth 1 to MAX_DEPTH
    for (int depth = 1; depth <= MAX_DEPTH; ++depth) {
        if (is_time_up(start_time, time_limit)) {
            break;
        }
        
        int alpha = ALPHA_INIT;
        int beta = BETA_INIT;
        Move current_best_move = legal_moves[0];
        int current_best_score = ALPHA_INIT;
        
        for (const Move& move : legal_moves) {
        if (time_limit.count() > 0 && is_time_up(start_time, time_limit)) {
            break;
        }
            
            // Make the move
            auto undo_data = board.make_move(move);
            
            // Search with minimax - now the opponent is to move
            int score = -minimax(board, depth - 1, -beta, -alpha, start_time, time_limit);
            
            // Undo the move immediately
            board.undo_move(undo_data);
            
            if (score > current_best_score) {
                current_best_score = score;
                current_best_move = move;
            }
            
            alpha = std::max(alpha, score);
            if (alpha >= beta) {
                current_stats.beta_cutoffs++;
                break; // Beta cutoff
            }
        }
        
        // Update best move and score if we completed the depth
        if (!is_time_up(start_time, time_limit)) {
            best_move = current_best_move;
            best_score = current_best_score;
            result.depth = depth;
            
            // Check for mate
            if (is_mate_score(best_score)) {
                result.is_mate = true;
                result.mate_in = mate_distance(best_score);
                break; // Found mate, no need to search deeper
            }
        }
    }
    
    result.best_move = best_move;
    result.score = best_score;
    result.stats = current_stats;
    
    auto end_time = std::chrono::steady_clock::now();
    result.stats.time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    return result;
}

int Search::minimax(Board& board, int depth, int alpha, int beta,
                   std::chrono::steady_clock::time_point start_time,
                   std::chrono::milliseconds time_limit) {
    current_stats.nodes_searched++;
    
    // Check time limit only if specified
    if (time_limit.count() > 0 && is_time_up(start_time, time_limit)) {
        return 0; // Return neutral score if time is up
    }
    
    // Terminal node - evaluate position
    if (depth == 0) {
        if (evaluation) {
            return evaluation->evaluate(board);
        }
        return 0; // No evaluation function available
    }
    
    // Check for draw
    if (is_draw(board)) {
        return 0;
    }
    
    // Generate legal moves for the current active player
    std::vector<Move> legal_moves = move_generator.generate_legal_moves(board);
    
    if (legal_moves.empty()) {
        // No legal moves - checkmate or stalemate
        if (move_generator.is_in_check(board, board.get_active_color())) {
            // Checkmate - return negative mate score (bad for current player)
            return -MATE_SCORE + depth;
        }
        // Stalemate
        return 0;
    }
    
    // Order moves for better pruning
    order_moves(legal_moves, board);
    
    int best_score = ALPHA_INIT;
    
    for (const Move& move : legal_moves) {
        if (is_time_up(start_time, time_limit)) {
            break;
        }
        
        // Make the move
        board.print();
        auto undo_data = board.make_move(move);
        
        // Recursive call with negated alpha-beta window
        // After making a move, it's the opponent's turn, so we negate the result
        int score = -minimax(board, depth - 1, -beta, -alpha, start_time, time_limit);
        std::cout << "Score for move " << move.to_algebraic() << ": " << score << "\n";
        // Undo the move immediately

        board.print();
        board.undo_move(undo_data);
        
        best_score = std::max(best_score, score);
        alpha = std::max(alpha, score);
        
        if (alpha >= beta) {
            current_stats.beta_cutoffs++;
            break; // Beta cutoff
        }
    }
    
    return best_score;
}

bool Search::is_time_up(std::chrono::steady_clock::time_point start_time,
                       std::chrono::milliseconds time_limit) const {
    if (time_limit.count() <= 0) {
        return false; // No time limit
    }
    
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
    return elapsed >= time_limit;
}

bool Search::is_mate_score(int score) const {
    return std::abs(score) >= MATE_SCORE - MAX_DEPTH;
}

int Search::mate_distance(int score) const {
    if (score > 0) {
        return (MATE_SCORE - score + 1) / 2;
    } else {
        return -(MATE_SCORE + score + 1) / 2;
    }
}

bool Search::is_draw(const Board& board) const {
    // Check 50-move rule
    if (board.get_halfmove_clock() >= 100) {
        return true;
    }
    
    // Basic insufficient material check
    Bitboard white_pieces = board.get_color_bitboard(Board::WHITE);
    Bitboard black_pieces = board.get_color_bitboard(Board::BLACK);
    
    // Count pieces for each side
    int white_count = count_bits(white_pieces);
    int black_count = count_bits(black_pieces);
    
    // King vs King
    if (white_count == 1 && black_count == 1) {
        return true;
    }
    
    // King and Knight/Bishop vs King
    if ((white_count == 2 && black_count == 1) || (white_count == 1 && black_count == 2)) {
        Bitboard knights = board.get_piece_bitboard(Board::KNIGHT, Board::WHITE) |
                          board.get_piece_bitboard(Board::KNIGHT, Board::BLACK);
        Bitboard bishops = board.get_piece_bitboard(Board::BISHOP, Board::WHITE) |
                          board.get_piece_bitboard(Board::BISHOP, Board::BLACK);
        
        if (count_bits(knights) == 1 || count_bits(bishops) == 1) {
            return true;
        }
    }
    
    return false;
}

void Search::order_moves(std::vector<Move>& moves, const Board& board) {
    // Simple move ordering: captures first, then quiet moves
    std::sort(moves.begin(), moves.end(), [this, &board](const Move& a, const Move& b) {
        return get_move_score(a, board) > get_move_score(b, board);
    });
}

int Search::get_move_score(const Move& move, const Board& board) const {
    int score = 0;
    
    // Prioritize captures (MVV-LVA: Most Valuable Victim - Least Valuable Attacker)
    if (move.captured_piece != '.') {
        char captured_piece = move.captured_piece;
        char moving_piece = move.piece;
        
        // Get piece values
        int victim_value = 0;
        int attacker_value = 0;
        
        switch (std::tolower(captured_piece)) {
            case 'p': victim_value = 100; break;
            case 'n': victim_value = 300; break;
            case 'b': victim_value = 300; break;
            case 'r': victim_value = 500; break;
            case 'q': victim_value = 900; break;
            case 'k': victim_value = 10000; break;
        }
        
        switch (std::tolower(moving_piece)) {
            case 'p': attacker_value = 100; break;
            case 'n': attacker_value = 300; break;
            case 'b': attacker_value = 300; break;
            case 'r': attacker_value = 500; break;
            case 'q': attacker_value = 900; break;
            case 'k': attacker_value = 10000; break;
        }
        
        score += victim_value - attacker_value / 10;
    }
    
    // Prioritize promotions
    if (move.promotion_piece != '.') {
        score += 800;
    }
    
    // Prioritize checks (if we can detect them easily)
    // This would require making the move and checking, which is expensive
    // So we skip this for now in the simple implementation
    
    return score;
}