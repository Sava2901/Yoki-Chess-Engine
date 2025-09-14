#include "Search.h"
#include "../board/MoveGenerator.h"
#include <algorithm>
#include <future>
#include <random>
#include <cassert>

// TODO: Timed functions still does not work
// TODO: Depth above 3 freezes the search

// Constructor
Search::Search() 
    : evaluator(std::make_unique<Evaluation>()),
      stop_flag(false),
      search_active(false),
      thread_count(4),
      thread_pool_active(false) {
    // Constructor now only initializes basic members
    // Killer moves and history tables are now per-thread in SearchContext
}

// Destructor
Search::~Search() {
    stop_search();
    shutdown_thread_pool();
    
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
    thread_local MoveGenerator move_gen;
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
            thread_local MoveGenerator move_gen;
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
    thread_local MoveGenerator move_gen;
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
            thread_local MoveGenerator move_gen;
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
    // Use thread-local MoveGenerator to avoid repeated instantiation
    thread_local MoveGenerator move_gen;
    MoveList legal_moves = move_gen.generate_legal_moves(board);
    
    if (legal_moves.empty()) {
        // No legal moves - checkmate or stalemate
        result.score = 0; // Assume stalemate for now
        return;
    }
    
    // Initialize with first legal move
    result.best_move = legal_moves[0];
    result.depth = 0;
    
    // Initialize thread pool
    init_thread_pool();
    
    // Iterative deepening loop with aspiration windows and PVS
    int aspiration_window = 50; // Initial aspiration window size
    int prev_score = 0;
    Move pv_move; // Best move from previous iteration
    
    for (int depth = 1; depth <= max_depth && !should_stop(); ++depth) {
        SearchResult depth_result;
        
        // Order moves for this depth, using PV move from previous iteration
        SearchContext temp_context(board);
        order_moves_with_context(legal_moves, temp_context, 0, pv_move);
        
        // Set aspiration window bounds
        int alpha, beta;
        if (depth == 1) {
            // Full window for first depth
            alpha = -INFINITY_SCORE;
            beta = INFINITY_SCORE;
        } else {
            // Narrow window around previous score
            alpha = prev_score - aspiration_window;
            beta = prev_score + aspiration_window;
        }
        
        bool search_failed = false;
        int search_attempts = 0;
        const int max_attempts = 4;
        
        do {
            search_failed = false;
            search_attempts++;
            
            if (thread_count <= 1 || legal_moves.size() < 2) {
            // Single-threaded search with aspiration window
            depth_result = search_worker_with_window(board, legal_moves, 0, legal_moves.size(), depth, alpha, beta);
        } else {
            // Parallel root search with aspiration window
            depth_result = parallel_root_search_with_window(board, legal_moves, depth, alpha, beta);
        }
            
            // Check if search failed high or low
            if (!should_stop() && depth_result.best_move.is_valid()) {
                if (depth_result.score <= alpha) {
                    // Failed low - widen alpha
                    alpha = -INFINITY_SCORE;
                    search_failed = true;
                } else if (depth_result.score >= beta) {
                    // Failed high - widen beta
                    beta = INFINITY_SCORE;
                    search_failed = true;
                }
            }
            
            // Prevent infinite loop
            if (search_attempts >= max_attempts) {
                search_failed = false;
            }
            
        } while (search_failed && !should_stop());
        
        // Update result if depth completed successfully
        if (!should_stop() && depth_result.best_move.is_valid()) {
            result.best_move = depth_result.best_move;
            result.score = depth_result.score;
            result.depth = depth;
            result.sel_depth = std::max(result.sel_depth, depth);
            prev_score = depth_result.score;
            pv_move = depth_result.best_move; // Store PV move for next iteration
            
            // Adjust aspiration window for next iteration
            if (search_attempts == 1) {
                // Search succeeded on first try - can narrow window
                aspiration_window = std::max(25, aspiration_window - 5);
            } else {
                // Search failed - widen window for next time
                aspiration_window = std::min(200, aspiration_window + 25);
            }
            
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

// Old minimax function removed - using minimax_with_context instead

// Old quiescence_search function removed - using quiescence_search_with_context instead

SearchResult Search::search_worker(Board& board, const MoveList& moves, int start_idx, int end_idx, int depth) {
    SearchResult result;
    result.score = -INFINITY_SCORE;
    
    // Create SearchContext for this worker
    SearchContext context(board);
    
    for (int i = start_idx; i < end_idx && i < moves.size(); ++i) {
        if (should_stop()) break;
        
        const Move& move = moves[i];
        
        // Make move and get undo data
        BitboardMoveUndoData undo_data = context.board.apply_move(move);
        
        int score = minimax_with_context(context, depth - 1, -INFINITY_SCORE, INFINITY_SCORE, false, 0);
        
        // Undo move
        context.board.undo_move(undo_data);
        
        if (score > result.score) {
            result.best_move = move;
            result.score = score;
        }
    }
    
    return result;
}

// Search worker with aspiration window and PVS
SearchResult Search::search_worker_with_window(Board& board, const MoveList& moves, int start_idx, int end_idx, int depth, int alpha, int beta) {
    SearchResult result;
    result.score = alpha;
    
    // Create SearchContext for this worker
    SearchContext context(board);
    
    bool first_move = true;
    
    for (int i = start_idx; i < end_idx && i < moves.size(); ++i) {
        if (should_stop()) break;
        
        const Move& move = moves[i];
        
        // Make move and get undo data
        BitboardMoveUndoData undo_data = context.board.apply_move(move);
        
        int score;
        
        if (first_move) {
            // Search first move (PV) with full window
            score = minimax_with_context(context, depth - 1, -beta, -alpha, false, 0);
            first_move = false;
        } else {
            // Search remaining moves with null window (PVS)
            score = minimax_with_context(context, depth - 1, -alpha - 1, -alpha, false, 0);
            
            // If null window search beats alpha, re-search with full window
            if (score > alpha && score < beta) {
                score = minimax_with_context(context, depth - 1, -beta, -score, false, 0);
            }
        }
        
        score = -score; // Negate because we're at root (maximizing)
        
        // Undo move
        context.board.undo_move(undo_data);
        
        if (score > result.score) {
            result.best_move = move;
            result.score = score;
            alpha = std::max(alpha, score);
        }
        
        // Beta cutoff at root
        if (score >= beta) {
            break;
        }
    }
    
    // Merge per-thread stats into result
    result.stats = context.stats;
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

// Old order_moves and evaluate_move_priority functions removed - using context-based versions instead

// Parallel root search implementation

SearchResult Search::parallel_root_search(Board& board, const MoveList& moves, int depth) {
    SearchResult best_result;
    best_result.score = -INFINITY_SCORE;
    
    // Calculate moves per thread
    int num_threads = std::min(thread_count, static_cast<int>(moves.size()));
    int moves_per_thread = moves.size() / num_threads;
    int remaining_moves = moves.size() % num_threads;
    
    // Clear previous futures
    search_futures.clear();
    search_futures.reserve(num_threads);
    
    // Launch worker threads
    int start_idx = 0;
    for (int i = 0; i < num_threads; ++i) {
        int end_idx = start_idx + moves_per_thread;
        if (i < remaining_moves) {
            end_idx++; // Distribute remaining moves to first threads
        }
        
        // Launch async task with board copy (SearchContext created inside worker)
        search_futures.emplace_back(
            std::async(std::launch::async, 
                      [this, board, &moves, start_idx, end_idx, depth]() {
                          SearchContext context(board);
                          return parallel_root_worker(context, moves, start_idx, end_idx, depth);
                      })
        );
        
        start_idx = end_idx;
    }
    
    // Collect results from all threads
    SearchStats combined_stats;
    for (auto& future : search_futures) {
        if (should_stop()) break;
        
        try {
            SearchResult thread_result = future.get();
            
            // Update best move if this thread found a better one
            if (thread_result.best_move.is_valid() && thread_result.score > best_result.score) {
                best_result.best_move = thread_result.best_move;
                best_result.score = thread_result.score;
            }
            
            // Accumulate statistics
            combined_stats.nodes_searched += thread_result.stats.nodes_searched;
            combined_stats.quiescence_nodes += thread_result.stats.quiescence_nodes;
            combined_stats.tt_hits += thread_result.stats.tt_hits;
            combined_stats.beta_cutoffs += thread_result.stats.beta_cutoffs;
            combined_stats.null_move_cutoffs += thread_result.stats.null_move_cutoffs;
            combined_stats.lmr_reductions += thread_result.stats.lmr_reductions;
            
        } catch (const std::exception& e) {
            // Handle thread exceptions gracefully
            continue;
        }
    }
    
    best_result.stats = combined_stats;
    best_result.depth = depth;
    
    return best_result;
}

// Parallel root search with aspiration window and PVS
SearchResult Search::parallel_root_search_with_window(Board& board, const MoveList& moves, int depth, int alpha, int beta) {
    SearchResult best_result;
    best_result.score = alpha;
    
    // Calculate moves per thread
    int num_threads = std::min(thread_count, static_cast<int>(moves.size()));
    int moves_per_thread = moves.size() / num_threads;
    int remaining_moves = moves.size() % num_threads;
    
    // Clear previous futures
    search_futures.clear();
    search_futures.reserve(num_threads);
    
    // Launch worker threads
    int start_idx = 0;
    for (int i = 0; i < num_threads; ++i) {
        int end_idx = start_idx + moves_per_thread;
        if (i < remaining_moves) {
            end_idx++; // Distribute remaining moves to first threads
        }
        
        // Launch async task with board copy (SearchContext created inside worker)
        search_futures.emplace_back(
            std::async(std::launch::async, 
                      [this, &board, &moves, start_idx, end_idx, depth, alpha, beta]() {
                          Board board_copy = board; // Make a copy for thread safety // FIX: work around unncecessary coppies
                          return search_worker_with_window(board_copy, moves, start_idx, end_idx, depth, alpha, beta);
                      })
        );
        
        start_idx = end_idx;
    }
    
    // Collect results from all threads
    SearchStats combined_stats;
    for (auto& future : search_futures) {
        if (should_stop()) break;
        
        try {
            SearchResult thread_result = future.get();
            
            // Update best move if this thread found a better one
            if (thread_result.best_move.is_valid() && thread_result.score > best_result.score) {
                best_result.best_move = thread_result.best_move;
                best_result.score = thread_result.score;
            }
            
            // Accumulate statistics
            combined_stats.nodes_searched += thread_result.stats.nodes_searched;
            combined_stats.quiescence_nodes += thread_result.stats.quiescence_nodes;
            combined_stats.tt_hits += thread_result.stats.tt_hits;
            combined_stats.beta_cutoffs += thread_result.stats.beta_cutoffs;
            combined_stats.null_move_cutoffs += thread_result.stats.null_move_cutoffs;
            combined_stats.lmr_reductions += thread_result.stats.lmr_reductions;
            
        } catch (const std::exception& e) {
            // Handle thread exceptions gracefully
            continue;
        }
    }
    
    best_result.stats = combined_stats;
    best_result.depth = depth;
    
    return best_result;
}

// Thread pool management methods

void Search::init_thread_pool() {
    if (thread_pool_active.load()) {
        return; // Already initialized
    }
    
    thread_pool_active.store(true);
    // Thread pool is created on-demand during search
}

void Search::shutdown_thread_pool() {
    thread_pool_active.store(false);
    
    // Wait for any pending futures to complete
    for (auto& future : search_futures) {
        if (future.valid()) {
            future.wait();
        }
    }
    search_futures.clear();
}

SearchResult Search::parallel_root_worker(SearchContext& context, const MoveList& moves, int start_idx, int end_idx, int depth) {
    SearchResult result;
    result.score = -INFINITY_SCORE;
    
    for (int i = start_idx; i < end_idx && i < moves.size(); ++i) {
        if (should_stop()) break;
        
        const Move& move = moves[i];
        
        // Make move on the thread's private board copy
        BitboardMoveUndoData undo_data = context.board.apply_move(move);
        
        // Use thread-local minimax with per-thread data
        int score = minimax_with_context(context, depth - 1, -INFINITY_SCORE, INFINITY_SCORE, false, 0);
        
        // Undo move
        context.board.undo_move(undo_data);
        
        if (score > result.score) {
            result.best_move = move;
            result.score = score;
        }
        
        // Update per-thread statistics (already updated in minimax_with_context)
    }
    
    // Merge per-thread stats into result
    result.stats = context.stats;
    return result;
}

// Helper function to check if position has non-pawn material
bool Search::has_non_pawn_material(const Board& board, Board::Color color) const {
    return (board.get_piece_bitboard(Board::KNIGHT, color) |
            board.get_piece_bitboard(Board::BISHOP, color) |
            board.get_piece_bitboard(Board::ROOK, color) |
            board.get_piece_bitboard(Board::QUEEN, color)) != 0;
}

// Helper minimax function that uses SearchContext instead of global data
int Search::minimax_with_context(SearchContext& context, int depth, int alpha, int beta, bool maximizing_player, int ply) {
    if (should_stop() || depth <= 0) {
        if (depth <= 0) {
            return quiescence_search_with_context(context, alpha, beta, maximizing_player, 0);
        }
        return evaluator->evaluate(context.board);
    }
    
    context.stats.nodes_searched++;
    
    // Check for mate distance pruning
    if (ply > 0) {
        alpha = std::max(alpha, -MATE_SCORE + ply);
        beta = std::min(beta, MATE_SCORE - ply - 1);
        if (alpha >= beta) {
            return alpha;
        }
    }
    
    // Use MoveGenerator from SearchContext to avoid repeated instantiation
    Board::Color active_color = context.board.get_active_color();
    bool in_check = context.move_generator.is_in_check(context.board, active_color);
    
    // Null-move pruning
    if (!in_check && ply > 0 && depth >= 3 && 
        beta < MATE_SCORE - 1000 && beta > -MATE_SCORE + 1000 &&
        has_non_pawn_material(context.board, active_color)) {
        
        // Make null move
        context.board.set_active_color(active_color == Board::WHITE ? Board::BLACK : Board::WHITE);
        
        // Search with reduced depth (R=2)
        int null_score = -minimax_with_context(context, depth - 1 - 2, -beta, -beta + 1, !maximizing_player, ply + 1);
        
        // Undo null move
        context.board.set_active_color(active_color);
        
        if (null_score >= beta) {
            context.stats.null_move_cutoffs++;
            return beta;
        }
    }
    
    // Transposition table probe (shared TT)
    uint64_t zobrist_key = context.board.get_zobrist_hash();
    TTEntry tt_entry;
    Move tt_move;
    
    const TTEntry* tt_result = transposition_table.probe(zobrist_key, depth, alpha, beta, ply);
    if (tt_result != nullptr) {
        tt_entry = *tt_result;
        if (tt_entry.depth >= depth) {
            int tt_score = tt_entry.score;
            
            // Adjust mate scores
            if (tt_score > MATE_SCORE - 1000) {
                tt_score -= ply;
            } else if (tt_score < -MATE_SCORE + 1000) {
                tt_score += ply;
            }
            
            switch (tt_entry.get_type()) {
                case TTEntryType::EXACT:
                    return tt_score;
                case TTEntryType::LOWER_BOUND:
                    alpha = std::max(alpha, tt_score);
                    break;
                case TTEntryType::UPPER_BOUND:
                    beta = std::min(beta, tt_score);
                    break;
            }
            
            if (alpha >= beta) {
                return tt_score;
            }
        }
        
        // Extract TT move for move ordering
        tt_move.from_uint32(tt_entry.move);
    }
    
    // Generate moves using context's MoveGenerator
    MoveList legal_moves = context.move_generator.generate_legal_moves(context.board);
    
    if (legal_moves.empty()) {
        // No legal moves - checkmate or stalemate
        if (context.move_generator.is_in_check(context.board, context.board.get_active_color())) {
            return -MATE_SCORE + ply; // Checkmate
        } else {
            return DRAW_SCORE; // Stalemate
        }
    }
    
    // Order moves using per-thread data
    order_moves_with_context(legal_moves, context, ply, tt_move);
    
    Move best_move;
    int best_score;
    TTEntryType entry_type;
    
    if (maximizing_player) {
        best_score = -INFINITY_SCORE;
        int move_count = 0;
        
        for (const Move& move : legal_moves) {
            if (should_stop()) break;
            
            move_count++;
            BitboardMoveUndoData undo_data = context.board.apply_move(move);
            
            int eval;
            
            // Late Move Reductions (LMR)
            if (move_count > 4 && depth >= 3 && !move.is_capture() && 
                !move.is_promotion() && !context.move_generator.is_in_check(context.board, context.board.get_active_color()) &&
                move != context.killer_moves[0][ply] && move != context.killer_moves[1][ply]) {
                
                // Calculate reduction
                int reduction = 1 + (depth > 6 ? 1 : 0) + (move_count > 6 ? 1 : 0);
                context.stats.lmr_reductions++;
                
                // Search with reduced depth
                eval = minimax_with_context(context, depth - 1 - reduction, alpha, beta, false, ply + 1);
                
                // If reduced search beats alpha, re-search with full depth
                if (eval > alpha) {
                    eval = minimax_with_context(context, depth - 1, alpha, beta, false, ply + 1);
                }
            } else {
                // Normal full-depth search
                eval = minimax_with_context(context, depth - 1, alpha, beta, false, ply + 1);
            }
            
            context.board.undo_move(undo_data);
            
            if (eval > best_score) {
                best_score = eval;
                best_move = move;
            }
            
            alpha = std::max(alpha, eval);
            
            if (beta <= alpha) {
                // Beta cutoff - update per-thread killer moves and history
                if (!move.is_capture() && ply < MAX_DEPTH) {
                    // Update killer moves
                    if (context.killer_moves[1][ply] != move) {
                        context.killer_moves[0][ply] = context.killer_moves[1][ply];
                        context.killer_moves[1][ply] = move;
                    }
                    
                    // Update history heuristic
                    int from_square = move.from_rank * 8 + move.from_file;
                    int to_square = move.to_rank * 8 + move.to_file;
                    if (from_square < 64 && to_square < 64) {
                        context.history_table[context.board.get_active_color()][from_square][to_square] += depth * depth;
                    }
                }
                break;
            }
        }
        
        entry_type = (best_score <= alpha) ? TTEntryType::UPPER_BOUND : 
                    (best_score >= beta) ? TTEntryType::LOWER_BOUND : TTEntryType::EXACT;
    } else {
        best_score = INFINITY_SCORE;
        int move_count = 0;
        
        for (const Move& move : legal_moves) {
            if (should_stop()) break;
            
            move_count++;
            BitboardMoveUndoData undo_data = context.board.apply_move(move);
            
            int eval;
            
            // Late Move Reductions (LMR)
            if (move_count > 4 && depth >= 3 && !move.is_capture() && 
                !move.is_promotion() && !context.move_generator.is_in_check(context.board, context.board.get_active_color()) &&
                move != context.killer_moves[0][ply] && move != context.killer_moves[1][ply]) {
                
                // Calculate reduction
                int reduction = 1 + (depth > 6 ? 1 : 0) + (move_count > 6 ? 1 : 0);
                context.stats.lmr_reductions++;
                
                // Search with reduced depth
                eval = minimax_with_context(context, depth - 1 - reduction, alpha, beta, true, ply + 1);
                
                // If reduced search beats beta, re-search with full depth
                if (eval < beta) {
                    eval = minimax_with_context(context, depth - 1, alpha, beta, true, ply + 1);
                }
            } else {
                // Normal full-depth search
                eval = minimax_with_context(context, depth - 1, alpha, beta, true, ply + 1);
            }
            
            context.board.undo_move(undo_data);
            
            if (eval < best_score) {
                best_score = eval;
                best_move = move;
            }
            
            beta = std::min(beta, eval);
            
            if (beta <= alpha) {
                // Alpha cutoff - update per-thread killer moves and history
                if (!move.is_capture() && ply < MAX_DEPTH) {
                    // Update killer moves
                    if (context.killer_moves[1][ply] != move) {
                        context.killer_moves[0][ply] = context.killer_moves[1][ply];
                        context.killer_moves[1][ply] = move;
                    }
                    
                    // Update history heuristic
                    int from_square = move.from_rank * 8 + move.from_file;
                    int to_square = move.to_rank * 8 + move.to_file;
                    if (from_square < 64 && to_square < 64) {
                        context.history_table[context.board.get_active_color()][from_square][to_square] += depth * depth;
                    }
                }
                break;
            }
        }
        
        entry_type = (best_score >= beta) ? TTEntryType::LOWER_BOUND : 
                    (best_score <= alpha) ? TTEntryType::UPPER_BOUND : TTEntryType::EXACT;
    }
    
    // Adjust mate scores for storage
    int store_score = best_score;
    if (store_score > MATE_SCORE - 1000) {
        store_score += ply;
    } else if (store_score < -MATE_SCORE + 1000) {
        store_score -= ply;
    }
    
    // Store in transposition table
    transposition_table.store(zobrist_key, depth, store_score, entry_type, best_move.to_uint32(), ply);
    
    return best_score;
}

// Helper quiescence search with context
int Search::quiescence_search_with_context(SearchContext& context, int alpha, int beta, bool maximizing_player, int qs_depth) {
    if (should_stop() || qs_depth >= MAX_QUIESCENCE_DEPTH) {
        return evaluator->evaluate(context.board);
    }
    
    context.stats.quiescence_nodes++;
    
    context.stats.nodes_searched++;
    
    int stand_pat = evaluator->evaluate(context.board);
    
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
    
    // Generate tactical moves (captures, promotions) efficiently using context's MoveGenerator
    MoveList tactical_moves = context.move_generator.generate_tactical_moves(context.board);
    
    if (tactical_moves.empty()) {
        return stand_pat;
    }
    
    // Order tactical moves
    order_moves_with_context(tactical_moves, context, qs_depth);
    
    if (maximizing_player) {
        int max_eval = stand_pat;
        
        for (const Move& move : tactical_moves) {
            if (should_stop()) break;
            
            BitboardMoveUndoData undo_data = context.board.apply_move(move);
            int eval = quiescence_search_with_context(context, alpha, beta, false, qs_depth + 1);
            context.board.undo_move(undo_data);
            
            max_eval = std::max(max_eval, eval);
            alpha = std::max(alpha, eval);
            
            if (beta <= alpha) {
                break;
            }
        }
        
        return max_eval;
    }
    int min_eval = stand_pat;
        
    for (const Move& move : tactical_moves) {
        if (should_stop()) break;
            
        BitboardMoveUndoData undo_data = context.board.apply_move(move);
        int eval = quiescence_search_with_context(context, alpha, beta, true, qs_depth + 1);
        context.board.undo_move(undo_data);
            
        min_eval = std::min(min_eval, eval);
        beta = std::min(beta, eval);
            
        if (beta <= alpha) {
            break;
        }
    }
        
    return min_eval;
}

// Helper struct for move scoring
struct ScoredMove {
    Move move;
    int score;
};

// Helper move ordering with context
void Search::order_moves_with_context(MoveList& moves, SearchContext& context, int ply, const Move& tt_move) {
    if (moves.empty()) return;
    
    // For small move lists, use insertion sort for better performance
    if (moves.size() < 20) {
        // Score each move once and use insertion sort
        std::vector<ScoredMove> scored_moves;
        scored_moves.reserve(moves.size());
        
        for (const Move& move : moves) {
            scored_moves.push_back({move, evaluate_move_priority_with_context(move, context, ply, tt_move)});
        }
        
        // Insertion sort by score (descending)
        for (size_t i = 1; i < scored_moves.size(); ++i) {
            ScoredMove key = scored_moves[i];
            int j = static_cast<int>(i) - 1;
            
            while (j >= 0 && scored_moves[j].score < key.score) {
                scored_moves[j + 1] = scored_moves[j];
                j--;
            }
            scored_moves[j + 1] = key;
        }
        
        // Rewrite original moves from sorted scored moves
        for (size_t i = 0; i < moves.size(); ++i) {
            moves[i] = scored_moves[i].move;
        }
    } else {
        // For larger move lists, score once and use std::sort
        std::vector<ScoredMove> scored_moves;
        scored_moves.reserve(moves.size());
        
        for (const Move& move : moves) {
            scored_moves.push_back({move, evaluate_move_priority_with_context(move, context, ply, tt_move)});
        }
        
        // Sort by score (descending)
        std::sort(scored_moves.begin(), scored_moves.end(), [](const ScoredMove& a, const ScoredMove& b) {
            return a.score > b.score;
        });
        
        // Rewrite original moves from sorted scored moves
        for (size_t i = 0; i < moves.size(); ++i) {
            moves[i] = scored_moves[i].move;
        }
    }
}

int Search::evaluate_move_priority_with_context(const Move& move, const SearchContext& context, int ply, const Move& tt_move) {
    int priority = 0;
    
    // Highest priority: TT move
    if (tt_move.is_valid() && move == tt_move) {
        priority += 10000;
    }
    
    // Prioritize captures
    if (move.is_capture()) {
        priority += 1000;
        
        // MVV-LVA
        char victim = move.captured_piece;
        char attacker = move.piece;
        
        auto get_piece_value = [](char piece) {
            switch (std::tolower(piece)) {
                case 'p': return 100;
                case 'n': return 295;
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
    
    // Check per-thread killer moves
    if (!move.is_capture() && ply < MAX_DEPTH) {
        if (move == context.killer_moves[0][ply]) {
            priority += 500;
        } else if (move == context.killer_moves[1][ply]) {
            priority += 400;
        }
    }
    
    // Per-thread history heuristic
    if (!move.is_capture()) {
        int from_square = move.from_rank * 8 + move.from_file;
        int to_square = move.to_rank * 8 + move.to_file;
        if (from_square < 64 && to_square < 64) {
            priority += context.history_table[context.board.get_active_color()][from_square][to_square] / 100;
        }
    }
    
    return priority;
}