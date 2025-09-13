#include "Search.h"
#include "../board/MoveGenerator.h"
#include <algorithm>
#include <future>
#include <random>
#include <cassert>

// Constructor
Search::Search() 
    : evaluator(std::make_unique<Evaluation>()),
      stop_flag(false),
      search_active(false),
      thread_count(4) {
    // Initialize killer moves table
    for (int i = 0; i < MAX_DEPTH; ++i) {
        for (int j = 0; j < KILLER_MOVES_PER_PLY; ++j) {
            killer_moves[i][j] = Move(); // Default constructor creates invalid move
        }
    }
    
    // Initialize history table
    for (int i = 0; i < 64; ++i) {
        for (int j = 0; j < 64; ++j) {
            history_table[i][j] = 0;
        }
    }
}

// Destructor
Search::~Search() {
    stop_search();
    
    // Join any active threads
    if (time_manager_thread.joinable()) {
        time_manager_thread.join();
    }
    
    for (auto& thread : worker_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

// Main search interface implementations

Move Search::search_move(Board& board, int max_depth) {
    SearchResult result = search(board, max_depth);
    return result.best_move;
}

Move Search::search_move(Board& board, std::chrono::milliseconds time_limit, int max_depth) {
    SearchResult result = search(board, time_limit, max_depth);
    return result.best_move;
}

SearchResult Search::search(Board& board, int max_depth) {
    SearchResult result;
    auto start_time = std::chrono::steady_clock::now();
    
    // Reset search state
    stop_flag.store(false);
    search_active.store(true);
    result.stats.clear();
    
    // Get legal moves to ensure we have at least one move to return
    MoveGenerator move_gen;
    try {
        MoveList legal_moves = move_gen.generate_legal_moves(board);
        if (legal_moves.empty()) {
            // No legal moves (checkmate or stalemate)
            auto end_time = std::chrono::steady_clock::now();
            result.time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            return result;
        }
        result.best_move = legal_moves[0]; // Default to first legal move
    } catch (...) {
        // Error generating moves
        auto end_time = std::chrono::steady_clock::now();
        result.time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        return result;
    }
    
    try {
        iterative_deepening(board, max_depth, result);
    } catch (...) {
        // Ensure we always have a valid move
        if (!result.best_move.is_valid()) {
            MoveGenerator move_gen;
            MoveList legal_moves = move_gen.generate_legal_moves(board);
            if (!legal_moves.empty()) {
                result.best_move = legal_moves[0];
            }
        }
    }
    
    auto end_time = std::chrono::steady_clock::now();
    result.time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    search_active.store(false);
    
    return result;
}

SearchResult Search::search(Board& board, std::chrono::milliseconds time_limit, int max_depth) {
    SearchResult result;
    auto start_time = std::chrono::steady_clock::now();
    
    // Reset search state
    stop_flag.store(false);
    search_active.store(true);
    result.stats.clear();
    
    // Get legal moves to ensure we have at least one move to return
    MoveGenerator move_gen;
    try {
        MoveList legal_moves = move_gen.generate_legal_moves(board);
        if (legal_moves.empty()) {
            // No legal moves (checkmate or stalemate)
            auto end_time = std::chrono::steady_clock::now();
            result.time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            return result;
        }
        result.best_move = legal_moves[0]; // Default to first legal move
    } catch (...) {
        // Error generating moves
        auto end_time = std::chrono::steady_clock::now();
        result.time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        return result;
    }
    
    // Start time management thread
    time_manager_thread = std::thread(&Search::time_management_worker, this, time_limit);
    
    try {
        iterative_deepening(board, max_depth, result);
    } catch (...) {
        // Ensure we always have a valid move
        if (!result.best_move.is_valid()) {
            MoveGenerator move_gen;
            MoveList legal_moves = move_gen.generate_legal_moves(board);
            if (!legal_moves.empty()) {
                result.best_move = legal_moves[0];
            }
        }
    }
    
    // Stop search and join time manager
    stop_flag.store(true);
    if (time_manager_thread.joinable()) {
        time_manager_thread.join();
    }
    
    auto end_time = std::chrono::steady_clock::now();
    result.time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    search_active.store(false);
    
    return result;
}

// Control functions

void Search::stop_search() {
    stop_flag.store(true);
}

bool Search::is_searching() const {
    return search_active.load();
}

void Search::set_thread_count(int num_threads) {
    std::lock_guard<std::mutex> lock(search_mutex);
    thread_count = std::max(MIN_THREADS, std::min(MAX_THREADS, num_threads));
}

int Search::get_thread_count() const {
    std::lock_guard<std::mutex> lock(search_mutex);
    return thread_count;
}

// Core search implementation

void Search::iterative_deepening(Board& board, int max_depth, SearchResult& result) {
    MoveGenerator move_gen;
    MoveList legal_moves = move_gen.generate_legal_moves(board);
    
    if (legal_moves.empty()) {
        // No legal moves - checkmate or stalemate
        result.score = 0; // Assume stalemate for now
        return;
    }
    
    // Initialize with first legal move
    result.best_move = legal_moves[0];
    result.depth = 0;
    
    // Order moves once for all depths
    order_moves(legal_moves, board);
    
    // Iterative deepening loop
    for (int depth = 1; depth <= max_depth && !should_stop(); ++depth) {
        SearchResult depth_result = search_worker(board, legal_moves, 0, legal_moves.size(), depth);
        
        // Update result if depth completed successfully
        if (!should_stop() && depth_result.best_move.is_valid()) {
            result.best_move = depth_result.best_move;
            result.score = depth_result.score;
            result.depth = depth;
            result.sel_depth = std::max(result.sel_depth, depth);
            
            // Accumulate statistics
            result.stats.nodes_searched += depth_result.stats.nodes_searched;
            result.stats.quiescence_nodes += depth_result.stats.quiescence_nodes;
            result.stats.tt_hits += depth_result.stats.tt_hits;
            result.stats.beta_cutoffs += depth_result.stats.beta_cutoffs;
            result.stats.null_move_cutoffs += depth_result.stats.null_move_cutoffs;
            result.stats.lmr_reductions += depth_result.stats.lmr_reductions;
        }
        
        // Check for mate
        if (std::abs(result.score) >= MATE_SCORE - 100) {
            break;
        }
    }
}

int Search::minimax(Board& board, int depth, int alpha, int beta, bool maximizing_player, SearchStats& stats, int ply) {
    stats.nodes_searched++;
    
    // Check stop condition at safe points
    if (stats.nodes_searched % 1000 == 0 && should_stop()) {
        return 0;
    }
    
    // Prevent stack overflow by limiting maximum ply depth
    if (ply >= MAX_DEPTH) {
        return evaluator->evaluate(board);
    }
    
    // Terminal node - evaluate position
    if (depth == 0) {
        return quiescence_search(board, alpha, beta, maximizing_player, stats, 0);
    }
    
    MoveGenerator move_gen;
    MoveList moves = move_gen.generate_legal_moves(board);
    
    if (moves.empty()) {
        // No legal moves - checkmate or stalemate
        if (board.is_in_check(board.get_active_color())) {
            // Checkmate: return mate score adjusted by depth
            // Closer mates are better/worse depending on perspective
            return maximizing_player ? (-MATE_SCORE + depth) : (MATE_SCORE - depth);
        } else {
            // Stalemate: draw
            return 0;
        }
    }
    
    // Order moves for better pruning
    order_moves(moves, board, ply);
    
    if (maximizing_player) {
        int max_eval = -INFINITY_SCORE;
        
        for (const Move& move : moves) {
            if (should_stop()) break;
            
            // Make move and get undo data
            BitboardMoveUndoData undo_data = board.apply_move(move);
            
            int eval = minimax(board, depth - 1, alpha, beta, false, stats, ply + 1);
            
            // Undo move
            board.undo_move(undo_data);
            max_eval = std::max(max_eval, eval);
            alpha = std::max(alpha, eval);
            
            if (beta <= alpha) {
                stats.beta_cutoffs++;
                // Store killer move if it's not a capture
                if (ply < MAX_DEPTH && !move.is_capture()) {
                    std::lock_guard<std::mutex> lock(killer_history_mutex);
                    // Shift killer moves and add new one
                    killer_moves[ply][1] = killer_moves[ply][0];
                    killer_moves[ply][0] = move;
                }
                // Update history heuristic
                int from_square = move.from_rank * 8 + move.from_file;
                int to_square = move.to_rank * 8 + move.to_file;
                if (from_square < 64 && to_square < 64) {
                    std::lock_guard<std::mutex> lock(killer_history_mutex);
                    history_table[from_square][to_square] += depth * depth;
                }
                break; // Beta cutoff
            }
        }
        
        return max_eval;
    } else {
        int min_eval = INFINITY_SCORE;
        
        for (const Move& move : moves) {
            if (should_stop()) break;
            
            // Make move and get undo data
            BitboardMoveUndoData undo_data = board.apply_move(move);
            
            int eval = minimax(board, depth - 1, alpha, beta, true, stats, ply + 1);
            
            // Undo move
            board.undo_move(undo_data);
            min_eval = std::min(min_eval, eval);
            beta = std::min(beta, eval);
            
            if (beta <= alpha) {
                stats.beta_cutoffs++;
                // Store killer move if it's not a capture
                if (ply < MAX_DEPTH && !move.is_capture()) {
                    std::lock_guard<std::mutex> lock(killer_history_mutex);
                    // Shift killer moves and add new one
                    killer_moves[ply][1] = killer_moves[ply][0];
                    killer_moves[ply][0] = move;
                }
                // Update history heuristic
                int from_square = move.from_rank * 8 + move.from_file;
                int to_square = move.to_rank * 8 + move.to_file;
                if (from_square < 64 && to_square < 64) {
                    std::lock_guard<std::mutex> lock(killer_history_mutex);
                    history_table[from_square][to_square] += depth * depth;
                }
                break; // Alpha cutoff
            }
        }
        
        return min_eval;
    }
}

int Search::quiescence_search(Board& board, int alpha, int beta, bool maximizing_player, SearchStats& stats, int qs_depth) {
    stats.quiescence_nodes++;
    
    // Check stop condition
    if (should_stop()) {
        return 0;
    }
    
    // Limit quiescence search depth to prevent explosion
    if (qs_depth >= MAX_QUIESCENCE_DEPTH) {
        return evaluator->evaluate(board);
    }
    
    // Stand pat evaluation
    int stand_pat = evaluator->evaluate(board);
    
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
    
    MoveGenerator move_gen;
    MoveList moves = move_gen.generate_legal_moves(board);
    
    // Filter for captures and promotions only
    MoveList tactical_moves;
    for (const Move& move : moves) {
        if (move.captured_piece != '.' || move.promotion_piece != '.') {
            // For promotions, only consider queen promotions in quiescence
            if (move.promotion_piece != '.' && 
                Board::char_to_piece_type(move.promotion_piece) != Board::QUEEN) {
                continue;
            }
            tactical_moves.push_back(move);
        }
    }
    
    // Order captures by MVV-LVA
    order_moves(tactical_moves, board);
    
    if (maximizing_player) {
        int max_eval = stand_pat;
        
        for (const Move& move : tactical_moves) {
            if (should_stop()) break;
            
            // Make move and get undo data
            BitboardMoveUndoData undo_data = board.apply_move(move);
            
            int eval = quiescence_search(board, alpha, beta, false, stats, qs_depth + 1);
            
            // Undo move
            board.undo_move(undo_data);
            max_eval = std::max(max_eval, eval);
            alpha = std::max(alpha, eval);
            
            if (beta <= alpha) {
                break; // Beta cutoff
            }
        }
        
        return max_eval;
    } else {
        int min_eval = stand_pat;
        
        for (const Move& move : tactical_moves) {
            if (should_stop()) break;
            
            // Make move and get undo data
            BitboardMoveUndoData undo_data = board.apply_move(move);
            
            int eval = quiescence_search(board, alpha, beta, true, stats, qs_depth + 1);
            
            // Undo move
            board.undo_move(undo_data);
            min_eval = std::min(min_eval, eval);
            beta = std::min(beta, eval);
            
            if (beta <= alpha) {
                break; // Alpha cutoff
            }
        }
        
        return min_eval;
    }
}

SearchResult Search::search_worker(Board& board, const MoveList& moves, int start_idx, int end_idx, int depth) {
    SearchResult result;
    result.score = -INFINITY_SCORE;
    
    for (int i = start_idx; i < end_idx && i < moves.size(); ++i) {
        if (should_stop()) break;
        
        const Move& move = moves[i];
        
        // Make move and get undo data
        BitboardMoveUndoData undo_data = board.apply_move(move);
        
        int score = minimax(board, depth - 1, -INFINITY_SCORE, INFINITY_SCORE, false, result.stats, 0);
        
        // Undo move
        board.undo_move(undo_data);
        
        if (score > result.score) {
            result.best_move = move;
            result.score = score;
        }
    }
    
    return result;
}

void Search::time_management_worker(std::chrono::milliseconds time_limit) {
    auto start_time = std::chrono::steady_clock::now();
    
    while (!should_stop()) {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
        
        if (elapsed >= time_limit) {
            stop_flag.store(true);
            break;
        }
        
        // Check every 10ms
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool Search::should_stop() const {
    return stop_flag.load();
}

void Search::order_moves(MoveList& moves, Board& board, int ply) {
    // Enhanced move ordering: captures, killer moves, history heuristic
    std::sort(moves.begin(), moves.end(), [this, &board, ply](const Move& a, const Move& b) {
        return evaluate_move_priority(a, board, ply) > evaluate_move_priority(b, board, ply);
    });
}

int Search::evaluate_move_priority(const Move& move, Board& board, int ply) {
    int priority = 0;
    
    // Prioritize captures
    if (move.is_capture()) {
        priority += 1000;
        
        // MVV-LVA (Most Valuable Victim - Least Valuable Attacker)
        char victim = move.captured_piece;
        char attacker = move.piece;
        
        // Simple piece values for ordering
        auto get_piece_value = [](char piece) {
            switch (std::tolower(piece)) {
                case 'p': return 100;
                case 'n': return 300;
                case 'b': return 300;
                case 'r': return 500;
                case 'q': return 900;
                case 'k': return 10000;
                default: return 0;
            }
        };
        
        priority += get_piece_value(victim) - get_piece_value(attacker) / 10;
    }
    
    // Prioritize promotions
    if (move.is_promotion()) {
        priority += 800;
    }
    
    // Prioritize castling
    if (move.is_castling) {
        priority += 50;
    }
    
    // Check killer moves (only for non-captures)
    if (!move.is_capture() && ply < MAX_DEPTH) {
        std::lock_guard<std::mutex> lock(killer_history_mutex);
        if (move == killer_moves[ply][0]) {
            priority += 500;  // First killer move
        } else if (move == killer_moves[ply][1]) {
            priority += 400;  // Second killer move
        }
    }
    
    // History heuristic (for non-captures)
    if (!move.is_capture()) {
        int from_square = move.from_rank * 8 + move.from_file;
        int to_square = move.to_rank * 8 + move.to_file;
        if (from_square < 64 && to_square < 64) {
            std::lock_guard<std::mutex> lock(killer_history_mutex);
            priority += history_table[from_square][to_square] / 100;  // Scale down history score
        }
    }
    
    // Add some randomness to avoid deterministic behavior
    // Temporarily disabled for debugging
    // static std::random_device rd;
    // static std::mt19937 gen(rd());
    // static std::uniform_int_distribution<> dis(0, 10);
    // priority += dis(gen);
    
    return priority;
}