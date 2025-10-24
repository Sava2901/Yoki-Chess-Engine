#include "../../include/engine/Search.h"
#include <algorithm>
#include <random>
#include <iomanip>
#include <iostream>

// Material values for piece evaluation
static constexpr int MATERIAL_VALUES[6] = {
    100,  // Pawn
    320,  // Knight
    330,  // Bishop
    500,  // Rook
    900,  // Queen
    20000 // King
};

// ThreadPool implementation

ThreadPool::ThreadPool(int num_threads) : stop_flag(false) {
    // Create persistent worker threads
    workers.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        workers.emplace_back([this] {
            // Worker thread loop - continuously process tasks
            while (true) {
                std::function<void()> task;
                
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    
                    // Wait for a task or stop signal
                    condition.wait(lock, [this] {
                        return stop_flag || !tasks.empty();
                    });
                    
                    // Exit if stop flag is set and queue is empty
                    if (stop_flag && tasks.empty()) {
                        return;
                    }
                    
                    // Get next task from queue
                    if (!tasks.empty()) {
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                }
                
                // Execute task outside of lock
                if (task) {
                    task();
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::stop() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop_flag = true;
    }
    condition.notify_all();
    
    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers.clear();
}

void ThreadPool::restart(int num_threads) {
    stop();
    stop_flag = false;
    
    // Recreate worker threads with new count
    workers.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        workers.emplace_back([this] {
            // Worker thread loop - continuously process tasks
            while (true) {
                std::function<void()> task;
                
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    
                    // Wait for a task or stop signal
                    condition.wait(lock, [this] {
                        return stop_flag || !tasks.empty();
                    });
                    
                    // Exit if stop flag is set and queue is empty
                    if (stop_flag && tasks.empty()) {
                        return;
                    }
                    
                    // Get next task from queue
                    if (!tasks.empty()) {
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                }
                
                // Execute task outside of lock
                if (task) {
                    task();
                }
            }
        });
    }
}

Search::Search() : Search(SearchConfig()) {}

Search::Search(const SearchConfig& config) : config(config) {
    // Initialize transposition table
    tt = std::make_unique<TranspositionTable>(config.tt_size_mb * 1024 * 1024);
    
    // Initialize evaluator and move generator
    evaluator = std::make_unique<Evaluation>();
    move_gen = std::make_unique<MoveGenerator>();
    
    // Initialize thread pool
    thread_pool = std::make_unique<ThreadPool>(config.thread_count);
    
    // Initialize PV table
    pv_table.resize(MAX_PLY);
    for (auto& pv : pv_table) {
        pv.reserve(MAX_PLY);
    }
    
    // Initialize previous moves array
    for (int i = 0; i < 64; ++i) {
        previous_moves[i] = Move();
    }
    
    // Clear tables
    clear_tables();
}

Search::~Search() {
    stop_search();
    stop_timer();  // Clean up timer thread
}

Move Search::search_move(const Board& board, int depth) {
    SearchResult result = search(board, depth);
    return result.best_move;
}

Move Search::search_move(const Board& board, std::chrono::milliseconds time_limit, int max_depth) {
    SearchResult result = search(board, time_limit, max_depth);
    return result.best_move;
}

SearchResult Search::search(const Board& board, int depth) {
    return iterative_deepening(board, depth, std::chrono::milliseconds(0));
}

SearchResult Search::search(const Board& board, std::chrono::milliseconds time_limit, int max_depth) {
    return iterative_deepening(board, max_depth, time_limit);
}

void Search::set_thread_count(int thread_count) {
    if (thread_count != config.thread_count) {
        config.thread_count = thread_count;
        thread_pool->restart(thread_count);
    }
}

int Search::get_thread_count() const {
    return config.thread_count;
}

void Search::stop_search() {
    stop_flag.store(true, std::memory_order_relaxed);
    search_cv.notify_all();
}

bool Search::should_stop() const {
    return stop_flag.load(std::memory_order_relaxed);
}

void Search::set_config(const SearchConfig& new_config) {
    config = new_config;
    
    // Update thread pool if needed
    if (thread_pool->get_thread_count() != config.thread_count) {
        thread_pool->restart(config.thread_count);
    }
    
    // Resize transposition table if needed
    size_t new_tt_size = config.tt_size_mb * 1024 * 1024;
    if (tt->get_size_mb() != new_tt_size) {
        tt = std::make_unique<TranspositionTable>(new_tt_size);
    }
}

void Search::clear_tables() {
    if (tt) tt->clear();
    killer_moves.clear();
    history_table.clear();
    counter_move_table.clear();
    current_stats.reset();
    
    // Clear previous moves tracking
    for (int i = 0; i < 64; ++i) {
        previous_moves[i] = Move();
    }
}

SearchResult Search::iterative_deepening(const Board& board, int max_depth, 
                                        std::chrono::milliseconds time_limit) {
    // Reset search state
    stop_flag.store(false, std::memory_order_relaxed);
    search_start_time = std::chrono::steady_clock::now();
    allocated_time = time_limit;
    current_stats.reset();
    
    // *** START TIMER THREAD ***
    start_timer(time_limit);
    
    SearchResult result;
    
    // Generate root moves first to ensure we have valid moves available
    Board mutable_board = board;
    MoveList legal_moves = move_gen->generate_legal_moves(mutable_board);

    if (legal_moves.empty()) {
        stop_timer();
        result.best_move = Move();
        return result;
    }
    
    // Always set a fallback valid move - use the first legal move
    result.best_move = legal_moves[0];
    result.score = evaluator->evaluate(board);
    result.depth = 0;
    
    // If only one legal move, return it immediately
    if (legal_moves.size() == 1) {
        stop_timer();
        result.depth = 1;
        return result;
    }
    
    int prev_score = 0;
    std::vector<Move> best_pv;
    
    // Iterative deepening loop
    try {
        // Check if we should use Lazy SMP (parallel iterative deepening)
        if (config.lazy_smp_instances > 1 && config.enable_parallel_search && config.thread_count > 1) {
            // Use Lazy SMP - all threads do iterative deepening together
            int score = lazy_smp_search(board, max_depth, config.lazy_smp_instances, best_pv);
            
            if (!best_pv.empty()) {
                result.best_move = best_pv[0];
                result.score = score;
                result.principal_variation = best_pv;
                // Depth is approximated since threads may reach different depths
                result.depth = max_depth;
            }
        } else {
            // Standard iterative deepening with optional parallel root search
            for (int depth = 1; depth <= max_depth; ++depth) {
                if (should_stop()) {
                    break;
                }

                std::vector<Move> current_pv;
                int score;
            
            // Choose search strategy based on configuration
            // Use parallel search at depth 6+ when multiple threads available (need sufficient work to overcome overhead)
            if (config.enable_parallel_search && config.thread_count > 1 && depth >= 6) {
                // Use parallel root search - improved to be more efficient
                score = parallel_root_search(const_cast<Board&>(board), depth, -MATE_SCORE, MATE_SCORE, current_pv);
            } else if (config.enable_aspiration_windows && depth >= 5 && prev_score != 0 && abs(prev_score) < MATE_SCORE - 100) {
                // Use aspiration windows for faster convergence (only at higher depths to avoid overhead)
                // Note: Aspiration windows disabled in parallel search to avoid complications
                score = aspiration_search(const_cast<Board&>(board), depth, prev_score, current_pv);
            } else {
                // Use regular alpha-beta search with full window
                score = alpha_beta(const_cast<Board&>(board), depth, -MATE_SCORE, MATE_SCORE, 0, current_pv);
            }
            
            if (!current_pv.empty()) {
                result.best_move = current_pv[0];
                result.score = score;
                result.depth = depth;
                result.principal_variation = current_pv;
                prev_score = score;
            }
            
            // Check for mate
            if (abs(score) > MATE_SCORE - 100) {
                break; // Found mate, no need to search deeper
            }
            
        }
        }
    } catch (const TimeUpException&) {
        // Time expired during search - gracefully return best move found so far
        std::cout << "TimeUpException caught in iterative_deepening - returning best move found" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception during search: " << e.what() << std::endl;
    }
    
    stop_timer();
    
    // Calculate final statistics
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - search_start_time);
    current_stats.time_elapsed_ms = static_cast<double>(elapsed.count());
    current_stats.calculate_derived_stats();
    
    result.stats = current_stats;
    result.time_expired = time_expired();
    result.search_stopped = stop_flag.load(std::memory_order_relaxed);
    
    // Fallback logic: if search was stopped and no move was found, use first legal move
    if (should_stop() && !result.best_move.is_valid()) {
        MoveList legal_moves = move_gen->generate_legal_moves(const_cast<Board&>(board));
        if (!legal_moves.empty()) {
            result.best_move = legal_moves[0];
        }
    }
    
    return result;
}

int Search::alpha_beta(Board& board, int depth, int alpha, int beta, int ply, std::vector<Move>& pv) {
    // Semantics:
    //  - `depth` is the *remaining* search depth (in plies) allowed from this node.
    //  - `ply` is the distance from the root (0 at root, increases by 1 going down).
    //  - `pv` is the principal variation returned for this node (best_move + child's PV).
    //
    // Notes: we save entry_alpha/entry_beta (the window used at node entry) for
    // transposition-table bound classification when storing.
    //
    // This function was refactored to avoid mutating the "entry window" when we decide
    // TT bound type later, and to always return a coherent PV (if any move exists).

    // *** CHECK STOP FLAG AT ENTRY ***
    if (should_stop()) {
        return evaluator->evaluate(board);
    }
    
    // Initialize PV as empty - don't clear if already has valid content
    // Only clear at the very end when we have the final best line
    
    // Check for stop conditions
    if (ply >= MAX_PLY) {
        current_stats.nodes_searched++;
        return evaluator->evaluate(board);
    }
    
    // Check for draw by repetition or 50-move rule
    if (board.get_halfmove_clock() >= 100) {
        return 0; // Draw
    }
    
    // Mate distance pruning
    int mate_alpha = std::max(alpha, -MATE_SCORE + ply);
    int mate_beta = std::min(beta, MATE_SCORE - ply - 1);
    if (mate_alpha >= mate_beta) {
        return mate_alpha;
    }
    alpha = mate_alpha;
    beta = mate_beta;

    // Save the entry window (after mate-distance adjust) for TT-bound classification later.
    int entry_alpha = alpha;
    int entry_beta  = beta;

    // Transposition table lookup (use remaining 'depth' as passed)
    Move tt_move;
    bool tt_hit = false;
    if (config.enable_transposition_table) {
        TTResult tt_result = tt->probe(board.get_zobrist_hash(), depth, alpha, beta, ply);
        if (tt_result.found) {
            current_stats.tt_hits++;
            tt_move = tt_result.move;
            tt_hit = true;
            
            if (tt_result.can_cutoff) {
                current_stats.tt_cutoffs++;
                return tt_result.score;
            }
        }
    }
    
    // Terminal node check (depth 0 or leaf)
    if (depth <= 0) {
        // Count this as a leaf node
        current_stats.nodes_searched++;
        
        // Clear PV for terminal nodes - no moves to add
        pv.clear();
        if (config.enable_quiescence_search) {
            std::vector<Move> qsearch_pv;
            int score = quiescence_search(board, alpha, beta, ply, qsearch_pv);
            // Copy quiescence PV to main PV
            pv = qsearch_pv;
            return score;
        }
        return evaluator->evaluate(board);
    }

    // Generate moves
    MoveList legal_moves = move_gen->generate_legal_moves(board);
    if (legal_moves.empty()) {
        // Count this as a leaf node (terminal position)
        current_stats.nodes_searched++;
        
        // Clear PV for terminal nodes (checkmate/stalemate)
        pv.clear();
        // Checkmate or stalemate
        if (move_gen->is_in_check(board, board.get_active_color())) {
            return -MATE_SCORE + ply; // Checkmate
        }
        return 0; // Stalemate
    }
    
    // Convert to MoveScore vector for ordering
    std::vector<MoveScore> moves;
    moves.reserve(legal_moves.size());
    for (const Move& move : legal_moves) {
        moves.emplace_back(move);
    }
    
    // Order moves
    order_moves(board, moves, ply, tt_move);
    
    // Static evaluation for pruning decisions
    bool in_check = move_gen->is_in_check(board, board.get_active_color());
    int static_eval = in_check ? -MATE_SCORE : evaluator->evaluate(board);
    bool is_pv = (beta - alpha > 1); // PV node if window is wider than 1
    
    // Reverse Futility Pruning (Static Null Move Pruning)
    // If our position is so good that even a null move would beat beta, prune
    if (depth <= 7 && !in_check && abs(beta) < MATE_SCORE - 100 && !is_pv) {
        int margin = 150 * depth; // More aggressive margin
        
        if (static_eval - margin >= beta) {
            current_stats.futility_prunes++;
            return static_eval - margin; // Return conservative estimate
        }
    }
    
    // Null move pruning - only if we're not too deep
    if (config.enable_null_move_pruning && can_do_null_move(board, depth, beta) && ply < MAX_PLY - 5) {
        Board null_board = board;
        null_board.set_active_color((null_board.get_active_color() == Board::WHITE) ? Board::BLACK : Board::WHITE);
        
        std::vector<Move> null_pv;
        int null_score = -alpha_beta(null_board, depth - config.null_move_reduction - 1,
                                     -beta, -beta + 1, ply + 1, null_pv);

        if (should_stop()) {
            return beta; // Return beta for null move cutoff
        }
        
        if (null_score >= beta) {
            current_stats.null_move_cutoffs++;
            return beta;
        }
    }
    
    // Razoring - drop into quiescence if position looks bad
    if (depth <= 3 && !move_gen->is_in_check(board, board.get_active_color()) && abs(alpha) < MATE_SCORE - 100) {
        int static_eval = evaluator->evaluate(board);
        int razor_margin = 300 + 200 * depth;
        
        if (static_eval + razor_margin < alpha) {
            // Position looks bad - do quiescence search
            std::vector<Move> qpv;
            int qscore = quiescence_search(board, alpha - razor_margin, alpha - razor_margin + 1, ply, qpv);
            if (qscore < alpha) {
                current_stats.futility_prunes++;
                return qscore;
            }
        }
    }
    
    // Main search loop
    int best_score = -MATE_SCORE;
    Move best_move;
    bool found_pv = false;
    int move_count = 0;
    
    // YBWC: Check if we should use parallel splitting for remaining moves
    // Criteria: depth >= min_split_depth, multiple threads available, not too many active splits
    bool can_split_ybwc = config.enable_parallel_search && 
                          config.thread_count > 1 &&
                          depth >= config.min_split_depth &&
                          moves.size() > 4 &&
                          active_split_points.load() < config.max_parallel_tasks;
    
    for (const MoveScore& move_score : moves) {
        if (should_stop()) {
            return best_score > -MATE_SCORE ? best_score : alpha;
        }
        
        const Move& move = move_score.move;
        
        // === FUTILITY PRUNING ===
        // Skip quiet moves in low-depth positions that can't raise alpha
        if (depth <= 3 && !is_pv && !in_check && move_count > 0 && 
            !move.is_capture() && !move.is_promotion()) {
            
            static constexpr int futility_margins[4] = {0, 200, 350, 500};
            int futility_value = static_eval + futility_margins[depth];
            
            if (futility_value <= alpha) {
                move_count++;
                continue; // Skip this move
            }
        }
        
        BitboardMoveUndoData undo_data = board.apply_move(move);
        move_count++;
        
        // Calculate extensions
        int extensions = 0;
        if (config.enable_check_extensions || config.enable_singular_extensions || config.enable_recapture_extensions) {
            if (ply < MAX_PLY - 10) {
                extensions = calculate_extensions(board, move, ply, 0);
            }
        }
        
        // === IMPROVED LATE MOVE REDUCTIONS ===
        int reduction = 0;
        if (config.enable_late_move_reductions && move_count > 3 &&
            depth >= 3 && !move.is_capture() && !move.is_promotion() && !in_check) {
            
            // Aggressive reduction
            reduction = 1 + (move_count / 4) + (depth / 4);
            
            // Reduce less for PV nodes
            if (is_pv) reduction = std::max(0, reduction - 1);
            
            // Don't reduce killer moves as much
            if (config.enable_killer_moves && killer_moves.is_killer(ply, move)) {
                reduction = std::max(0, reduction - 1);
            }
            
            // Clamp
            reduction = std::min(reduction, depth - 2);
        }
        
        int new_depth = depth - 1 + extensions - reduction;
        if (ply + 1 >= MAX_PLY) {
            new_depth = 0; // Force evaluation at next level
        }
        std::vector<Move> current_line;
        int score;
        
        // Principal Variation Search (PVS)
        // NOTE: At the root (ply == 0), always use full window to avoid wasteful re-searches
        if (move_count == 1 || ply == 0) {
            score = -alpha_beta(board, new_depth, -beta, -alpha, ply + 1, current_line);

            if (should_stop()) {
                board.undo_move(undo_data);
                return best_score > -MATE_SCORE ? best_score : entry_alpha;
            }
            
            // After first move, check if we should spawn helpers for remaining moves (YBWC)
            // Only split if first move didn't cause beta cutoff
            if (can_split_ybwc && score < beta && moves.size() > move_count) {
                // Increment split point count
                active_split_points.fetch_add(1);
                
                // Undo first move before parallel search
                board.undo_move(undo_data);
                
                // Update best from first move
                if (score > best_score) {
                    best_score = score;
                    best_move = move;
                }
                if (score > alpha) {
                    alpha = score;
                    found_pv = true;
                    pv.clear();
                    pv.push_back(move);
                    pv.insert(pv.end(), current_line.begin(), current_line.end());
                }
                
                // Launch parallel search for remaining moves
                std::mutex parallel_mutex;
                std::atomic<int> parallel_alpha(alpha);
                std::atomic<bool> helpers_should_stop(false); // Local stop flag for helpers
                std::vector<Move> parallel_best_pv;
                int parallel_best_score = best_score;
                Move parallel_best_move = best_move;
                
                // Split remaining moves among threads
                int remaining_count = static_cast<int>(moves.size()) - move_count;
                int num_helpers = std::min(config.thread_count - 1, remaining_count);
                int moves_per_helper = remaining_count / num_helpers;
                
                std::vector<std::future<void>> helper_futures;
                helper_futures.reserve(num_helpers);
                
                for (int helper_id = 0; helper_id < num_helpers; ++helper_id) {
                    int start_idx = move_count + helper_id * moves_per_helper;
                    int end_idx = (helper_id == num_helpers - 1) ? static_cast<int>(moves.size()) : start_idx + moves_per_helper;
                    
                    auto helper_task = [this, &board, depth, &parallel_alpha, beta, ply, &moves, start_idx, end_idx,
                                       &parallel_mutex, &parallel_best_score, &parallel_best_move, &parallel_best_pv, 
                                       new_depth, &helpers_should_stop]() {
                        Board helper_board = board; // Thread-local board copy
                        
                        for (int i = start_idx; i < end_idx && !should_stop() && !helpers_should_stop.load(); ++i) {
                            const Move& helper_move = moves[i].move;
                            BitboardMoveUndoData helper_undo = helper_board.apply_move(helper_move);
                            
                            std::vector<Move> helper_pv;
                            int helper_score = -alpha_beta(helper_board, new_depth, -beta, -parallel_alpha.load(), ply + 1, helper_pv);
                            
                            helper_board.undo_move(helper_undo);
                            
                            if (should_stop() || helpers_should_stop.load()) {
                                return;
                            }
                            
                            // Update shared best result
                            {
                                std::lock_guard<std::mutex> lock(parallel_mutex);
                                if (helper_score > parallel_best_score) {
                                    parallel_best_score = helper_score;
                                    parallel_best_move = helper_move;
                                    parallel_best_pv.clear();
                                    parallel_best_pv.push_back(helper_move);
                                    parallel_best_pv.insert(parallel_best_pv.end(), helper_pv.begin(), helper_pv.end());
                                    
                                    if (helper_score > parallel_alpha.load()) {
                                        parallel_alpha.store(helper_score);
                                    }
                                    
                                    // Beta cutoff - stop all helpers (but not entire search)
                                    if (helper_score >= beta) {
                                        helpers_should_stop.store(true);
                                        return;
                                    }
                                }
                            }
                        }
                    };
                    
                    helper_futures.push_back(thread_pool->submit(helper_task));
                }
                
                // Wait for all helpers to complete
                for (auto& future : helper_futures) {
                    try {
                        future.get();
                    } catch (const std::exception& e) {
                        std::cerr << "YBWC helper error: " << e.what() << std::endl;
                    }
                }
                
                // Decrement split point count
                active_split_points.fetch_sub(1);
                
                // Update with parallel results
                best_score = parallel_best_score;
                best_move = parallel_best_move;
                alpha = parallel_alpha.load();
                if (!parallel_best_pv.empty()) {
                    pv = parallel_best_pv;
                    found_pv = true;
                }
                
                // Skip to end of move loop since we processed all moves in parallel
                break;
            }
        } else {
            // Late moves - search with null window first
            score = -alpha_beta(board, new_depth, -alpha - 1, -alpha, ply + 1, current_line);

            if (should_stop()) {
                board.undo_move(undo_data);
                return best_score > -MATE_SCORE ? best_score : entry_alpha;
            }
            
            // If it beats alpha, re-search with full window
            if (score > alpha && score < beta) {
                current_line.clear();
                score = -alpha_beta(board, new_depth, -beta, -alpha, ply + 1, current_line);

                if (should_stop()) {
                    board.undo_move(undo_data);
                    return best_score > -MATE_SCORE ? best_score : entry_alpha;
                }
            }
        }
        
        // Undo move
        board.undo_move(undo_data);
        
        // Update best score and move
        if (score > best_score) {
            best_score = score;
            best_move = move;
        }

        // Update alpha and PV if we improved alpha (found new principal variation)
        if (score > alpha) {
            alpha = score;
            found_pv = true;
            current_stats.alpha_improvements++;
            
            // Construct PV: current move + child's PV
            pv.clear();
            pv.push_back(move);
            pv.insert(pv.end(), current_line.begin(), current_line.end());

            // Beta cutoff actions - this move was so good it caused a cutoff
            if (alpha >= beta) {
                current_stats.beta_cutoffs++;

                // Update move ordering heuristics for quiet moves
                if (!move.is_capture() && !move.is_promotion()) {
                    // Add to killer moves
                    if (config.enable_killer_moves) {
                        killer_moves.add_killer(ply, move);
                    }
                    
                    // Update history heuristic (reward this move)
                    if (config.enable_history_heuristic) {
                        history_table.update(move, depth);
                        
                        // Penalize moves that were tried before this one (failed to cause cutoff)
                        for (int i = 0; i < move_count - 1; ++i) {
                            const Move& failed_move = moves[i].move;
                            if (!failed_move.is_capture() && !failed_move.is_promotion()) {
                                history_table.penalize(failed_move, depth);
                            }
                        }
                    }
                    
                    // Update counter-move table (this move refuted opponent's last move)
                    if (config.enable_counter_moves && ply > 0) {
                        Move prev_move = previous_moves[ply - 1];
                        counter_move_table.update(prev_move, move);
                    }
                }

                // Store this move for counter-move tracking at next ply
                if (ply < MAX_PLY) {
                    previous_moves[ply] = move;
                }

                break; // beta cutoff
            }
        } else {
            // Move failed to improve alpha - penalize it slightly in history
            if (config.enable_history_heuristic && !move.is_capture() && !move.is_promotion()) {
                history_table.penalize(move, depth / 4); // Small penalty
            }
        }
    } // end moves loop

    // If no PV was set (no alpha improvement), but we have a best move, 
    // search it again to get a proper PV line
    if (!found_pv && best_move.from_rank != -1) {
        pv.clear();
        pv.push_back(best_move);
        
        // Try to get a deeper PV by searching the best move again
        if (depth > 1) {
            BitboardMoveUndoData undo_data = board.apply_move(best_move);
            std::vector<Move> deeper_pv;
            
            // Search with a null window to get the PV
            alpha_beta(board, depth - 1, -beta, -alpha, ply + 1, deeper_pv);
            
            board.undo_move(undo_data);
            
            // Append the deeper PV to our current PV
            pv.insert(pv.end(), deeper_pv.begin(), deeper_pv.end());
        }
    }

    // Store in transposition table using the *entry* window saved earlier
    if (config.enable_transposition_table && !should_stop()) {
        TTBoundType bound_type;
        if (best_score <= entry_alpha) {
            bound_type = TTBoundType::UPPER_BOUND;
        } else if (best_score >= entry_beta) {
            bound_type = TTBoundType::LOWER_BOUND;
        } else {
            bound_type = TTBoundType::EXACT;
        }
        
        tt->store(board.get_zobrist_hash(), best_move, best_score, depth, ply, bound_type);
    }
    
    return best_score;
}

// Timer implementation functions
void Search::timer_worker(std::chrono::milliseconds time_limit) {
    std::unique_lock<std::mutex> lock(timer_mutex);
    
    auto start_time = std::chrono::steady_clock::now();
    auto end_time = start_time + time_limit;
    
    // Display interval (every 500ms)
    const auto display_interval = std::chrono::milliseconds(500);
    auto next_display = start_time + display_interval;
    
    while (true) {
        auto now = std::chrono::steady_clock::now();
        
        // Check if we should exit early
        if (timer_should_exit.load(std::memory_order_relaxed)) {
            return; // Exit early
        }
        
        // Check if time has expired
        if (now >= end_time) {
            stop_flag.store(true, std::memory_order_relaxed);  // Stop the search
        }
        if (should_stop()) {
            std::cout << "Time expired!" << std::endl;
            return;
        }

        
        // Display remaining time if it's time to do so
        if (now >= next_display) {
            auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - now);
            double remaining_seconds = static_cast<double>(remaining.count()) / 1000.0;
            std::cout << "Time remaining: " << std::fixed << std::setprecision(1) 
                      << remaining_seconds << "s" << std::endl;
            next_display = now + display_interval;
        }
        
        // Wait for a short interval or until signaled to exit
        auto wait_until = std::min(next_display, end_time);
        auto wait_duration = std::chrono::duration_cast<std::chrono::milliseconds>(wait_until - now);

        if (wait_duration > std::chrono::milliseconds(0)) {
            bool exit_early = timer_cv.wait_for(lock, wait_duration, [this] {
                return timer_should_exit.load(std::memory_order_relaxed);
            });
            
            if (exit_early) {
                return; // Exit early
            }
        }
    }
}

void Search::start_timer(std::chrono::milliseconds time_limit) {
    if (time_limit.count() <= 0) {
        return; // No time limit, don't start timer
    }
    
    // Stop any existing timer first
    stop_timer();
    
    // Reset exit flag
    timer_should_exit = false;
    
    // Launch timer thread
    timer_thread = std::make_unique<std::thread>(&Search::timer_worker, this, time_limit);
}

void Search::stop_timer() {
    if (timer_thread && timer_thread->joinable()) {
        // Signal timer to exit
        {
            std::unique_lock<std::mutex> lock(timer_mutex);
            timer_should_exit = true;
        }
        timer_cv.notify_all();
        
        // Wait for timer thread to finish
        timer_thread->join();
        timer_thread.reset();
    }
}

int Search::quiescence_search(Board& board, int alpha, int beta, int ply, std::vector<Move>& pv) {
    // *** CHECK STOP FLAG AT ENTRY ***
    if (should_stop()) {
        pv.clear();
        return evaluator->evaluate(board);
    }
    
    current_stats.qnodes_searched++;
    // Use separate limit for quiescence search to prevent deep tactical searches
    if (ply >= MAX_PLY || ply >= MAX_QUIESCENCE_PLY) {
        pv.clear();
        return evaluator->evaluate(board);
    }
    
    // Initialize PV as empty
    pv.clear();
    
    // Stand pat evaluation
    int stand_pat = evaluator->evaluate(board);
    
    // Beta cutoff on stand pat
    if (stand_pat >= beta) {
        return beta;
    }
    
    // Update alpha
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }
    
    // Generate captures and checks
    MoveList captures;
    if (move_gen->is_in_check(board, board.get_active_color())) {
        // In check - must search all legal moves
        captures = move_gen->generate_legal_moves(board);
    } else {
        // Not in check - only search captures and promotions
        captures = move_gen->generate_captures(board);
        MoveList promotions = move_gen->generate_tactical_moves(board);
        captures.insert(captures.end(), promotions.begin(), promotions.end());
    }
    
    // Convert to MoveScore vector and order
    std::vector<MoveScore> moves;
    moves.reserve(captures.size());
    for (const Move& move : captures) {
        moves.emplace_back(move);
    }
    
    // Simple ordering for quiescence (MVV-LVA)
    std::sort(moves.begin(), moves.end(), [this](const MoveScore& a, const MoveScore& b) {
        return calculate_mvv_lva_score(a.move) > calculate_mvv_lva_score(b.move);
    });
    
    // Search captures
    for (const MoveScore& move_score : moves) {
        // *** PERIODIC STOP CHECK IN MOVE LOOP ***
        if (should_stop()) {
            return alpha;
        }
        
        const Move& move = move_score.move;
        
        // Delta pruning - skip obviously bad captures
        if (!move_gen->is_in_check(board, board.get_active_color()) && move.is_capture()) {
            Board::PieceType captured_type = Board::char_to_piece_type(move.captured_piece);
            int captured_value = MATERIAL_VALUES[captured_type];
            if (stand_pat + captured_value + 200 < alpha) {
                continue; // Delta pruning
            }
        }
        
        // SEE pruning - skip losing captures
        if (config.enable_see_ordering && move.is_capture()) {
            int see_score = calculate_see_score(board, move);
            if (see_score < 0) {
                continue; // Losing capture
            }
        }
        
        // Make move
        BitboardMoveUndoData undo_data = board.apply_move(move);
        
        // Recursive quiescence search
        std::vector<Move> child_pv;
        int score = -quiescence_search(board, -beta, -alpha, ply + 1, child_pv);
        
        // *** IMMEDIATE STOP CHECK AFTER RECURSIVE CALL ***
        if (should_stop()) {
            board.undo_move(undo_data);
            return alpha;
        }
        
        // Undo move
        board.undo_move(undo_data);
        
        // Update alpha and PV
        if (score > alpha) {
            alpha = score;
            
            // Construct PV: current move + child's PV
            pv.clear();
            pv.push_back(move);
            pv.insert(pv.end(), child_pv.begin(), child_pv.end());
            
            // Beta cutoff
            if (alpha >= beta) {
                return beta;
            }
        }
    }
    
    return alpha;
}

int Search::aspiration_search(Board& board, int depth, int prev_score, std::vector<Move>& pv) {
    // *** CHECK STOP FLAG AT ENTRY ***
    if (should_stop()) {
        return evaluator->evaluate(board);
    }
    
    int window = config.aspiration_window_size;
    int alpha = prev_score - window;
    int beta = prev_score + window;
    
    while (true) {
        // *** CHECK STOP FLAG IN LOOP ***
        if (should_stop()) {
            return evaluator->evaluate(board);
        }
        
        std::vector<Move> current_pv;
        int score = alpha_beta(board, depth, alpha, beta, 0, current_pv);
        
        // *** IMMEDIATE STOP CHECK AFTER RECURSIVE CALL ***
        if (should_stop()) {
            return evaluator->evaluate(board);
        }
        
        if (score <= alpha) {
            // Fail low - widen alpha
            alpha = std::max(alpha - window, -MATE_SCORE);
            window *= 2;
        } else if (score >= beta) {
            // Fail high - widen beta
            beta = std::min(beta + window, MATE_SCORE);
            window *= 2;
        } else {
            // Success - score is within window
            pv = current_pv;
            return score;
        }
        
        // Prevent infinite widening
        if (window > config.aspiration_window_max) {
            alpha = -MATE_SCORE;
            beta = MATE_SCORE;
        }
    }
}

void Search::order_moves(const Board& board, std::vector<MoveScore>& moves, 
                        int ply, const Move& tt_move) {
    // Get previous move for counter-move heuristic
    Move prev_move = (ply > 0) ? previous_moves[ply - 1] : Move();
    
    // Score all moves
    for (MoveScore& move_score : moves) {
        const Move& move = move_score.move;
        int score = 0;
        
        // Hash move gets highest priority (TT move is likely best)
        if (move == tt_move) {
            score = 10000000;
        }
        // Captures - evaluate winning/equal captures highly
        else if (move.is_capture()) {
            int mvv_lva = calculate_mvv_lva_score(move);
            score = 8000000 + mvv_lva;
            
            // SEE adjustment - good captures get bonus, bad ones get penalty
            if (config.enable_see_ordering) {
                int see_score = calculate_see_score(board, move);
                if (see_score < 0) {
                    // Losing capture - demote significantly
                    score = 2000000 + see_score;
                } else {
                    score += see_score * 10; // Amplify good captures
                }
            }
        }
        // Queen promotions are extremely good
        else if (move.is_promotion()) {
            Board::PieceType promotion_type = Board::char_to_piece_type(move.promotion_piece);
            if (promotion_type == Board::QUEEN) {
                score = 9000000; // Almost as good as hash move
            } else {
                score = 7000000 + static_cast<int>(promotion_type) * 10000;
            }
        }
        // Counter-move heuristic (response to opponent's last move)
        else if (config.enable_counter_moves && ply > 0 && 
                 counter_move_table.is_counter(prev_move, move)) {
            score = 6000000;
        }
        // Killer moves (non-capture moves that caused cutoffs at this ply)
        else if (config.enable_killer_moves && killer_moves.is_killer(ply, move)) {
            // First killer is better than second
            if (move == killer_moves.killers[ply][0]) {
                score = 5000000;
            } else {
                score = 4000000;
            }
        }
        // Quiet moves - use history heuristic and positional bonuses
        else {
            if (config.enable_history_heuristic) {
                score = history_table.get_score(move);
            }
            
            // Bonus for advancing pawns to 7th rank (likely promotion threats)
            char piece = move.piece;
            if (piece == 'P' && move.to_rank == 6) {
                score += 50000; // White pawn to 7th rank
            } else if (piece == 'p' && move.to_rank == 1) {
                score += 50000; // Black pawn to 7th rank
            }
            
            // Small bonus for central moves (e4, d4, e5, d5)
            if ((move.to_rank == 3 || move.to_rank == 4) && 
                (move.to_file == 3 || move.to_file == 4)) {
                score += 1000;
            }
        }
        
        move_score.score = score;
        move_score.evaluated = true;
    }
    
    // Sort moves by score (highest first)
    std::sort(moves.begin(), moves.end());
}

int Search::calculate_mvv_lva_score(const Move& move) const {
    if (!move.is_capture()) {
        return 0;
    }
    
    // MVV-LVA: Most Valuable Victim - Least Valuable Attacker
    // Higher score = better capture
    Board::PieceType victim_type = Board::char_to_piece_type(move.captured_piece);
    Board::PieceType attacker_type = Board::char_to_piece_type(move.piece);
    
    // Use fixed values optimized for move ordering (not exact material values)
    static constexpr int VICTIM_VALUES[6] = {
        100,    // PAWN
        320,    // KNIGHT
        330,    // BISHOP
        500,    // ROOK
        900,    // QUEEN
        10000   // KING (shouldn't happen but just in case)
    };
    
    static constexpr int ATTACKER_VALUES[6] = {
        1,   // PAWN (least valuable attacker is good)
        2,   // KNIGHT
        3,   // BISHOP
        4,   // ROOK
        5,   // QUEEN
        6    // KING (most valuable attacker is bad)
    };
    
    int victim_value = VICTIM_VALUES[victim_type];
    int attacker_value = ATTACKER_VALUES[attacker_type];
    
    // Score formula: prioritize victim value, then penalize attacker value
    // This ensures QxP > RxP > BxP > NxP > PxP for the same victim
    return victim_value * 10 - attacker_value;
}

int Search::calculate_see_score(const Board& board, const Move& move) const {
    // Simplified SEE (Static Exchange Evaluation) implementation
    // Estimates the material outcome of a capture sequence
    if (!move.is_capture()) {
        return 0;
    }
    
    Board::PieceType victim_type = Board::char_to_piece_type(move.captured_piece);
    Board::PieceType attacker_type = Board::char_to_piece_type(move.piece);
    
    // Material values for SEE calculation
    static constexpr int PIECE_VALUES[6] = {
        100,   // PAWN
        320,   // KNIGHT
        330,   // BISHOP
        500,   // ROOK
        900,   // QUEEN
        20000  // KING
    };
    
    int gain = PIECE_VALUES[victim_type];
    int risk = PIECE_VALUES[attacker_type];
    
    // Basic SEE heuristic:
    // - If we capture with equal/lower value piece: likely good (gain - risk/2)
    // - If we capture with higher value piece: might be risky (gain - risk)
    // - PxQ is great (+800), QxP is risky (-800)
    
    if (attacker_type <= victim_type) {
        // Good capture: equal or better exchange
        return gain - (risk / 4); // Small penalty for risk
    } else {
        // Risky capture: using more valuable piece to capture less valuable
        return gain - risk; // Full risk penalty
    }
}

int Search::calculate_extensions(const Board& board, const Move& move, int ply, int extensions_used) {
    if (extensions_used >= config.max_extensions_per_path) {
        return 0;
    }
    
    int extension = 0;
    
    // Check extension
    if (config.enable_check_extensions && move_gen->is_in_check(board, board.get_active_color())) {
        extension = 1;
        current_stats.check_extensions++;
    }
    // Recapture extension
    else if (config.enable_recapture_extensions && move.is_capture() && ply > 0) {
        // Simple recapture detection - same target square as previous move
        extension = 1;
        current_stats.recapture_extensions++;
    }
    
    return extension;
}

int Search::calculate_lmr_reduction(int depth, int move_count, const Move& move, bool is_pv_node) {
    if (move_count <= config.lmr_full_depth_moves || depth <= 2) {
        return 0;
    }
    
    // More aggressive LMR formula: log(depth) * log(move_count) / 2
    // This gives much bigger reductions for late moves at deep depths
    int reduction = 0;
    
    if (move_count >= 4) {
        // Logarithmic scaling - more aggressive reduction
        reduction = 1 + (depth / 3) + (move_count / 6);
        
        // Reduce more for non-PV nodes
        if (!is_pv_node) {
            reduction += 1;
        }
        
        // Even more reduction for very late moves
        if (move_count > 12) {
            reduction += 1;
        }
    }
    
    // Don't reduce too much
    reduction = std::min(reduction, config.lmr_reduction_limit);
    reduction = std::min(reduction, depth - 1);
    
    if (reduction > 0) {
        current_stats.lmr_reductions++;
    }
    return reduction;
}

bool Search::can_do_null_move(const Board& board, int depth, int beta) {
    if (depth < 3) return false;
    if (move_gen->is_in_check(board, board.get_active_color())) return false;
    
    // Don't do null move in endgame
    int material = evaluator->evaluate_material(board);
    if (material < 1000) return false; // Less than a rook
    
    return true;
}

bool Search::can_futility_prune(const Board& board, int depth, int alpha) {
    if (depth > 3) return false;
    if (move_gen->is_in_check(board, board.get_active_color())) return false;
    
    int eval = evaluator->evaluate(board);
    int margin = FUTILITY_MARGIN * depth;
    
    return eval + margin <= alpha;
}

void Search::check_time_and_stop() {
}

bool Search::time_expired() const {
    // Check if we have allocated time and if it has been exceeded
    if (allocated_time > std::chrono::milliseconds(0)) {
        auto elapsed = std::chrono::steady_clock::now() - search_start_time;
        return elapsed >= allocated_time;
    }
    return false;
}

void Search::update_time_management() {
    // Update time management statistics
    auto elapsed = std::chrono::steady_clock::now() - search_start_time;
    current_stats.time_elapsed_ms = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
}

int Search::lazy_smp_search(const Board& board, int depth, int num_instances, std::vector<Move>& pv) {
    // IMPROVED: Parallel Iterative Deepening Strategy
    // Each thread runs iterative deepening independently but shares the transposition table
    // This allows threads to benefit from each other's work while minimizing synchronization
    
    if (num_instances <= 1 || !config.enable_parallel_search) {
        return alpha_beta(const_cast<Board&>(board), depth, -MATE_SCORE, MATE_SCORE, 0, pv);
    }
    
    // Shared results across threads
    struct ThreadResult {
        Move best_move;
        int score = -MATE_SCORE;
        int depth_reached = 0;
        std::vector<Move> pv;
        std::mutex mutex;
    };
    
    auto shared_result = std::make_shared<ThreadResult>();
    
    // Worker function: each thread does iterative deepening
    auto worker = [this, shared_result, depth](Board worker_board, int thread_id) {
        try {
            // Each thread searches to the target depth using iterative deepening
            // Thread 0 searches normally, others add slight variations to avoid duplicating work
            int start_depth = (thread_id == 0) ? 1 : std::max(1, depth - 2);
            
            for (int d = start_depth; d <= depth && !should_stop(); ++d) {
                std::vector<Move> local_pv;
                int score = alpha_beta(worker_board, d, -MATE_SCORE, MATE_SCORE, 0, local_pv);
                
                if (!should_stop() && !local_pv.empty()) {
                    // Update shared result if we found a better move
                    std::lock_guard<std::mutex> lock(shared_result->mutex);
                    if (d > shared_result->depth_reached || 
                        (d == shared_result->depth_reached && score > shared_result->score)) {
                        shared_result->best_move = local_pv[0];
                        shared_result->score = score;
                        shared_result->depth_reached = d;
                        shared_result->pv = local_pv;
                    }
                }
            }
        } catch (...) {
            // Handle exceptions gracefully
        }
    };
    
    // Launch worker threads
    std::vector<std::future<void>> futures;
    futures.reserve(num_instances);
    
    for (int i = 0; i < num_instances; ++i) {
        Board thread_board = board;
        futures.push_back(thread_pool->submit([worker, thread_board = std::move(thread_board), i]() mutable {
            worker(thread_board, i);
        }));
    }
    
    // Wait for all threads
    for (auto& f : futures) {
        try {
            f.get();
        } catch (...) {
            // Handle exceptions
        }
    }
    
    // Return best result found
    pv = shared_result->pv;
    return shared_result->score;
}

int Search::parallel_root_search(Board& board, int depth, int alpha, int beta, std::vector<Move>& pv) {
    // Generate and order all root moves
    MoveList legal_moves = move_gen->generate_legal_moves(board);
    
    if (legal_moves.empty()) {
        return move_gen->is_in_check(board, board.get_active_color()) ? 
               -MATE_SCORE : 0;
    }
    
    // Convert to MoveScore and order
    std::vector<MoveScore> moves;
    moves.reserve(legal_moves.size());
    for (const Move& move : legal_moves) {
        moves.emplace_back(move);
    }
    
    Move tt_move;
    order_moves(board, moves, 0, tt_move);
    
    // Single move? No need for parallelization
    if (moves.size() == 1) {
        pv.clear();
        pv.push_back(moves[0].move);
        BitboardMoveUndoData undo = board.apply_move(moves[0].move);
        std::vector<Move> child_pv;
        int score = -alpha_beta(board, depth - 1, -beta, -alpha, 1, child_pv);
        board.undo_move(undo);
        pv.insert(pv.end(), child_pv.begin(), child_pv.end());
        return score;
    }
    
    // Determine how many threads to use
    int num_threads = std::min(config.thread_count, static_cast<int>(moves.size()));
    
    // Use parallel search at depth 6+ (need sufficient work to overcome overhead)
    if (num_threads <= 1 || !config.enable_parallel_search || moves.size() < 2 || depth < 6) {
        // Single-threaded search - use PVS with apply_move for efficiency
        int best_score = -MATE_SCORE;
        Move best_move;
        std::vector<Move> best_pv;
        
        for (size_t i = 0; i < moves.size() && !should_stop(); ++i) {
            const Move& move = moves[i].move;
            BitboardMoveUndoData undo = board.apply_move(move);
            
            std::vector<Move> current_pv;
            int score;
            
            if (i == 0) {
                // First move - full window
                score = -alpha_beta(board, depth - 1, -beta, -alpha, 1, current_pv);
            } else {
                // PVS: try null window first
                score = -alpha_beta(board, depth - 1, -alpha - 1, -alpha, 1, current_pv);
                
                if (score > alpha && score < beta) {
                    // Re-search with full window
                    current_pv.clear();
                    score = -alpha_beta(board, depth - 1, -beta, -alpha, 1, current_pv);
                }
            }
            
            board.undo_move(undo);
            
            if (score > best_score) {
                best_score = score;
                best_move = move;
                best_pv = current_pv;
                
                if (score > alpha) {
                    alpha = score;
                    pv.clear();
                    pv.push_back(move);
                    pv.insert(pv.end(), current_pv.begin(), current_pv.end());
                }
            }
        }
        
        return best_score;
    }
    
    // SIMPLIFIED PARALLEL STRATEGY with reduced overhead
    // Search first 2 moves on main thread, parallelize the rest
    
    struct MoveResult {
        int score = -MATE_SCORE;
        bool searched = false;
    };
    
    auto results = std::make_shared<std::vector<MoveResult>>(moves.size());
    auto pvs = std::make_shared<std::vector<std::vector<Move>>>(moves.size());
    
    // Search first 2 moves sequentially to get good alpha
    const size_t sequential_moves = std::min(static_cast<size_t>(2), moves.size());
    int best_score = -MATE_SCORE;
    size_t best_idx = 0;
    
    for (size_t i = 0; i < sequential_moves && !should_stop(); ++i) {
        const Move& move = moves[i].move;
        BitboardMoveUndoData undo = board.apply_move(move);
        
        if (i == 0) {
            (*results)[i].score = -alpha_beta(board, depth - 1, -beta, -alpha, 1, (*pvs)[i]);
        } else {
            (*results)[i].score = -alpha_beta(board, depth - 1, -alpha - 1, -alpha, 1, (*pvs)[i]);
            if ((*results)[i].score > alpha && (*results)[i].score < beta) {
                (*pvs)[i].clear();
                (*results)[i].score = -alpha_beta(board, depth - 1, -beta, -alpha, 1, (*pvs)[i]);
            }
        }
        
        board.undo_move(undo);
        (*results)[i].searched = true;
        
        if ((*results)[i].score > best_score) {
            best_score = (*results)[i].score;
            best_idx = i;
        }
        
        if ((*results)[i].score > alpha) {
            alpha = (*results)[i].score;
        }
        
        if (alpha >= beta) {
            pv.clear();
            pv.push_back(move);
            pv.insert(pv.end(), (*pvs)[i].begin(), (*pvs)[i].end());
            return (*results)[i].score;
        }
    }
    
    if (sequential_moves >= moves.size()) {
        pv.clear();
        pv.push_back(moves[best_idx].move);
        pv.insert(pv.end(), (*pvs)[best_idx].begin(), (*pvs)[best_idx].end());
        return best_score;
    }
    
    // Parallel search remaining moves
    const size_t remaining = moves.size() - sequential_moves;
    const int actual_threads = std::min(num_threads, static_cast<int>(remaining));
    const int search_alpha = alpha;
    const int search_beta = beta;
    const int search_depth = depth;
    
    // Share moves vector with threads (read-only, safe)
    auto moves_ptr = std::make_shared<std::vector<MoveScore>>(moves);
    
    auto worker = [this, results, pvs, search_alpha, search_beta, search_depth, moves_ptr](
        Board local_board, size_t start_idx, size_t end_idx) {
        
        for (size_t i = start_idx; i < end_idx && !should_stop(); ++i) {
            const Move& move = (*moves_ptr)[i].move;
            BitboardMoveUndoData undo = local_board.apply_move(move);
            
            int score = -alpha_beta(local_board, search_depth - 1, -search_alpha - 1, -search_alpha, 1, (*pvs)[i]);
            
            if (!should_stop() && score > search_alpha && score < search_beta) {
                (*pvs)[i].clear();
                score = -alpha_beta(local_board, search_depth - 1, -search_beta, -search_alpha, 1, (*pvs)[i]);
            }
            
            local_board.undo_move(undo);
            (*results)[i].score = score;
            (*results)[i].searched = true;
        }
    };
    
    std::vector<std::future<void>> futures;
    futures.reserve(actual_threads);
    
    const size_t moves_per_thread = (remaining + actual_threads - 1) / actual_threads;
    
    for (int t = 0; t < actual_threads; ++t) {
        size_t start_idx = sequential_moves + t * moves_per_thread;
        size_t end_idx = std::min(start_idx + moves_per_thread, moves.size());
        
        if (start_idx >= moves.size()) break;
        
        Board thread_board = board;
        futures.push_back(thread_pool->submit([worker, thread_board, start_idx, end_idx]() mutable {
            worker(std::move(thread_board), start_idx, end_idx);
        }));
    }
    
    for (auto& f : futures) {
        try {
            f.get();
        } catch (...) {
            // Handle exceptions gracefully
        }
    }
    
    // Collect best result
    for (size_t i = 0; i < moves.size(); ++i) {
        if ((*results)[i].searched && (*results)[i].score > best_score) {
            best_score = (*results)[i].score;
            best_idx = i;
        }
    }
    
    pv.clear();
    pv.push_back(moves[best_idx].move);
    pv.insert(pv.end(), (*pvs)[best_idx].begin(), (*pvs)[best_idx].end());
    
    return best_score;
}

int Search::parallel_search_worker(Board board, int depth, int alpha, int beta,
                                  const std::vector<MoveScore>& moves,
                                  int start_index, int end_index) {
    
    int best_score = -MATE_SCORE;
    
    for (int i = start_index; i < end_index && !should_stop(); ++i) {
        const Move& move = moves[i].move;
        
        BitboardMoveUndoData undo_data = board.apply_move(move);
        
        std::vector<Move> pv;
        int score = -alpha_beta(board, depth - 1, -beta, -alpha, 1, pv);
        
        // *** IMMEDIATE STOP CHECK AFTER RECURSIVE CALL ***
        if (should_stop()) {
            board.undo_move(undo_data);
            return best_score;
        }
        
        board.undo_move(undo_data);
        
        best_score = std::max(best_score, score);
        
        if (score >= beta) {
            break; // Beta cutoff
        }
    }
    
    return best_score;
}