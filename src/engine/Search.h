#ifndef SEARCH_H
#define SEARCH_H

#include "../board/Board.h"
#include "../board/Move.h"
#include "Evaluation.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>
#include <mutex>

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
    Move search_move(Board& board, int max_depth = 10);
    
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
    Move search_move(Board& board, std::chrono::milliseconds time_limit, int max_depth = 50);
    
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
    SearchResult search(Board& board, int max_depth = 10);
    
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
    SearchResult search(Board& board, std::chrono::milliseconds time_limit, int max_depth = 50);
    
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
    
    /**
     * @brief Minimax search with alpha-beta pruning
     * 
     * Core minimax algorithm implementation with alpha-beta pruning
     * for efficient tree search.
     * 
     * @param board Current board position
     * @param depth Remaining search depth
     * @param alpha Alpha value for pruning
     * @param beta Beta value for pruning
     * @param maximizing_player True if maximizing player's turn
     * @param stats Reference to search statistics
     * @param ply Current ply from root (for killer moves)
     * @return Evaluation score of the position
     */
    int minimax(Board& board, int depth, int alpha, int beta, bool maximizing_player, SearchStats& stats, int ply = 0);
    
    /**
     * @brief Quiescence search for tactical stability
     * 
     * Extends the search in tactical positions to avoid horizon effects
     * by searching only capture moves until a quiet position is reached.
     * 
     * @param board Current board position
     * @param alpha Alpha value for pruning
     * @param beta Beta value for pruning
     * @param maximizing_player True if maximizing player's turn
     * @param stats Reference to search statistics
     * @param qs_depth Current quiescence search depth (for limiting)
     * @return Evaluation score of the quiet position
     */
    int quiescence_search(Board& board, int alpha, int beta, bool maximizing_player, SearchStats& stats, int qs_depth = 0);
    
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
    
    /**
     * @brief Order moves for better alpha-beta pruning efficiency
     * 
     * Sorts the move list to improve the likelihood of early cutoffs
     * in the alpha-beta search, significantly improving performance.
     * Captures, killer moves, and history heuristic are used.
     * 
     * @param moves List of moves to order
     * @param board Current board position for move evaluation
     * @param ply Current ply for killer move lookup
     */
    void order_moves(MoveList& moves, Board& board, int ply = 0);
    
    /**
     * @brief Evaluate move priority for ordering
     * 
     * Assigns a priority score to a move for ordering purposes.
     * Higher scores indicate moves that should be searched first.
     * 
     * @param move The move to evaluate
     * @param board Current board position
     * @param ply Current ply for killer move lookup
     * @return Priority score for the move
     */
    int evaluate_move_priority(const Move& move, Board& board, int ply = 0);
    
    // Member variables
    
    std::unique_ptr<Evaluation> evaluator;  ///< Position evaluation engine
    
    // Thread management
    std::atomic<bool> stop_flag;            ///< Global stop flag for cooperative cancellation
    std::atomic<bool> search_active;        ///< Flag indicating if search is running
    int thread_count;                       ///< Number of threads to use for search
    std::vector<std::thread> worker_threads; ///< Pool of worker threads
    std::thread time_manager_thread;        ///< Time management thread
    
    // Synchronization
    mutable std::mutex search_mutex;        ///< Mutex for thread-safe operations
    mutable std::mutex killer_history_mutex; ///< Mutex for killer moves and history table protection
    
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
    
    // Killer moves table [ply][killer_index]
    Move killer_moves[MAX_DEPTH][KILLER_MOVES_PER_PLY];
    
    // History heuristic table [from][to]
    int history_table[64][64];
};

#endif // SEARCH_H