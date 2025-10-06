#ifndef SEARCH_H
#define SEARCH_H

#include "../board/Board.h"
#include "../board/Move.h"
#include "../board/MoveGenerator.h"
#include "Evaluation.h"
#include "../board/TranspositionTable.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>
#include <mutex>
#include <future>

/**
 * @struct MoveScore
 * @brief Structure to hold a move and its evaluation score
 * 
 * Used for parallel move evaluation at the root level to track
 * which moves have been evaluated and their scores.
 */
struct MoveScore {
    Move move;
    int score;
    bool evaluated;
    
    MoveScore() : move(), score(-32000), evaluated(false) {}
    MoveScore(const Move& m) : move(m), score(-32000), evaluated(false) {}
};

/**
 * @struct SearchStats
 * @brief Statistics collected during chess search operations
 * 
 * This structure tracks various performance metrics and statistics
 * during the search process, useful for debugging, profiling, and
 * engine analysis.
 */
struct SearchStats {
    long long nodes_searched = 0;      ///< Total nodes evaluated
    long long quiescence_nodes = 0;    ///< Nodes searched in quiescence
    long long tt_hits = 0;             ///< Transposition table hits
    long long beta_cutoffs = 0;        ///< Beta cutoffs (useful for profiling)
    long long null_move_cutoffs = 0;   ///< Null-move pruning cutoffs
    long long lmr_reductions = 0;      ///< Late move reductions applied
    
    /**
     * @brief Reset all statistics to zero
     */
    void clear() { *this = {}; }
};

/**
 * @struct SearchResult
 * @brief Complete result of a chess search operation
 * 
 * Contains the best move found, evaluation score, search depth,
 * performance statistics, and timing information from a search.
 */
struct SearchResult {
    Move best_move;                    ///< Best move found
    int score = 0;                     ///< Evaluation score (cp or mate score)
    int depth = 0;                     ///< Search depth reached
    int sel_depth = 0;                 ///< Selective depth (deepest ply reached)
    SearchStats stats;                 ///< Collected search statistics
    std::chrono::milliseconds time_elapsed{0};  ///< Total search time
};

/**
 * @brief Per-thread search context for parallel root search
 * 
 * Contains all the data structures that each search thread needs
 * to operate independently without contention. This includes a
 * private board copy, per-thread statistics, and local killer/history tables.
 */
struct SearchContext {
    Board board;                    ///< Private board copy for this thread
    SearchStats stats;              ///< Per-thread search statistics
    Move killer_moves[2][64];       ///< Per-thread killer move table [depth][slot]
    int history_table[2][64][64];   ///< Per-thread history heuristic table [color][from][to]
    MoveGenerator move_generator;   ///< Thread-local move generator to avoid repeated instantiation
    
    /**
     * @brief Constructor that initializes the context with a board copy
     * @param original_board The board to copy for this thread
     */
    SearchContext(const Board& original_board) : board(original_board), stats() {
        // Initialize killer moves to invalid moves
        for (int depth = 0; depth < 64; depth++) {
            for (int slot = 0; slot < 2; slot++) {
                killer_moves[slot][depth] = Move();
            }
        }
        
        // Initialize history table to zeros
        for (int color = 0; color < 2; color++) {
            for (int from = 0; from < 64; from++) {
                for (int to = 0; to < 64; to++) {
                    history_table[color][from][to] = 0;
                }
            }
        }
    }
};

/**
 * @class Search
 * @brief High-performance multithreaded chess search engine
 * 
 * This class implements a sophisticated chess search algorithm using:
 * - Minimax algorithm with alpha-beta pruning
 * - Iterative deepening for time management
 * - Multithreading for parallel search
 * - Cooperative cancellation using atomic stop flags
 * - Comprehensive search statistics and profiling
 * 
 * The search engine supports both time-limited and depth-limited searches,
 * with four main search function variants for different use cases.
 * 
 * @note All search operations are thread-safe and support cooperative
 *       cancellation without using dangerous thread termination methods.
 */
class Search {
public:
    /**
     * @brief Constructor - initializes search engine with evaluation system
     * 
     * Creates a new search engine instance with default parameters.
     * Initializes the evaluation system, sets up thread management,
     * and prepares internal data structures.
     */
    Search();
    
    /**
     * @brief Destructor - ensures clean shutdown of all threads
     * 
     * Stops any running search operations and joins all worker threads
     * to ensure clean shutdown without abandoned threads.
     */
    ~Search();
    
    // Main search interface - 4 function variants as required
    
    /**
     * @brief Search for the best move without time limit
     * 
     * Performs a chess search to find the best move in the current position
     * using iterative deepening. The search continues until a reasonable
     * depth is reached or manually stopped.
     * 
     * @param board The current board position to search from
     * @param max_depth Maximum search depth (default: 10)
     * @return The best move found
     * 
     * @note This function may take a long time for complex positions.
     *       Consider using the time-limited variant for practical use.
     */
    Move search_move(Board& board, int max_depth);
    
    /**
     * @brief Search for the best move with time limit
     * 
     * Performs a time-limited chess search using iterative deepening.
     * The search stops when the time limit is reached and returns the
     * best move found so far.
     * 
     * @param board The current board position to search from
     * @param time_limit Maximum time allowed for the search
     * @param max_depth Maximum search depth (default: 50)
     * @return The best move found within the time limit
     * 
     * @note Always returns a legal move, even if interrupted early.
     */
    Move search_move(Board& board, std::chrono::milliseconds time_limit, int max_depth);
    
    /**
     * @brief Search for the best move and return detailed results without time limit
     * 
     * Performs a comprehensive chess search and returns detailed information
     * including the best move, evaluation score, search depth, and statistics.
     * 
     * @param board The current board position to search from
     * @param max_depth Maximum search depth (default: 10)
     * @return Complete search results including move, score, and statistics
     * 
     * @note This function may take a long time for complex positions.
     */
    SearchResult search(Board& board, int max_depth);
    
    /**
     * @brief Search for the best move and return detailed results with time limit
     * 
     * Performs a time-limited comprehensive chess search using iterative deepening
     * and multithreading. Returns detailed results including performance statistics.
     * 
     * @param board The current board position to search from
     * @param time_limit Maximum time allowed for the search
     * @param max_depth Maximum search depth (default: 50)
     * @return Complete search results including move, score, and statistics
     * 
     * @note This is the most comprehensive search function, recommended for
     *       engine analysis and performance profiling.
     */
    SearchResult search(Board& board, std::chrono::milliseconds time_limit, int max_depth);
    
    /**
     * @brief Stop any currently running search operation
     * 
     * Sets the global stop flag to signal all search threads to terminate
     * cooperatively. This function is thread-safe and can be called from
     * any thread.
     */
    void stop_search();
    
    /**
     * @brief Check if a search operation is currently running
     * 
     * @return true if search is active, false otherwise
     */
    bool is_searching() const;
    
    /**
     * @brief Set the number of threads to use for parallel search
     * 
     * Configures the search engine to use the specified number of threads
     * for parallel search operations. More threads can improve search speed
     * on multi-core systems.
     * 
     * @param num_threads Number of threads to use (1-64, default: 4)
     * 
     * @note Changes take effect on the next search operation.
     *       Using too many threads may decrease performance due to overhead.
     */
    void set_thread_count(int num_threads);
    
    /**
     * @brief Get the current number of search threads
     * 
     * @return Current number of threads configured for search
     */
    int get_thread_count() const;
    
    /**
     * @brief Search for the best move using incremental evaluation with optional debug output
     * 
     * Performs a chess search using incremental evaluation updates for efficiency.
     * The incremental evaluation maintains evaluation state across moves to avoid
     * full re-evaluation at each position.
     * 
     * @param board The current board position to search from
     * @param max_depth Maximum search depth (default: 10)
     * @param debug_output Enable debug output showing move depth and scores (default: false)
     * @return Complete search results including move, score, and statistics
     * 
     * @note This method uses the Evaluation class's incremental evaluation methods
     *       for improved performance on deep searches.
     */
    SearchResult search_incremental(Board& board, int max_depth, bool debug_output = false);
    
private:
    // Core search implementation
    
    /**
     * @brief Main iterative deepening search loop
     * 
     * Implements the core iterative deepening algorithm that progressively
     * searches deeper until time runs out or maximum depth is reached.
     * 
     * @param board The board position to search
     * @param max_depth Maximum depth to search
     * @param result Reference to store search results
     */
    void iterative_deepening(Board& board, int max_depth, SearchResult& result);
    
    // Old minimax declaration removed - using minimax_with_context instead
    
    // Old quiescence_search declaration removed - using quiescence_search_with_context instead
    
    /**
     * @brief Worker thread function for parallel search
     * 
     * Each worker thread runs this function to search a portion of the
     * move tree in parallel with other threads.
     * 
     * @param board Reference to the board position to search
     * @param moves List of moves to search
     * @param start_idx Starting index in the move list
     * @param end_idx Ending index in the move list
     * @param depth Search depth for this worker
     * @return SearchResult with best move and score found
     */
    SearchResult search_worker(Board& board, const MoveList& moves, int start_idx, int end_idx, int depth);
    
    /**
     * @brief Worker thread function for parallel search with aspiration window and PVS
     * 
     * Each worker thread runs this function to search a portion of the move tree
     * using Principal Variation Search with aspiration windows for better pruning.
     * 
     * @param board Reference to the board position to search
     * @param moves List of moves to search
     * @param start_idx Starting index in the move list
     * @param end_idx Ending index in the move list
     * @param depth Search depth for this worker
     * @param alpha Alpha bound for aspiration window
     * @param beta Beta bound for aspiration window
     * @return SearchResult with best move and score found
     */
    SearchResult search_worker_with_window(Board& board, const MoveList& moves, int start_idx, int end_idx, int depth, int alpha, int beta);
    
    /**
     * @brief Parallel root search worker function
     * 
     * Each thread runs this function to search a subset of root moves
     * with its own SearchContext to avoid contention.
     * 
     * @param context Per-thread search context with board copy and local data
     * @param moves List of moves to search
     * @param start_idx Starting index in the move list for this thread
     * @param end_idx Ending index in the move list for this thread
     * @param depth Search depth
     * @return SearchResult with best move and score found by this thread
     */
    SearchResult parallel_root_worker(SearchContext& context, const MoveList& moves, int start_idx, int end_idx, int depth);
    
    SearchResult parallel_root_search(Board& board, const MoveList& moves, int depth);
    
    /**
     * @brief Parallel root search with aspiration window and PVS
     * 
     * Performs parallel search of root moves using aspiration windows and
     * Principal Variation Search for improved pruning efficiency.
     * 
     * @param board The board position to search
     * @param moves List of moves to search
     * @param depth Search depth
     * @param alpha Alpha bound for aspiration window
     * @param beta Beta bound for aspiration window
     * @return SearchResult with best move and score found
     */
    SearchResult parallel_root_search_with_window(Board& board, const MoveList& moves, int depth, int alpha, int beta);
    
    /**
     * @brief Initialize the persistent thread pool
     * 
     * Creates and starts the worker threads that will be reused
     * across multiple search operations.
     */
    void init_thread_pool();
    
    /**
     * @brief Shutdown the persistent thread pool
     * 
     * Stops all worker threads and cleans up resources.
     */
    void shutdown_thread_pool();
    
    // Context-based search methods for thread safety
    int minimax_with_context(SearchContext& context, int depth, int alpha, int beta, bool maximizing_player, int ply);
    int quiescence_search_with_context(SearchContext& context, int alpha, int beta, bool maximizing_player, int qs_depth = 0);
    
    // Incremental evaluation minimax for search_incremental
    int minimax_incremental(Board& board, int depth, int alpha, int beta, bool maximizing_player, int ply);
    void order_moves_with_context(MoveList& moves, SearchContext& context, int ply, const Move& tt_move = Move());
    int evaluate_move_priority_with_context(const Move& move, const SearchContext& context, int ply, const Move& tt_move);
    
    // Helper functions for search optimizations
    bool has_non_pawn_material(const Board& board, Board::Color color) const;
    int get_piece_value(char piece) const;
    
    // Parallel move evaluation functions
    void parallel_evaluate_moves(Board& board, std::vector<MoveScore>& move_scores, int depth, SearchStats* out_stats = nullptr);
    void sequential_evaluate_moves(Board& board, std::vector<MoveScore>& move_scores, int depth, SearchStats* out_stats = nullptr);
    void parallel_evaluate_moves_with_aspiration(Board& board, std::vector<MoveScore>& move_scores, int depth, int prev_score);
    void parallel_evaluate_moves_windowed(Board& board, std::vector<MoveScore>& move_scores, int depth, int alpha, int beta);
    void sequential_evaluate_moves_windowed(Board& board, std::vector<MoveScore>& move_scores, int depth, int alpha, int beta);
    
    /**
     * @brief Time management worker thread
     * 
     * Monitors the search time and sets the stop flag when the time
     * limit is reached, ensuring cooperative cancellation.
     * 
     * @param time_limit Maximum time allowed for search
     */
    void time_management_worker(std::chrono::milliseconds time_limit);
    
    /**
     * @brief Check if search should stop (cooperative cancellation)
     * 
     * Checks the atomic stop flag to determine if the search should
     * terminate. Called at safe points during search.
     * 
     * @return true if search should stop, false to continue
     */
    bool should_stop() const;
    
    // Old order_moves and evaluate_move_priority declarations removed - using context-based versions instead
    
    // Member variables
    std::unique_ptr<Evaluation> evaluator;  ///< Position evaluation engine
    TranspositionTable transposition_table; ///< Transposition table for caching search results
    
    // Thread management and synchronization
    std::atomic<bool> stop_flag;            ///< Global stop flag for cooperative cancellation
    std::atomic<bool> search_active;        ///< Flag indicating if search is running
    
    // Global timer system for strict time enforcement
    std::atomic<std::chrono::steady_clock::time_point> search_start_time; ///< Global search start time
    std::atomic<std::chrono::milliseconds> time_limit_ms;                 ///< Global time limit in milliseconds
    std::atomic<bool> time_limit_active;                                  ///< Flag indicating if time limit is active
    int thread_count;                       ///< Number of threads to use for search
    std::vector<std::thread> worker_threads; ///< Pool of worker threads
    // Note: time_manager_thread removed - time checking is now integrated into search loops
    
    // Parallel root search components
    std::atomic<bool> thread_pool_active;   ///< Flag indicating if thread pool is running
    std::vector<std::future<SearchResult>> search_futures; ///< Futures for collecting parallel search results
    
    // Synchronization
    mutable std::mutex search_mutex;        ///< Mutex for thread-safe operations
    
    // Search parameters
    static constexpr int MAX_DEPTH = 64;    ///< Maximum search depth limit
    static constexpr int MIN_THREADS = 1;   ///< Minimum number of threads
    static constexpr int MAX_THREADS = 64;  ///< Maximum number of threads
    
    // Search constants
    static constexpr int MATE_SCORE = 30000;     ///< Score representing checkmate
    static constexpr int DRAW_SCORE = 0;         ///< Score representing a draw
    static constexpr int INFINITY_SCORE = 32000; ///< Infinity value for alpha-beta
    static constexpr int MAX_QUIESCENCE_DEPTH = 16; ///< Maximum quiescence search depth
    static constexpr int KILLER_MOVES_PER_PLY = 2;  ///< Number of killer moves per ply
};

#endif // SEARCH_H