#include "Search.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <future>

// Constructor
Search::Search() 
    : evaluator(std::make_unique<Evaluation>())
    , transposition_table()
    , stop_flag(false)
    , search_active(false)
    , search_start_time(std::chrono::steady_clock::now())
    , time_limit_ms(std::chrono::milliseconds(0))
    , time_limit_active(false)
    , thread_count(4)
    , thread_pool_active(false)
{
    // Initialize thread pool
    init_thread_pool();
}

// Destructor
Search::~Search() {
    // Stop any running search
    stop_search();
    
    // Shutdown thread pool
    shutdown_thread_pool();
    
    // No longer using separate time manager thread - time checking is integrated into search loops
}

// Initialize thread pool
void Search::init_thread_pool() {
    std::lock_guard<std::mutex> lock(search_mutex);
    thread_pool_active = true;
}

// Shutdown thread pool
void Search::shutdown_thread_pool() {
    std::lock_guard<std::mutex> lock(search_mutex);
    thread_pool_active = false;
    
    // Join all worker threads
    for (auto& thread : worker_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads.clear();
}

// Stop search operation
void Search::stop_search() {
    stop_flag.store(true, std::memory_order_release);
}

// Check if search should stop - includes time limit checking
bool Search::should_stop() const {
    // First check the manual stop flag
    if (stop_flag.load(std::memory_order_acquire)) {
        return true;
    }
    
    // Then check time limit if active
    if (time_limit_active.load(std::memory_order_acquire)) {
        auto current_time = std::chrono::steady_clock::now();
        auto start_time = search_start_time.load(std::memory_order_acquire);
        auto limit = time_limit_ms.load(std::memory_order_acquire);
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
        if (elapsed >= limit) {
            // Time limit exceeded - set stop flag for other threads
            const_cast<Search*>(this)->stop_flag.store(true, std::memory_order_release);
            return true;
        }
    }
    
    return false;
}

// Check if search is active
bool Search::is_searching() const {
    return search_active.load(std::memory_order_acquire);
}

// Set thread count
void Search::set_thread_count(int num_threads) {
    std::lock_guard<std::mutex> lock(search_mutex);
    thread_count = std::max(MIN_THREADS, std::min(MAX_THREADS, num_threads));
}

// Get thread count
int Search::get_thread_count() const {
    std::lock_guard<std::mutex> lock(search_mutex);
    return thread_count;
}

// Legacy time management worker - no longer used
// Time checking is now integrated directly into search loops via should_stop()
void Search::time_management_worker(std::chrono::milliseconds time_limit) {
    // This function is kept for compatibility but is no longer used
    // Time management is now handled by the global timer system in should_stop()
}

// Basic minimax with alpha-beta pruning and context support
int Search::minimax_with_context(SearchContext& context, int depth, int alpha, int beta, bool maximizing_player, int ply) {
    context.stats.nodes_searched++;
    
    // Check for time limit every 1000 nodes for responsiveness
    if ((context.stats.nodes_searched % 1000) == 0 && should_stop()) {
        return maximizing_player ? alpha : beta;
    }
    
    // Base case: depth reached or game over
    if (depth == 0) {
        return quiescence_search_with_context(context, alpha, beta, maximizing_player);
    }
    
    // Generate moves using instance method
    MoveList moves = context.move_generator.generate_legal_moves(context.board);
    
    // Check for checkmate or stalemate
    if (moves.empty()) {
        if (context.move_generator.is_in_check(context.board, context.board.get_active_color())) {
            // Checkmate - return mate score adjusted by ply
            return maximizing_player ? -MATE_SCORE + ply : MATE_SCORE - ply;
        } else {
            // Stalemate
            return DRAW_SCORE;
        }
    }
    
    // Order moves for better pruning
    order_moves_with_context(moves, context, ply);
    
    if (maximizing_player) {
        int max_eval = -INFINITY_SCORE;
        for (const Move& move : moves) {
            // Check for stop condition before each move
            if (should_stop()) {
                break;
            }
            
            // Make move and get undo data
            BitboardMoveUndoData undo_data = context.board.make_move(move);
            
            // Recursive search
            int eval = minimax_with_context(context, depth - 1, alpha, beta, false, ply + 1);
            
            // Unmake move using undo data
            context.board.undo_move(undo_data);
            
            max_eval = std::max(max_eval, eval);
            alpha = std::max(alpha, eval);
            
            // Beta cutoff
            if (beta <= alpha) {
                context.stats.beta_cutoffs++;
                break;
            }
        }
        return max_eval;
    } else {
        int min_eval = INFINITY_SCORE;
        for (const Move& move : moves) {
            // Check for stop condition before each move
            if (should_stop()) {
                break;
            }
            
            // Make move and get undo data
            BitboardMoveUndoData undo_data = context.board.make_move(move);
            
            // Recursive search
            int eval = minimax_with_context(context, depth - 1, alpha, beta, true, ply + 1);
            
            // Unmake move using undo data
            context.board.undo_move(undo_data);
            
            min_eval = std::min(min_eval, eval);
            beta = std::min(beta, eval);
            
            // Alpha cutoff
            if (beta <= alpha) {
                context.stats.beta_cutoffs++;
                break;
            }
        }
        return min_eval;
    }
}

// Quiescence search with context support
int Search::quiescence_search_with_context(SearchContext& context, int alpha, int beta, bool maximizing_player, int qs_depth) {
    // Limit quiescence depth to prevent infinite search
    if (qs_depth >= MAX_QUIESCENCE_DEPTH) {
        return evaluator->evaluate(context.board);
    }
    
    context.stats.quiescence_nodes++;
    
    // Check for time limit periodically in quiescence search
    if ((context.stats.quiescence_nodes % 1000) == 0 && should_stop()) {
        return maximizing_player ? alpha : beta;
    }
    
    // Stand pat evaluation
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
    
    // Generate only capture moves for quiescence using instance method
    MoveList captures = context.move_generator.generate_captures(context.board);
    
    for (const Move& move : captures) {
        // Check for stop condition
        if (should_stop()) {
            break;
        }
        
        // Make move and get undo data
        BitboardMoveUndoData undo_data = context.board.make_move(move);
        
        // Recursive quiescence search
        int eval = quiescence_search_with_context(context, alpha, beta, !maximizing_player, qs_depth + 1);
        
        // Unmake move using undo data
        context.board.undo_move(undo_data);
        
        if (maximizing_player) {
            alpha = std::max(alpha, eval);
            if (beta <= alpha) {
                break;
            }
        } else {
            beta = std::min(beta, eval);
            if (beta <= alpha) {
                break;
            }
        }
    }
    
    return maximizing_player ? alpha : beta;
}

// Iterative deepening search
void Search::iterative_deepening(Board& board, int max_depth, SearchResult& result) {
    SearchContext context(board);
    Move best_move;
    bool found_move = false;
    
    // Generate legal moves
    MoveList legal_moves = context.move_generator.generate_legal_moves(context.board);
    if (legal_moves.empty()) {
        return; // No legal moves
    }
    
    // Use first legal move as fallback
    best_move = legal_moves[0];
    found_move = true;
    
    // Initialize move scores
    std::vector<MoveScore> move_scores;
    move_scores.reserve(legal_moves.size());
    for (const Move& move : legal_moves) {
        move_scores.emplace_back(move);
    }
    
    // Iterative deepening loop
    for (int depth = 1; depth <= max_depth && !should_stop(); depth++) {
        // Reset evaluation flags for this depth
        for (auto& ms : move_scores) {
            ms.evaluated = false;
            ms.score = -INFINITY_SCORE;
        }
        
        // Parallelize move evaluation at root
        parallel_evaluate_moves(context.board, move_scores, depth);
        
        // Check if search was stopped
        if (should_stop()) {
            break;
        }
        
        // Find best move from evaluated moves
        auto best_it = std::max_element(move_scores.begin(), move_scores.end(),
            [](const MoveScore& a, const MoveScore& b) {
                if (!a.evaluated) return true;
                if (!b.evaluated) return false;
                return a.score < b.score;
            });
        
        if (best_it != move_scores.end() && best_it->evaluated) {
            best_move = best_it->move;
            result.best_move = best_move;
            result.score = best_it->score;
            result.depth = depth;
            found_move = true;
            
            // Sort moves for next iteration (best move first)
            std::sort(move_scores.begin(), move_scores.end(),
                [](const MoveScore& a, const MoveScore& b) {
                    if (!a.evaluated && !b.evaluated) return false;
                    if (!a.evaluated) return false;
                    if (!b.evaluated) return true;
                    return a.score > b.score;
                });
        }
    }
    
    // Ensure we return a valid move
    if (found_move) {
        result.best_move = best_move;
    }
    
    // Copy statistics
    result.stats = context.stats;
}

// Move ordering with context
void Search::order_moves_with_context(MoveList& moves, SearchContext& context, int ply, const Move& tt_move) {
    // Simple move ordering: captures first, then quiet moves
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        return evaluate_move_priority_with_context(a, context, ply, tt_move) > 
               evaluate_move_priority_with_context(b, context, ply, tt_move);
    });
}

// Evaluate move priority with context
int Search::evaluate_move_priority_with_context(const Move& move, const SearchContext& context, int ply, const Move& tt_move) {
    int priority = 0;
    
    // Transposition table move gets highest priority
    if (move == tt_move) {
        priority += 10000;
    }
    
    // Captures get high priority
    if (move.is_capture()) {
        priority += 1000;
        // MVV-LVA: Most Valuable Victim - Least Valuable Attacker
        priority += get_piece_value(move.captured_piece) - get_piece_value(move.piece);
    }
    
    // Promotions get high priority
    if (move.is_promotion()) {
        priority += 900;
    }
    
    // Killer moves get medium priority
    for (int i = 0; i < KILLER_MOVES_PER_PLY; i++) {
        if (ply < MAX_DEPTH && move == context.killer_moves[i][ply]) {
            priority += 500 - i * 100;
            break;
        }
    }
    
    return priority;
}

// Helper function to get piece value for move ordering
int Search::get_piece_value(char piece) const {
    switch (std::tolower(piece)) {
        case 'p': return 100;
        case 'n': return 300;
        case 'b': return 300;
        case 'r': return 500;
        case 'q': return 900;
        case 'k': return 10000;
        default: return 0;
    }
}

// Check if side has non-pawn material
bool Search::has_non_pawn_material(const Board& board, Board::Color color) const {
    // This is a simplified check - in a real implementation you'd check the bitboards
    return true; // Placeholder implementation
}

// MAIN SEARCH FUNCTIONS - THE 4 REQUIRED FUNCTIONS

// 1. Search move without time limit
Move Search::search_move(Board& board, int max_depth) {
    // Reset stop flag
    stop_flag.store(false, std::memory_order_release);
    search_active.store(true, std::memory_order_release);
    
    SearchResult result;
    result.best_move = Move(); // Invalid move initially
    
    try {
        iterative_deepening(board, max_depth, result);
    } catch (...) {
        // Ensure we always return a valid move
    }
    
    search_active.store(false, std::memory_order_release);
    
    // If no move found, return first legal move as fallback
    if (!result.best_move.is_valid()) {
        MoveGenerator gen;
        MoveList legal_moves = gen.generate_legal_moves(board);
        if (!legal_moves.empty()) {
            result.best_move = legal_moves[0];
        }
    }
    
    return result.best_move;
}

// 2. Search move with time limit - STRICT TIME ENFORCEMENT
Move Search::search_move(Board& board, std::chrono::milliseconds time_limit, int max_depth) {
    // Reset stop flag and set up global timer
    stop_flag.store(false, std::memory_order_release);
    search_active.store(true, std::memory_order_release);
    
    // Set up global timer system
    search_start_time.store(std::chrono::steady_clock::now(), std::memory_order_release);
    time_limit_ms.store(time_limit, std::memory_order_release);
    time_limit_active.store(true, std::memory_order_release);
    
    SearchResult result;
    result.best_move = Move(); // Invalid move initially
    
    // Get a fallback move immediately
    MoveGenerator gen;
    MoveList legal_moves = gen.generate_legal_moves(board);
    if (!legal_moves.empty()) {
        result.best_move = legal_moves[0]; // Fallback move
    }
    
    try {
        iterative_deepening(board, max_depth, result);
    } catch (...) {
        // Ensure we always return a valid move
    }
    
    // Disable global timer
    time_limit_active.store(false, std::memory_order_release);
    search_active.store(false, std::memory_order_release);
    
    // Calculate elapsed time
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - search_start_time.load());
    result.time_elapsed = elapsed;
    
    return result.best_move;
}

// 3. Search with full result without time limit
SearchResult Search::search(Board& board, int max_depth) {
    // Reset stop flag
    stop_flag.store(false, std::memory_order_release);
    search_active.store(true, std::memory_order_release);
    
    auto start_time = std::chrono::steady_clock::now();
    
    SearchResult result;
    result.best_move = Move(); // Invalid move initially
    
    try {
        iterative_deepening(board, max_depth, result);
    } catch (...) {
        // Ensure we always return a valid move
    }
    
    search_active.store(false, std::memory_order_release);
    
    // If no move found, return first legal move as fallback
    if (!result.best_move.is_valid()) {
        MoveGenerator gen;
        MoveList legal_moves = gen.generate_legal_moves(board);
        if (!legal_moves.empty()) {
            result.best_move = legal_moves[0];
        }
    }
    
    // Calculate elapsed time
    auto end_time = std::chrono::steady_clock::now();
    result.time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    return result;
}

// 4. Search with full result and time limit - STRICT TIME ENFORCEMENT
SearchResult Search::search(Board& board, std::chrono::milliseconds time_limit, int max_depth) {
    // Reset stop flag and set up global timer
    stop_flag.store(false, std::memory_order_release);
    search_active.store(true, std::memory_order_release);
    
    // Set up global timer system
    search_start_time.store(std::chrono::steady_clock::now(), std::memory_order_release);
    time_limit_ms.store(time_limit, std::memory_order_release);
    time_limit_active.store(true, std::memory_order_release);
    
    SearchResult result;
    result.best_move = Move(); // Invalid move initially
    
    // Get a fallback move immediately
    MoveGenerator gen;
    MoveList legal_moves = gen.generate_legal_moves(board);
    if (!legal_moves.empty()) {
        result.best_move = legal_moves[0]; // Fallback move
    }
    
    try {
        iterative_deepening(board, max_depth, result);
    } catch (...) {
        // Ensure we always return a valid move
    }
    
    // Disable global timer
    time_limit_active.store(false, std::memory_order_release);
    search_active.store(false, std::memory_order_release);
    
    // Calculate elapsed time
    auto end_time = std::chrono::steady_clock::now();
    result.time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - search_start_time.load());
    
    return result;
}

// Placeholder implementations for other declared functions

SearchResult Search::search_worker(Board& board, const MoveList& moves, int start_idx, int end_idx, int depth) {
    SearchResult result;
    // Placeholder implementation
    return result;
}

SearchResult Search::search_worker_with_window(Board& board, const MoveList& moves, int start_idx, int end_idx, int depth, int alpha, int beta) {
    SearchResult result;
    // Placeholder implementation
    return result;
}

SearchResult Search::parallel_root_worker(SearchContext& context, const MoveList& moves, int start_idx, int end_idx, int depth) {
    SearchResult result;
    // Placeholder implementation
    return result;
}

SearchResult Search::parallel_root_search(Board& board, const MoveList& moves, int depth) {
    SearchResult result;
    // Placeholder implementation
    return result;
}

SearchResult Search::parallel_root_search_with_window(Board& board, const MoveList& moves, int depth, int alpha, int beta) {
    SearchResult result;
    // Placeholder implementation
    return result;
}

int Search::minimax_incremental(Board& board, int depth, int alpha, int beta, bool maximizing_player, int ply) {
    // Placeholder implementation
    return 0;
}

// New function: Parallel move evaluation at root
void Search::parallel_evaluate_moves(Board& board, std::vector<MoveScore>& move_scores, int depth) {
    const int num_moves = static_cast<int>(move_scores.size());
    const int num_threads = std::min(thread_count, num_moves);
    
    if (num_threads <= 1 || num_moves == 1) {
        // Single-threaded fallback
        sequential_evaluate_moves(board, move_scores, depth);
        return;
    }
    
    // Shared mutex for accessing move_scores
    std::mutex scores_mutex;
    
    // Atomic counter for work distribution
    std::atomic<int> next_move_index(0);
    
    // Launch worker threads
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&, t]() {
            // Each thread gets its own board copy and context
            Board thread_board = board;
            SearchContext thread_context(thread_board);
            
            while (!should_stop()) {
                // Get next move to evaluate
                int move_idx = next_move_index.fetch_add(1, std::memory_order_relaxed);
                
                if (move_idx >= num_moves) {
                    break; // No more moves to evaluate
                }
                
                const Move& move = move_scores[move_idx].move;
                
                // Make move
                BitboardMoveUndoData undo_data = thread_board.make_move(move);
                
                // Evaluate this move
                int score = -minimax_with_context(thread_context, depth - 1, 
                                                  -INFINITY_SCORE, INFINITY_SCORE, 
                                                  false, 1);
                
                // Unmake move
                thread_board.undo_move(undo_data);
                
                // Store result (thread-safe)
                if (!should_stop()) {
                    std::lock_guard<std::mutex> lock(scores_mutex);
                    move_scores[move_idx].score = score;
                    move_scores[move_idx].evaluated = true;
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

// Sequential fallback for single-threaded evaluation
void Search::sequential_evaluate_moves(Board& board, std::vector<MoveScore>& move_scores, int depth) {
    SearchContext context(board);
    
    for (auto& ms : move_scores) {
        if (should_stop()) {
            break;
        }
        
        const Move& move = ms.move;
        
        // Make move
        BitboardMoveUndoData undo_data = context.board.make_move(move);
        
        // Evaluate this move
        int score = -minimax_with_context(context, depth - 1, 
                                          -INFINITY_SCORE, INFINITY_SCORE, 
                                          false, 1);
        
        // Unmake move
        context.board.undo_move(undo_data);
        
        // Store result
        ms.score = score;
        ms.evaluated = true;
    }
}

// Optional: Aspiration window search for better move ordering
void Search::parallel_evaluate_moves_with_aspiration(Board& board, 
                                                      std::vector<MoveScore>& move_scores, 
                                                      int depth, 
                                                      int prev_score) {
    // First, try with narrow aspiration window
    const int window = 50; // Centipawns
    int alpha = prev_score - window;
    int beta = prev_score + window;
    
    parallel_evaluate_moves_windowed(board, move_scores, depth, alpha, beta);
    
    // Check if any move failed high or low
    bool need_research = false;
    for (const auto& ms : move_scores) {
        if (ms.evaluated && (ms.score <= alpha || ms.score >= beta)) {
            need_research = true;
            break;
        }
    }
    
    // Re-search with full window if needed
    if (need_research && !should_stop()) {
        for (auto& ms : move_scores) {
            ms.evaluated = false;
        }
        parallel_evaluate_moves(board, move_scores, depth);
    }
}

// Parallel evaluation with alpha-beta window
void Search::parallel_evaluate_moves_windowed(Board& board, 
                                               std::vector<MoveScore>& move_scores, 
                                               int depth, 
                                               int alpha, 
                                               int beta) {
    const int num_moves = static_cast<int>(move_scores.size());
    const int num_threads = std::min(thread_count, num_moves);
    
    if (num_threads <= 1 || num_moves == 1) {
        sequential_evaluate_moves_windowed(board, move_scores, depth, alpha, beta);
        return;
    }
    
    std::mutex scores_mutex;
    std::atomic<int> next_move_index(0);
    std::atomic<int> shared_alpha(alpha);
    
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&]() {
            Board thread_board = board;
            SearchContext thread_context(thread_board);
            
            while (!should_stop()) {
                int move_idx = next_move_index.fetch_add(1, std::memory_order_relaxed);
                
                if (move_idx >= num_moves) {
                    break;
                }
                
                const Move& move = move_scores[move_idx].move;
                
                // Get current alpha value
                int current_alpha = shared_alpha.load(std::memory_order_relaxed);
                
                // Make move
                BitboardMoveUndoData undo_data = thread_board.make_move(move);
                
                // Evaluate with current window
                int score = -minimax_with_context(thread_context, depth - 1, 
                                                  -beta, -current_alpha, 
                                                  false, 1);
                
                // Unmake move
                thread_board.undo_move(undo_data);
                
                if (!should_stop()) {
                    std::lock_guard<std::mutex> lock(scores_mutex);
                    move_scores[move_idx].score = score;
                    move_scores[move_idx].evaluated = true;
                    
                    // Update shared alpha if we found a better move
                    if (score > current_alpha) {
                        int expected = current_alpha;
                        while (score > expected && 
                               !shared_alpha.compare_exchange_weak(expected, score, 
                                                                   std::memory_order_relaxed)) {
                            // Keep trying to update if another thread hasn't set a higher value
                        }
                    }
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

// Sequential windowed evaluation
void Search::sequential_evaluate_moves_windowed(Board& board, 
                                                 std::vector<MoveScore>& move_scores, 
                                                 int depth, 
                                                 int alpha, 
                                                 int beta) {
    SearchContext context(board);
    
    for (auto& ms : move_scores) {
        if (should_stop()) {
            break;
        }
        
        const Move& move = ms.move;
        
        BitboardMoveUndoData undo_data = context.board.make_move(move);
        
        int score = -minimax_with_context(context, depth - 1, -beta, -alpha, false, 1);
        
        context.board.undo_move(undo_data);
        
        ms.score = score;
        ms.evaluated = true;
        
        // Update alpha
        if (score > alpha) {
            alpha = score;
        }
    }
}

SearchResult Search::search_incremental(Board& board, int max_depth, bool debug_output) {
    SearchResult result;
    // Placeholder implementation
    return result;
}