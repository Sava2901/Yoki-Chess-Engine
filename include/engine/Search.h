#ifndef SEARCH_H
#define SEARCH_H

#include <cstdint>
#include <chrono>
#include <atomic>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <queue>
#include <stdexcept>

#include "../board/Board.h"
#include "../board/Move.h"
#include "../board/MoveGenerator.h"
#include "./Evaluation.h"
#include "./TranspositionTable.h"

// Forward declarations
class Board;
class Move;
class MoveGenerator;
class Evaluation;
class TranspositionTable;

/**
 * @brief Exception thrown when search time expires
 * 
 * This exception is used to immediately unwind the recursive search stack
 * when the timer expires, ensuring clean termination without hanging.
 */
struct TimeUpException : public std::exception {
    const char* what() const noexcept override { 
        return "Search time expired"; 
    }
};

/**
 * @brief Search statistics structure
 * 
 * Contains comprehensive performance metrics and diagnostic information.
 */
struct SearchStats {
    std::atomic<uint64_t> nodes_searched{0};        ///< Total nodes searched (atomic for thread-safety)
    std::atomic<uint64_t> qnodes_searched{0};       ///< Quiescence nodes searched (atomic for thread-safety)
    std::atomic<uint64_t> tt_hits{0};               ///< Transposition table hits (atomic for thread-safety)
    std::atomic<uint64_t> tt_cutoffs{0};            ///< TT-induced cutoffs (atomic for thread-safety)
    std::atomic<uint64_t> beta_cutoffs{0};          ///< Beta cutoffs (fail-high, atomic for thread-safety)
    std::atomic<uint64_t> alpha_improvements{0};    ///< Alpha improvements (atomic for thread-safety)
    std::atomic<uint64_t> null_move_cutoffs{0};     ///< Null move pruning cutoffs (atomic for thread-safety)
    std::atomic<uint64_t> futility_prunes{0};       ///< Futility pruning cutoffs (atomic for thread-safety)
    std::atomic<uint64_t> lmr_reductions{0};        ///< Late move reductions applied (atomic for thread-safety)
    std::atomic<uint64_t> check_extensions{0};      ///< Check extensions applied (atomic for thread-safety)
    std::atomic<uint64_t> singular_extensions{0};   ///< Singular extensions applied (atomic for thread-safety)
    std::atomic<uint64_t> recapture_extensions{0};  ///< Recapture extensions applied (atomic for thread-safety)
    
    double branching_factor = 0.0;         ///< Average branching factor
    double time_elapsed_ms = 0.0;          ///< Total search time in milliseconds
    double nodes_per_second = 0.0;         ///< Search speed (nodes/second)
    
    /**
     * @brief Default constructor
     */
    SearchStats() = default;
    
    /**
     * @brief Copy constructor - loads atomic values
     */
    SearchStats(const SearchStats& other) 
        : nodes_searched(other.nodes_searched.load())
        , qnodes_searched(other.qnodes_searched.load())
        , tt_hits(other.tt_hits.load())
        , tt_cutoffs(other.tt_cutoffs.load())
        , beta_cutoffs(other.beta_cutoffs.load())
        , alpha_improvements(other.alpha_improvements.load())
        , null_move_cutoffs(other.null_move_cutoffs.load())
        , futility_prunes(other.futility_prunes.load())
        , lmr_reductions(other.lmr_reductions.load())
        , check_extensions(other.check_extensions.load())
        , singular_extensions(other.singular_extensions.load())
        , recapture_extensions(other.recapture_extensions.load())
        , branching_factor(other.branching_factor)
        , time_elapsed_ms(other.time_elapsed_ms)
        , nodes_per_second(other.nodes_per_second) {}
    
    /**
     * @brief Copy assignment operator - loads and stores atomic values
     */
    SearchStats& operator=(const SearchStats& other) {
        if (this != &other) {
            nodes_searched.store(other.nodes_searched.load());
            qnodes_searched.store(other.qnodes_searched.load());
            tt_hits.store(other.tt_hits.load());
            tt_cutoffs.store(other.tt_cutoffs.load());
            beta_cutoffs.store(other.beta_cutoffs.load());
            alpha_improvements.store(other.alpha_improvements.load());
            null_move_cutoffs.store(other.null_move_cutoffs.load());
            futility_prunes.store(other.futility_prunes.load());
            lmr_reductions.store(other.lmr_reductions.load());
            check_extensions.store(other.check_extensions.load());
            singular_extensions.store(other.singular_extensions.load());
            recapture_extensions.store(other.recapture_extensions.load());
            branching_factor = other.branching_factor;
            time_elapsed_ms = other.time_elapsed_ms;
            nodes_per_second = other.nodes_per_second;
        }
        return *this;
    }
    
    /**
     * @brief Reset all statistics to zero
     */
    void reset() {
        nodes_searched.store(0);
        qnodes_searched.store(0);
        tt_hits.store(0);
        tt_cutoffs.store(0);
        beta_cutoffs.store(0);
        alpha_improvements.store(0);
        null_move_cutoffs.store(0);
        futility_prunes.store(0);
        lmr_reductions.store(0);
        check_extensions.store(0);
        singular_extensions.store(0);
        recapture_extensions.store(0);
        branching_factor = 0.0;
        time_elapsed_ms = 0.0;
        nodes_per_second = 0.0;
    }
    
    /**
     * @brief Calculate derived statistics
     */
    void calculate_derived_stats() {
        if (time_elapsed_ms > 0.0) {
            nodes_per_second = (nodes_searched.load() * 1000.0) / time_elapsed_ms;
        }
    }
};



/**
 * @brief Search result structure
 * 
 * Contains the complete result of a search operation.
 */
struct SearchResult {
    Move best_move;                        ///< Best move found
    int score = 0;                         ///< Position evaluation in centipawns
    int depth = 0;                         ///< Actual search depth reached
    SearchStats stats;                     ///< Comprehensive search statistics
    std::vector<Move> principal_variation; ///< Principal variation (best line)
    bool time_expired = false;             ///< Whether search was stopped due to time limit
    bool search_stopped = false;           ///< Whether search was manually stopped
    
    /**
     * @brief Default constructor
     */
    SearchResult() = default;
    
    /**
     * @brief Check if result is valid
     * @return true if best_move is valid
     */
    bool is_valid() const {
        return best_move.is_valid();
    }
};

/**
 * @brief Move with score for move ordering
 * 
 * Used in move ordering algorithms to associate moves with their heuristic scores.
 */
struct MoveScore {
    Move move;                             ///< The chess move
    int score = -32000;                    ///< Heuristic score for ordering
    bool evaluated = false;                ///< Whether move has been evaluated
    
    /**
     * @brief Default constructor
     */
    MoveScore() = default;
    
    /**
     * @brief Constructor with move
     * @param m Move to initialize with
     */
    explicit MoveScore(const Move& m) : move(m) {}
    
    /**
     * @brief Constructor with move and score
     * @param m Move to initialize with
     * @param s Score to initialize with
     */
    MoveScore(const Move& m, int s) : move(m), score(s), evaluated(true) {}
    
    /**
     * @brief Comparison operator for sorting (higher scores first)
     */
    bool operator<(const MoveScore& other) const {
        return score > other.score; // Higher scores first
    }
};

/**
 * @brief Killer move table for move ordering
 */
struct KillerMoves {
    static constexpr int MAX_PLY = 64;  // Reduced to match Search::MAX_PLY
    static constexpr int KILLERS_PER_PLY = 2;
    
    Move killers[MAX_PLY][KILLERS_PER_PLY];
    mutable std::mutex mutex_;  // Protect concurrent access
    
    /**
     * @brief Add a killer move at the given ply
     * @param ply Search ply
     * @param move Killer move to add
     */
    void add_killer(int ply, const Move& move) {
        if (ply >= 0 && ply < MAX_PLY) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (killers[ply][0] != move) {
                killers[ply][1] = killers[ply][0];
                killers[ply][0] = move;
            }
        }
    }
    
    /**
     * @brief Check if move is a killer at the given ply
     * @param ply Search ply
     * @param move Move to check
     * @return true if move is a killer move
     */
    bool is_killer(int ply, const Move& move) const {
        if (ply >= 0 && ply < MAX_PLY) {
            std::lock_guard<std::mutex> lock(mutex_);
            return killers[ply][0] == move || killers[ply][1] == move;
        }
        return false;
    }
    
    /**
     * @brief Clear all killer moves
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (int i = 0; i < MAX_PLY; ++i) {
            for (int j = 0; j < KILLERS_PER_PLY; ++j) {
                killers[i][j] = Move();
            }
        }
    }
};

/**
 * @brief History heuristic table for move ordering using butterfly tables
 * Tracks move success from-square to to-square for better accuracy
 */
struct HistoryTable {
    static constexpr int MAX_SQUARES = 64;
    static constexpr int MAX_PIECES = 12; // 6 piece types * 2 colors
    
    // Butterfly tables: [piece][from_square][to_square]
    int history[MAX_PIECES][MAX_SQUARES][MAX_SQUARES];
    mutable std::mutex mutex_;  // Protect concurrent access
    
    /**
     * @brief Constructor - initializes table to zero
     */
    HistoryTable() {
        clear();
    }
    
    /**
     * @brief Update history score for a move (reward good moves)
     * @param move Move that caused cutoff
     * @param depth Search depth (higher depth = more important)
     * @param bonus Additional bonus (can be negative for failed moves)
     */
    void update(const Move& move, int depth, int bonus = 0) {
        int piece_index = get_piece_index(move);
        int from_square = move.from_rank * 8 + move.from_file;
        int to_square = move.to_rank * 8 + move.to_file;
        
        if (piece_index >= 0 && piece_index < MAX_PIECES && 
            from_square >= 0 && from_square < MAX_SQUARES &&
            to_square >= 0 && to_square < MAX_SQUARES) {
            
            std::lock_guard<std::mutex> lock(mutex_);
            
            // Bonus increases quadratically with depth
            int score_delta = (depth * depth) + bonus;
            history[piece_index][from_square][to_square] += score_delta;
            
            // Prevent overflow by scaling down if necessary
            if (abs(history[piece_index][from_square][to_square]) > 100000) {
                for (int i = 0; i < MAX_PIECES; ++i) {
                    for (int j = 0; j < MAX_SQUARES; ++j) {
                        for (int k = 0; k < MAX_SQUARES; ++k) {
                            history[i][j][k] /= 2;
                        }
                    }
                }
            }
        }
    }
    
    /**
     * @brief Penalize a move that failed to cause cutoff
     * @param move Move that failed
     * @param depth Search depth
     */
    void penalize(const Move& move, int depth) {
        update(move, 0, -(depth * depth / 4));
    }
    
    /**
     * @brief Get history score for a move
     * @param move Move to get score for
     * @return History score
     */
    int get_score(const Move& move) const {
        int piece_index = get_piece_index(move);
        int from_square = move.from_rank * 8 + move.from_file;
        int to_square = move.to_rank * 8 + move.to_file;
        
        if (piece_index >= 0 && piece_index < MAX_PIECES && 
            from_square >= 0 && from_square < MAX_SQUARES &&
            to_square >= 0 && to_square < MAX_SQUARES) {
            std::lock_guard<std::mutex> lock(mutex_);
            return history[piece_index][from_square][to_square];
        }
        return 0;
    }
    
    /**
     * @brief Clear all history scores
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (int i = 0; i < MAX_PIECES; ++i) {
            for (int j = 0; j < MAX_SQUARES; ++j) {
                for (int k = 0; k < MAX_SQUARES; ++k) {
                    history[i][j][k] = 0;
                }
            }
        }
    }
    
private:
    /**
     * @brief Get piece index for history table
     * @param move Move to get piece index for
     * @return Piece index (0-11) or -1 if invalid
     */
    int get_piece_index(const Move& move) const {
        char piece_char = move.piece;
        
        if (piece_char == '.') return -1;
        
        // Convert piece character to piece type and color
        bool is_white = (piece_char >= 'A' && piece_char <= 'Z');
        char normalized = is_white ? piece_char : (piece_char - 'a' + 'A');
        
        int piece_type = -1;
        switch (normalized) {
            case 'P': piece_type = 0; break; // PAWN
            case 'N': piece_type = 1; break; // KNIGHT
            case 'B': piece_type = 2; break; // BISHOP
            case 'R': piece_type = 3; break; // ROOK
            case 'Q': piece_type = 4; break; // QUEEN
            case 'K': piece_type = 5; break; // KING
            default: return -1;
        }
        
        return piece_type + (is_white ? 0 : 6);
    }
};

/**
 * @brief Counter-move table for move ordering
 * Tracks the best response to opponent's previous move
 */
struct CounterMoveTable {
    static constexpr int MAX_PIECES = 12; // 6 piece types * 2 colors
    static constexpr int MAX_SQUARES = 64;
    
    // counter_moves[previous_piece][previous_to_square] = best_counter_move
    Move counter_moves[MAX_PIECES][MAX_SQUARES];
    
    /**
     * @brief Constructor
     */
    CounterMoveTable() {
        clear();
    }
    
    /**
     * @brief Update counter-move for opponent's previous move
     * @param prev_move Opponent's previous move
     * @param counter_move Our move that refuted it
     */
    void update(const Move& prev_move, const Move& counter_move) {
        int piece_index = get_piece_index(prev_move);
        int to_square = prev_move.to_rank * 8 + prev_move.to_file;
        
        if (piece_index >= 0 && piece_index < MAX_PIECES &&
            to_square >= 0 && to_square < MAX_SQUARES) {
            counter_moves[piece_index][to_square] = counter_move;
        }
    }
    
    /**
     * @brief Get counter-move for opponent's previous move
     * @param prev_move Opponent's previous move
     * @return Counter-move (empty if none)
     */
    Move get_counter(const Move& prev_move) const {
        int piece_index = get_piece_index(prev_move);
        int to_square = prev_move.to_rank * 8 + prev_move.to_file;
        
        if (piece_index >= 0 && piece_index < MAX_PIECES &&
            to_square >= 0 && to_square < MAX_SQUARES) {
            return counter_moves[piece_index][to_square];
        }
        return Move();
    }
    
    /**
     * @brief Check if move is a counter-move
     * @param prev_move Opponent's previous move
     * @param move Move to check
     * @return true if move is the counter-move
     */
    bool is_counter(const Move& prev_move, const Move& move) const {
        Move counter = get_counter(prev_move);
        return counter.from_rank == move.from_rank && 
               counter.from_file == move.from_file &&
               counter.to_rank == move.to_rank &&
               counter.to_file == move.to_file;
    }
    
    /**
     * @brief Clear all counter-moves
     */
    void clear() {
        for (int i = 0; i < MAX_PIECES; ++i) {
            for (int j = 0; j < MAX_SQUARES; ++j) {
                counter_moves[i][j] = Move();
            }
        }
    }

private:
    /**
     * @brief Get piece index for move
     */
    int get_piece_index(const Move& move) const {
        char piece_char = move.piece;
        if (piece_char == '.') return -1;
        
        bool is_white = (piece_char >= 'A' && piece_char <= 'Z');
        char normalized = is_white ? piece_char : (piece_char - 'a' + 'A');
        
        int piece_type = -1;
        switch (normalized) {
            case 'P': piece_type = 0; break;
            case 'N': piece_type = 1; break;
            case 'B': piece_type = 2; break;
            case 'R': piece_type = 3; break;
            case 'Q': piece_type = 4; break;
            case 'K': piece_type = 5; break;
            default: return -1;
        }
        
        return piece_type + (is_white ? 0 : 6);
    }
};

/**
 * @brief Search configuration parameters
 */
struct SearchConfig {
    // Time management
    bool use_time_management = true;       ///< Enable time management
    double time_safety_margin = 0.05;     ///< Safety margin (5% of allocated time)
    
    // Search extensions
    bool enable_check_extensions = true;   ///< Enable check extensions
    bool enable_singular_extensions = true; ///< Enable singular extensions
    bool enable_recapture_extensions = true; ///< Enable recapture extensions
    int max_extensions_per_path = 16;      ///< Maximum extensions per search path
    
    // Pruning techniques
    bool enable_null_move_pruning = true;  ///< Enable null move pruning
    bool enable_futility_pruning = true;   ///< Enable futility pruning
    bool enable_late_move_reductions = true; ///< Enable late move reductions
    int null_move_reduction = 3;           ///< Null move search reduction
    int lmr_full_depth_moves = 4;          ///< Moves to search at full depth before LMR
    int lmr_reduction_limit = 3;           ///< Maximum LMR reduction
    
    // Move ordering
    bool enable_killer_moves = true;       ///< Enable killer move heuristic
    bool enable_history_heuristic = true;  ///< Enable history heuristic
    bool enable_counter_moves = true;      ///< Enable counter-move heuristic
    bool enable_see_ordering = true;       ///< Enable SEE-based move ordering
    
    // Transposition table
    bool enable_transposition_table = true; ///< Enable transposition table
    size_t tt_size_mb = 64;               ///< TT size in megabytes
    
    // Aspiration windows
    bool enable_aspiration_windows = true; ///< Enable aspiration windows
    int aspiration_window_size = 50;       ///< Initial aspiration window size
    int aspiration_window_max = 400;       ///< Maximum aspiration window size
    
    // Quiescence search
    bool enable_quiescence_search = true;  ///< Enable quiescence search
    int quiescence_max_depth = 16;         ///< Maximum quiescence search depth
    
    // Threading
    int thread_count = 1;                  ///< Number of search threads
    bool enable_parallel_search = true;    ///< Enable parallel search (now optimized with low overhead)
    int min_split_depth = 100;             ///< Minimum depth for YBWC node splitting (high default disables YBWC)
    int max_parallel_tasks = 64;           ///< Maximum simultaneous split points
    int lazy_smp_instances = 0;            ///< Number of independent Lazy SMP search instances (0 = no Lazy SMP, use parallel root search instead)
    
    /**
     * @brief Default constructor with sensible defaults
     */
    SearchConfig() = default;
};

/**
 * @brief Thread pool for parallel search
 */
class ThreadPool {
public:
    /**
     * @brief Constructor
     * @param num_threads Number of worker threads
     */
    explicit ThreadPool(int num_threads);
    
    /**
     * @brief Destructor - stops all threads
     */
    ~ThreadPool();
    
    /**
     * @brief Submit a task to the thread pool
     * @param task Task function to execute
     * @return Future for the task result
     */
    template<typename F, typename... Args>
    auto submit(F&& task, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>;
    
    /**
     * @brief Get number of worker threads
     * @return Number of threads
     */
    int get_thread_count() const { return static_cast<int>(workers.size()); }
    
    /**
     * @brief Stop all worker threads
     */
    void stop();
    
    /**
     * @brief Restart thread pool with new thread count
     * @param num_threads New number of threads
     */
    void restart(int num_threads);

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop_flag{false};
};

/**
 * @brief Main search engine class
 * 
 * Implements a comprehensive chess search engine with advanced algorithms,
 * move ordering, pruning techniques, and parallel search capabilities.
 */
class Search {
    // Friend class for testing private members
    friend class SearchTester;
    
public:
    /**
     * @brief Default constructor
     */
    Search();
    
    /**
     * @brief Constructor with custom configuration
     * @param config Search configuration
     */
    explicit Search(const SearchConfig& config);
    
    /**
     * @brief Destructor
     */
    ~Search();
    
    // Core search interface
    
    /**
     * @brief Search for best move with fixed depth
     * @param board Position to search
     * @param depth Maximum search depth
     * @return Best move found
     */
    Move search_move(const Board& board, int depth);
    
    /**
     * @brief Search for best move with time limit
     * @param board Position to search
     * @param time_limit Maximum search time
     * @param max_depth Maximum search depth (optional limit)
     * @return Best move found
     */
    Move search_move(const Board& board, std::chrono::milliseconds time_limit, int max_depth = 50);
    
    /**
     * @brief Search with full result and fixed depth
     * @param board Position to search
     * @param depth Maximum search depth
     * @return Complete search result
     */
    SearchResult search(const Board& board, int depth);
    
    /**
     * @brief Search with full result and time limit
     * @param board Position to search
     * @param time_limit Maximum search time
     * @param max_depth Maximum search depth (optional limit)
     * @return Complete search result
     */
    SearchResult search(const Board& board, std::chrono::milliseconds time_limit, int max_depth = 50);
    
    // Configuration and control
    
    /**
     * @brief Set number of search threads
     * @param thread_count Number of threads (1 for single-threaded)
     */
    void set_thread_count(int thread_count);
    
    /**
     * @brief Get current thread count
     * @return Number of search threads
     */
    int get_thread_count() const;
    
    /**
     * @brief Stop current search operation
     */
    void stop_search();
    
    /**
     * @brief Check if search should be stopped
     * @return true if search should stop
     */
    bool should_stop() const;
    
    /**
     * @brief Get current search configuration
     * @return Reference to search configuration
     */
    const SearchConfig& get_config() const { return config; }
    
    /**
     * @brief Update search configuration
     * @param new_config New configuration
     */
    void set_config(const SearchConfig& new_config);
    
    /**
     * @brief Clear all search tables and history
     */
    void clear_tables();

private:
    // Core search algorithms
    
    /**
     * @brief Main iterative deepening search
     * @param board Position to search
     * @param max_depth Maximum depth
     * @param time_limit Time limit (0 for no limit)
     * @return Search result
     */
    SearchResult iterative_deepening(const Board& board, int max_depth, 
                                   std::chrono::milliseconds time_limit);
    
    /**
     * @brief Alpha-beta search with pruning
     * @param board Current position
     * @param depth Remaining depth
     * @param alpha Alpha bound
     * @param beta Beta bound
     * @param ply Current ply from root
     * @param pv Principal variation
     * @return Position evaluation
     */
    int alpha_beta(Board& board, int depth, int alpha, int beta, int ply, 
                   std::vector<Move>& pv);
    
    /**
     * @brief Alpha-beta search with multiple PV collection
     * @param board Current position
     * @param depth Remaining depth
     * @param alpha Alpha bound
     * @param beta Beta bound
     * @param ply Current ply from root
     * @param pv_lines Vector to store multiple PV lines
     * @param max_pv_lines Maximum number of PV lines to collect (default 3)
     * @return Position evaluation
     */

    
    /**
     * @brief Quiescence search for tactical stability
     * @param board Current position
     * @param alpha Alpha bound
     * @param beta Beta bound
     * @param ply Current ply
     * @return Position evaluation
     */
    int quiescence_search(Board& board, int alpha, int beta, int ply, std::vector<Move>& pv);
    
    /**
     * @brief Aspiration window search
     * @param board Position to search
     * @param depth Search depth
     * @param prev_score Previous iteration score
     * @param pv Principal variation
     * @return Search score
     */
    int aspiration_search(Board& board, int depth, int prev_score, std::vector<Move>& pv);
    
    // Move ordering
    
    /**
     * @brief Order moves for optimal search performance
     * @param board Current position
     * @param moves List of moves to order
     * @param ply Current ply
     * @param tt_move Hash move from transposition table
     */
    void order_moves(const Board& board, std::vector<MoveScore>& moves, 
                     int ply, const Move& tt_move);
    
    /**
     * @brief Calculate MVV-LVA score for capture
     * @param move Capture move
     * @return MVV-LVA score
     */
    int calculate_mvv_lva_score(const Move& move) const;
    
    /**
     * @brief Calculate SEE (Static Exchange Evaluation) score
     * @param board Position
     * @param move Move to evaluate
     * @return SEE score
     */
    int calculate_see_score(const Board& board, const Move& move) const;
    
    // Search extensions and reductions
    
    /**
     * @brief Calculate search extensions for move
     * @param board Current position
     * @param move Move being searched
     * @param ply Current ply
     * @param extensions_used Extensions used in current path
     * @return Extension amount (in plies)
     */
    int calculate_extensions(const Board& board, const Move& move, int ply, int extensions_used);
    
    /**
     * @brief Calculate late move reduction
     * @param depth Current depth
     * @param move_count Number of moves searched
     * @param move Current move
     * @param is_pv_node Whether this is a PV node
     * @return Reduction amount
     */
    int calculate_lmr_reduction(int depth, int move_count, const Move& move, bool is_pv_node);
    
    /**
     * @brief Check if null move pruning is applicable
     * @param board Current position
     * @param depth Current depth
     * @param beta Beta bound
     * @return true if null move can be tried
     */
    bool can_do_null_move(const Board& board, int depth, int beta);
    
    /**
     * @brief Check if futility pruning is applicable
     * @param board Current position
     * @param depth Current depth
     * @param alpha Alpha bound
     * @return true if move can be pruned
     */
    bool can_futility_prune(const Board& board, int depth, int alpha);
    
    // Time management
    
    /**
     * @brief Check if time limit has been exceeded and set stop flag if needed
     * Should be called at the entry of every recursive function
     */
    void check_time_and_stop();
    
    /**
     * @brief Check if time limit has been exceeded
     * @return true if search should stop due to time
     */
    bool time_expired() const;
    
    /**
     * @brief Update time management statistics
     */
    void update_time_management();
    
    // Timer worker functions
    
    /**
     * @brief Timer worker function
     * @param time_limit Time limit for the search
     */
    void timer_worker(std::chrono::milliseconds time_limit);
    
    /**
     * @brief Start timer thread
     * @param time_limit Time limit for the search
     */
    void start_timer(std::chrono::milliseconds time_limit);
    
    /**
     * @brief Stop timer thread
     */
    void stop_timer();
    
    // Parallel search support
    
    /**
     * @brief Lazy SMP parallel search - launches multiple independent ID searches
     * @param board Position to search
     * @param depth Search depth
     * @param num_instances Number of independent search instances
     * @param pv Principal variation output
     * @return Best score found across all instances
     */
    int lazy_smp_search(const Board& board, int depth, int num_instances, std::vector<Move>& pv);
    
    /**
     * @brief Parallel root search - efficiently distribute root moves among threads
     * @param board Starting position
     * @param depth Search depth
     * @param alpha Alpha bound
     * @param beta Beta bound
     * @param pv Principal variation output
     * @return Best score found
     */
    int parallel_root_search(Board& board, int depth, int alpha, int beta, std::vector<Move>& pv);
    
    /**
     * @brief Parallel search worker function
     * @param board Position to search
     * @param depth Search depth
     * @param alpha Alpha bound
     * @param beta Beta bound
     * @param moves Moves to search
     * @param start_index Starting move index
     * @param end_index Ending move index
     * @return Best score found
     */
    int parallel_search_worker(Board board, int depth, int alpha, int beta,
                              const std::vector<MoveScore>& moves,
                              int start_index, int end_index);
    
    // Member variables
    SearchConfig config;                   ///< Search configuration
    std::unique_ptr<TranspositionTable> tt; ///< Transposition table
    std::unique_ptr<Evaluation> evaluator; ///< Position evaluator
    std::unique_ptr<MoveGenerator> move_gen; ///< Move generator
    std::unique_ptr<ThreadPool> thread_pool; ///< Thread pool for parallel search
    
    // Search state
    std::atomic<bool> stop_flag{false};    ///< Stop search flag
    std::chrono::steady_clock::time_point search_start_time; ///< Search start time
    std::chrono::milliseconds allocated_time{0}; ///< Allocated search time
    
    // Move ordering tables
    KillerMoves killer_moves;              ///< Killer move table
    HistoryTable history_table;            ///< History heuristic table
    CounterMoveTable counter_move_table;   ///< Counter-move table
    
    // Previous move tracking for counter-moves (per ply)
    std::array<Move, 64> previous_moves;   ///< Track previous moves by ply for counter-move heuristic
    
    // Search statistics
    SearchStats current_stats;             ///< Current search statistics
    
    // YBWC split point tracking
    std::atomic<int> active_split_points{0}; ///< Number of active split points
    std::mutex split_point_mutex;          ///< Mutex for split point management
    
    // Thread synchronization
    mutable std::mutex search_mutex;       ///< Mutex for search state
    std::condition_variable search_cv;     ///< Condition variable for search
    
    // Timer thread for time management
    std::unique_ptr<std::thread> timer_thread;
    std::mutex timer_mutex;
    std::condition_variable timer_cv;
    std::atomic<bool> timer_should_exit{false};
    
    // Principal variation
    std::vector<std::vector<Move>> pv_table; ///< PV table for each ply
    
    // Constants
    static constexpr int MATE_SCORE = 30000;     ///< Mate score
    static constexpr int MAX_PLY = 64;           ///< Maximum search ply (reduced to prevent stack overflow)
    static constexpr int MAX_QUIESCENCE_PLY = 16; ///< Maximum quiescence search depth
    static constexpr int FUTILITY_MARGIN = 100;  ///< Futility pruning margin
    static constexpr int RAZOR_MARGIN = 300;     ///< Razoring margin
};

// Template implementation for ThreadPool::submit
template<typename F, typename... Args>
auto ThreadPool::submit(F&& task, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
    using return_type = typename std::invoke_result<F, Args...>::type;
    
    auto packaged_task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(task), std::forward<Args>(args)...)
    );
    
    std::future<return_type> result = packaged_task->get_future();
    
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (stop_flag) {
            throw std::runtime_error("ThreadPool is stopped");
        }
        tasks.emplace([packaged_task](){ (*packaged_task)(); });
    }
    
    condition.notify_one();
    return result;
}

#endif // SEARCH_H