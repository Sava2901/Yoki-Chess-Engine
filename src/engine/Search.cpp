#include "Search.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <future>

// Constructor
Search::Search() 
    : evaluator(std::make_unique<Evaluation>())
    , transposition_table(&g_transposition_table)
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

// Basic minimax with alpha-beta pruning and context support
int Search::minimax_with_context(SearchContext& context, int depth, int alpha, int beta, bool maximizing_player, int ply) {
    context.stats.nodes_searched++;
    
    // Check for time limit every 1000 nodes for responsiveness
    if ((context.stats.nodes_searched % 1000) == 0 && should_stop()) {
        return maximizing_player ? alpha : beta;
    }
    
    // Get Zobrist hash for current position
    uint64_t zobrist_hash = context.board.get_zobrist_hash();
    
    // Probe transposition table
    TTResult tt_result = transposition_table->probe(zobrist_hash, depth, alpha, beta, ply);
    Move tt_move;
    
    if (tt_result.found) {
        context.stats.tt_hits++;
        tt_move = tt_result.move;
        
        // Use TT result if we can cutoff
        if (tt_result.can_cutoff) {
            return tt_result.score;
        }
    }
    
    // Base case: depth reached or game over
    if (depth == 0) {
        int leaf_score = quiescence_search_with_context(context, alpha, beta, maximizing_player);
        
        // Store leaf evaluation in transposition table
        Move empty_move; // No best move for leaf positions
        TTBoundType leaf_bound = TTBoundType::EXACT; // Leaf evaluations are exact
        transposition_table->store(zobrist_hash, empty_move, leaf_score, 0, ply, leaf_bound);
        
        return leaf_score;
    }
    
    // Generate moves using instance method
    MoveList moves = context.move_generator.generate_legal_moves(context.board);
    
    // Check for checkmate or stalemate
    if (moves.empty()) {
        int terminal_score;
        if (context.move_generator.is_in_check(context.board, context.board.get_active_color())) {
            // Checkmate - return mate score adjusted by ply
            terminal_score = maximizing_player ? -MATE_SCORE + ply : MATE_SCORE - ply;
        } else {
            // Stalemate
            terminal_score = DRAW_SCORE;
        }
        
        // Store terminal position in transposition table
        Move empty_move; // No best move for terminal positions
        transposition_table->store(zobrist_hash, empty_move, terminal_score, depth, ply, TTBoundType::EXACT);
        return terminal_score;
    }
    
    // Order moves for better pruning (prioritize TT move)
    order_moves_with_context(moves, context, ply, tt_move);
    
    int best_score = maximizing_player ? -INFINITY_SCORE : INFINITY_SCORE;
    Move best_move;
    TTBoundType bound_type = TTBoundType::UPPER_BOUND; // default if we never improved
    
    if (maximizing_player) {
        bool did_cutoff = false;
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
            
            if (eval > best_score) {
                best_score = eval;
                best_move = move;
            }
            
            alpha = std::max(alpha, eval);
            
            // Beta cutoff
            if (beta <= alpha) {
                context.stats.beta_cutoffs++;
                did_cutoff = true;
                bound_type = TTBoundType::LOWER_BOUND; // beta cutoff => LOWER_BOUND
                break;
            }
        }
        if (!did_cutoff) bound_type = TTBoundType::EXACT;
        
    } else {
        bool did_cutoff = false;
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
            
            if (eval < best_score) {
                best_score = eval;
                best_move = move;
            }
            
            beta = std::min(beta, eval);
            
            // Alpha cutoff
            if (beta <= alpha) {
                context.stats.beta_cutoffs++;
                did_cutoff = true;
                bound_type = TTBoundType::UPPER_BOUND; // alpha cutoff in minimizing => UPPER_BOUND
                break;
            }
        }
        if (!did_cutoff) bound_type = TTBoundType::EXACT;
    }
    
    // Store in transposition table
    if (best_move.is_valid()) {
        transposition_table->store(zobrist_hash, best_move, best_score, depth, ply, bound_type);
    }
    
    return best_score;
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
    // Start new search - advance TT age
    transposition_table->new_search();
    
    SearchContext context(board);
    Move fallback_move;
    Move best_evaluated_move; // Track the best move that was actually evaluated
    bool has_fallback_move = false;
    bool has_evaluated_move = false;
    int best_evaluated_score = -INFINITY_SCORE;
    int best_completed_depth = 0;
    
    // Generate legal moves
    MoveList legal_moves = context.move_generator.generate_legal_moves(context.board);
    if (legal_moves.empty()) {
        return; // No legal moves
    }
    
    // Use first legal move as fallback
    fallback_move = legal_moves[0];
    has_fallback_move = true;
    
    // Initialize move scores
    std::vector<MoveScore> move_scores;
    move_scores.reserve(legal_moves.size());
    for (const Move& move : legal_moves) {
        move_scores.emplace_back(move);
    }
    
    // Initialize combined statistics
    SearchStats combined_stats;
    
    // Iterative deepening loop
    for (int depth = 1; depth <= max_depth && !should_stop(); depth++) {
        // Reset evaluation flags for this depth
        for (auto& ms : move_scores) {
            ms.evaluated = false;
            ms.score = -INFINITY_SCORE;
        }
        
        // Clear stats for this depth
        context.stats.clear();
        
        // Parallelize move evaluation at root
        SearchStats depth_stats;
        parallel_evaluate_moves(context.board, move_scores, depth, &depth_stats);
        
        // Debug output
        std::cout << "Depth " << depth
                  << ": nodes=" << depth_stats.nodes_searched
                  << ", tt_hits=" << depth_stats.tt_hits
                  << ", tt_hit_rate=" << (depth_stats.nodes_searched > 0 ?
                      (100.0 * depth_stats.tt_hits / depth_stats.nodes_searched) : 0.0)
                  << "%" << std::endl;
        
        // Accumulate statistics from this depth
        combined_stats.nodes_searched += depth_stats.nodes_searched;
        combined_stats.quiescence_nodes += depth_stats.quiescence_nodes;
        combined_stats.tt_hits += depth_stats.tt_hits;
        combined_stats.beta_cutoffs += depth_stats.beta_cutoffs;
        combined_stats.null_move_cutoffs += depth_stats.null_move_cutoffs;
        combined_stats.lmr_reductions += depth_stats.lmr_reductions;
        
        // Check if search was stopped during evaluation
        bool depth_completed = !should_stop();
        
        // Find best move from evaluated moves at this depth
        auto best_it = std::max_element(move_scores.begin(), move_scores.end(),
            [](const MoveScore& a, const MoveScore& b) {
                if (!a.evaluated) return true;
                if (!b.evaluated) return false;
                return a.score < b.score;
            });
        
        // If we have at least one evaluated move at this depth
        if (best_it != move_scores.end() && best_it->evaluated) {
            // If this depth completed successfully, update our best result
            if (depth_completed) {
                best_evaluated_move = best_it->move;
                best_evaluated_score = best_it->score;
                best_completed_depth = depth;
                has_evaluated_move = true;
                
                // Update result with completed depth
                result.best_move = best_evaluated_move;
                result.score = best_evaluated_score;
                result.depth = depth;
                
                // Debug output: Display best move for this depth
                // std::cout << "Depth " << depth << ": Best move = " 
                //          << best_evaluated_move.to_algebraic() 
                //          << " (score: " << best_evaluated_score << ")" << std::endl;
                
                // Sort moves for next iteration (best move first)
                std::sort(move_scores.begin(), move_scores.end(),
                    [](const MoveScore& a, const MoveScore& b) {
                        if (!a.evaluated && !b.evaluated) return false;
                        if (!a.evaluated) return false;
                        if (!b.evaluated) return true;
                        return a.score > b.score;
                    });
            }
            // If depth was interrupted but we have some evaluated moves,
            // we keep the previous best result and don't update it
        }
        
        // If search was stopped, break out of the loop
        if (should_stop()) {
            break;
        }
    }
    
    // Final move selection logic:
    // 1. If we have a completed evaluated move, use it (already set in result)
    // 2. If we don't have any evaluated moves, fall back to first legal move
    if (!has_evaluated_move && has_fallback_move) {
        result.best_move = fallback_move;
        result.score = 0; // Unknown score
        result.depth = 0; // No depth completed
    }
    
    // Copy accumulated statistics
    result.stats = combined_stats;
}

// Move ordering with context
void Search::order_moves_with_context(MoveList& moves, SearchContext& context, int ply, const Move& tt_move) {
    // Check time limit before expensive sorting operation
    if (should_stop()) {
        return;
    }
    
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

//TODO: Remove duplicate backup move in all the search functions (the backup already exists in iterative_deepening)

//TODO: Remove the undo move where possible (if each thread has its own board copy there is no need to undo)

//TODO: Investigate search at depth 1 and transposition table hits at depth 1

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
        // Clear transposition table even if search was interrupted
        transposition_table->clear();
    }
    
    search_active.store(false, std::memory_order_release);
    
    // Clear transposition table after search completion
    transposition_table->clear();
    
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
        // Clear transposition table even if search was interrupted
        transposition_table->clear();
    }
    
    // Disable global timer
    time_limit_active.store(false, std::memory_order_release);
    search_active.store(false, std::memory_order_release);
    
    // Clear transposition table after search completion
    transposition_table->clear();
    
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
        // Clear transposition table even if search was interrupted
        transposition_table->clear();
    }
    
    search_active.store(false, std::memory_order_release);
    
    // Clear transposition table after search completion
    transposition_table->clear();
    
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
        // Clear transposition table even if search was interrupted
        transposition_table->clear();
    }
    
    // Disable global timer
    time_limit_active.store(false, std::memory_order_release);
    search_active.store(false, std::memory_order_release);
    
    // Clear transposition table after search completion
    transposition_table->clear();
    
    // Calculate elapsed time
    auto end_time = std::chrono::steady_clock::now();
    result.time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - search_start_time.load());
    
    return result;
}

// TODO: Use the proper vector (SmallVector) instead of std::vector for better performance

// New function: Parallel move evaluation at root
void Search::parallel_evaluate_moves(Board& board, std::vector<MoveScore>& move_scores, int depth, SearchStats* out_stats) {
    const int num_moves = static_cast<int>(move_scores.size());
    const int num_threads = std::min(thread_count, num_moves);
    
    if (num_threads <= 1 || num_moves == 1) {
        // Single-threaded fallback
        sequential_evaluate_moves(board, move_scores, depth, out_stats);
        return;
    }
    
    // Shared mutex for accessing move_scores and accumulating stats
    std::mutex scores_mutex;
    
    // Atomic counter for work distribution
    std::atomic<int> next_move_index(0);
    
    // Shared statistics accumulator
    SearchStats combined_stats;
    
    // Launch worker threads
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&, t]() {
            // Each thread gets its own board copy and context
            Board thread_board = board;
            SearchContext thread_context(thread_board);
            
            // DON'T clear stats here - accumulate across all moves this thread evaluates
            
            while (!should_stop()) {
                // Check time limit before getting next move
                if (should_stop()) {
                    break;
                }
                
                // Get next move to evaluate
                int move_idx = next_move_index.fetch_add(1, std::memory_order_relaxed);
                
                if (move_idx >= num_moves) {
                    break; // No more moves to evaluate
                }
                
                // Check time limit before evaluating move
                if (should_stop()) {
                    break;
                }
                
                const Move& move = move_scores[move_idx].move;
                
                // DON'T clear stats for each move - we want cumulative stats
                // thread_context.stats.clear();  // <-- REMOVED THIS LINE
                
                // Make move
                BitboardMoveUndoData undo_data = thread_board.make_move(move);
                
                // Evaluate this move
                int score = -minimax_with_context(thread_context, depth - 1, 
                                                  -INFINITY_SCORE, INFINITY_SCORE, 
                                                  false, 1);
                
                // Unmake move
                thread_board.undo_move(undo_data);
                
                // Store result (thread-safe) - check time limit one more time
                if (!should_stop()) {
                    std::lock_guard<std::mutex> lock(scores_mutex);
                    move_scores[move_idx].score = score;
                    move_scores[move_idx].evaluated = true;
                }
            }
            
            // Accumulate this thread's total stats at the end
            {
                std::lock_guard<std::mutex> lock(scores_mutex);
                combined_stats.nodes_searched += thread_context.stats.nodes_searched;
                combined_stats.quiescence_nodes += thread_context.stats.quiescence_nodes;
                combined_stats.tt_hits += thread_context.stats.tt_hits;
                combined_stats.beta_cutoffs += thread_context.stats.beta_cutoffs;
                combined_stats.null_move_cutoffs += thread_context.stats.null_move_cutoffs;
                combined_stats.lmr_reductions += thread_context.stats.lmr_reductions;
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // Return accumulated statistics if requested
    if (out_stats) {
        *out_stats = combined_stats;
    }
}

//TODO: Investigate, this function seems to return different moves than the parallel version at the same exact depth

// Sequential fallback for single-threaded evaluation
void Search::sequential_evaluate_moves(Board& board, std::vector<MoveScore>& move_scores, int depth, SearchStats* out_stats) {
    SearchContext context(board);
    SearchStats accumulated_stats;
    
    for (auto& ms : move_scores) {
        if (should_stop()) {
            break;
        }
        
        const Move& move = ms.move;
        
        // DON'T clear stats for each move
        // context.stats.clear();  // <-- REMOVED THIS LINE
        
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
    
    // Accumulate all stats at once
    if (out_stats) {
        *out_stats = context.stats;
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
        // Check time limit before expensive re-search
        if (should_stop()) {
            return;
        }
        
        for (auto& ms : move_scores) {
            ms.evaluated = false;
        }
        SearchStats depth_stats;
        parallel_evaluate_moves(board, move_scores, depth, &depth_stats);
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
                
                // Check time limit before evaluating move
                if (should_stop()) {
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