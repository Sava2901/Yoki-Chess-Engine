#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#include <cstdint>
#include <memory>
#include <atomic>
#include <algorithm>
#include <shared_mutex>
#include "../board/Move.h"

// Forward declarations
class Board;

/**
 * @brief Enumeration for transposition table bound types
 * 
 * Defines the type of bound stored in a transposition table entry:
 * - EXACT: Exact score from a PV node
 * - LOWER_BOUND: Beta cutoff (fail-high), score >= beta
 * - UPPER_BOUND: Alpha cutoff (fail-low), score <= alpha
 */
enum class TTBoundType : uint8_t {
    EXACT = 0,       ///< Exact score (PV node)
    LOWER_BOUND = 1, ///< Beta cutoff (fail-high)
    UPPER_BOUND = 2  ///< Alpha cutoff (fail-low)
};

/**
 * @brief Enumeration for transposition table replacement strategies
 * 
 * Defines how entries are replaced when the table is full:
 * - DEPTH_PREFERRED: Always prefer entries with higher depth
 * - AGE_PREFERRED: Always prefer newer entries
 * - HYBRID: Combine depth and age considerations (default)
 */
enum class TTReplacementStrategy : uint8_t {
    DEPTH_PREFERRED = 0,  ///< Always prefer higher depth
    AGE_PREFERRED = 1,    ///< Always prefer newer entries
    HYBRID = 2           ///< Combine depth and age (default)
};

/**
 * @brief Transposition table entry structure
 * 
 * Thread-safe 16-byte cache-aligned structure for storing position evaluations.
 * Uses atomic operations to prevent race conditions in multithreaded access.
 */
struct alignas(16) TTEntry {
    std::atomic<uint64_t> key;           ///< 64-bit Zobrist hash key (atomic)
    std::atomic<uint32_t> move;          ///< Packed move representation (atomic)
    std::atomic<int16_t> score;          ///< Position evaluation score (atomic)
    std::atomic<uint8_t> depth;          ///< Search depth for this entry (atomic)
    std::atomic<uint8_t> flags;          ///< Packed bound_type (2 bits) and age (6 bits) (atomic)
    
    /**
     * @brief Default constructor - creates empty entry
     */
    TTEntry() : key(0), move(0), score(0), depth(0), flags(0) {}
    
    /**
     * @brief Copy constructor for atomic members
     */
    TTEntry(const TTEntry& other) 
        : key(other.key.load(std::memory_order_acquire))
        , move(other.move.load(std::memory_order_acquire))
        , score(other.score.load(std::memory_order_acquire))
        , depth(other.depth.load(std::memory_order_acquire))
        , flags(other.flags.load(std::memory_order_acquire)) {}
    
    /**
     * @brief Assignment operator for atomic members
     */
    TTEntry& operator=(const TTEntry& other) {
        if (this != &other) {
            key.store(other.key.load(std::memory_order_acquire), std::memory_order_release);
            move.store(other.move.load(std::memory_order_acquire), std::memory_order_release);
            score.store(other.score.load(std::memory_order_acquire), std::memory_order_release);
            depth.store(other.depth.load(std::memory_order_acquire), std::memory_order_release);
            flags.store(other.flags.load(std::memory_order_acquire), std::memory_order_release);
        }
        return *this;
    }
    
    /**
     * @brief Check if entry is empty/unused
     * @return true if entry has never been used
     */
    bool is_empty() const {
        return key.load(std::memory_order_acquire) == 0;
    }
    
    /**
     * @brief Get bound type from packed flags (thread-safe)
     * @return Bound type (EXACT/LOWER/UPPER)
     */
    TTBoundType get_bound_type() const { 
        return static_cast<TTBoundType>(flags.load(std::memory_order_acquire) & 0x3); 
    }
    
    /**
     * @brief Get age from packed flags (thread-safe)
     * @return Entry age (0-63)
     */
    uint8_t get_age() const { 
        return (flags.load(std::memory_order_acquire) >> 2) & 0x3F; 
    }
    
    /**
     * @brief Set bound type in packed flags (thread-safe)
     * @param type Bound type to set
     */
    void set_bound_type(TTBoundType type) { 
        uint8_t current_flags, new_flags;
        do {
            current_flags = flags.load(std::memory_order_acquire);
            new_flags = (current_flags & 0xFC) | static_cast<uint8_t>(type);
        } while (!flags.compare_exchange_weak(current_flags, new_flags, 
                                            std::memory_order_release, 
                                            std::memory_order_acquire));
    }
    
    /**
     * @brief Set age in packed flags (thread-safe)
     * @param age Age to set (0-63)
     */
    void set_age(uint8_t age) { 
        uint8_t current_flags, new_flags;
        do {
            current_flags = flags.load(std::memory_order_acquire);
            new_flags = (current_flags & 0x3) | ((age & 0x3F) << 2);
        } while (!flags.compare_exchange_weak(current_flags, new_flags, 
                                            std::memory_order_release, 
                                            std::memory_order_acquire));
    }
    
    /**
     * @brief Atomically store all entry data (thread-safe)
     * @param zobrist_key Hash key
     * @param packed_move Packed move data
     * @param eval_score Evaluation score
     * @param search_depth Search depth
     * @param bound_type Bound type
     * @param entry_age Entry age
     */
    void store_atomic(uint64_t zobrist_key, uint32_t packed_move, int16_t eval_score, 
                     uint8_t search_depth, TTBoundType bound_type, uint8_t entry_age) {
        // Store in specific order to ensure consistency
        uint8_t new_flags = (static_cast<uint8_t>(bound_type) & 0x3) | ((entry_age & 0x3F) << 2);
        
        // Store non-key fields first
        move.store(packed_move, std::memory_order_relaxed);
        score.store(eval_score, std::memory_order_relaxed);
        depth.store(search_depth, std::memory_order_relaxed);
        flags.store(new_flags, std::memory_order_relaxed);
        
        // Store key last with release semantics to make entry visible
        key.store(zobrist_key, std::memory_order_release);
    }
    
    /**
     * @brief Atomically load all entry data (thread-safe)
     * @param zobrist_key Output hash key
     * @param packed_move Output packed move data
     * @param eval_score Output evaluation score
     * @param search_depth Output search depth
     * @param bound_type Output bound type
     * @param entry_age Output entry age
     * @return true if entry is valid and consistent
     */
    bool load_atomic(uint64_t& zobrist_key, uint32_t& packed_move, int16_t& eval_score,
                    uint8_t& search_depth, TTBoundType& bound_type, uint8_t& entry_age) const {
        // Load key first with acquire semantics
        zobrist_key = key.load(std::memory_order_acquire);
        if (zobrist_key == 0) {
            return false; // Empty entry
        }
        
        // Load other fields
        packed_move = move.load(std::memory_order_relaxed);
        eval_score = score.load(std::memory_order_relaxed);
        search_depth = depth.load(std::memory_order_relaxed);
        uint8_t flag_value = flags.load(std::memory_order_relaxed);
        
        // Verify key hasn't changed (detect concurrent writes)
        if (key.load(std::memory_order_acquire) != zobrist_key) {
            return false; // Entry was modified during read
        }
        
        bound_type = static_cast<TTBoundType>(flag_value & 0x3);
        entry_age = (flag_value >> 2) & 0x3F;
        return true;
    }
};

/**
 * @brief Transposition table bucket structure
 * 
 * 64-byte cache-line aligned bucket containing 4 TTEntry structures.
 * This reduces hash collisions and improves cache performance.
 */
struct alignas(64) TTBucket {
    static constexpr int BUCKET_SIZE = 4;
    TTEntry entries[BUCKET_SIZE];  ///< 4 entries per bucket
    
    /**
     * @brief Default constructor - initializes empty bucket
     */
    TTBucket() = default;
    
    /**
     * @brief Find entry with matching key (thread-safe)
     * @param key Zobrist hash key to search for
     * @return Pointer to matching entry or nullptr if not found
     */
    TTEntry* find_entry(uint64_t key) {
        for (int i = 0; i < BUCKET_SIZE; ++i) {
            if (entries[i].key.load(std::memory_order_acquire) == key) {
                return &entries[i];
            }
        }
        return nullptr;
    }
    
    /**
     * @brief Find best replacement candidate
     * @param current_age Current search age
     * @return Index of best entry to replace
     */
    int find_replacement_index(uint8_t current_age) {
        int best_idx = 0;
        int best_score = replacement_score(entries[0], current_age);
        
        for (int i = 1; i < BUCKET_SIZE; ++i) {
            int score = replacement_score(entries[i], current_age);
            if (score > best_score) {
                best_score = score;
                best_idx = i;
            }
        }
        return best_idx;
    }
    
private:
    /**
     * @brief Calculate replacement score for entry (thread-safe)
     * @param entry Entry to evaluate
     * @param current_age Current search age
     * @return Replacement score (higher = better candidate for replacement)
     */
    int replacement_score(const TTEntry& entry, uint8_t current_age) {
        int age_diff = (current_age - entry.get_age()) & 0x3F;
        return (age_diff << 8) - entry.depth.load(std::memory_order_acquire);  // Prefer old entries and shallow depths
    }
};

/**
 * @brief Transposition table statistics structure
 * 
 * Contains performance metrics and diagnostic information.
 */
struct TTStats {
    uint64_t probes;        ///< Total probe operations
    uint64_t hits;          ///< Successful hits
    uint64_t cutoffs;       ///< Alpha-beta cutoffs from TT
    uint64_t collisions;    ///< Hash collisions detected
    double fill_rate;       ///< Percentage of table filled
    double hit_rate;        ///< Hit rate percentage
    
    /**
     * @brief Default constructor
     */
    TTStats() : probes(0), hits(0), cutoffs(0), collisions(0), fill_rate(0.0), hit_rate(0.0) {}
};

/**
 * @brief Transposition table probe result
 * 
 * Contains the result of a transposition table lookup.
 */
struct TTResult {
    bool found = false;                     ///< Whether a usable entry was found
    bool can_cutoff = false;                ///< Whether this enables alpha-beta cutoff
    Move move;                              ///< Best move from TT
    int score = 0;                         ///< Position evaluation (adjusted for ply)
    TTBoundType bound_type = TTBoundType::EXACT; ///< Type of bound
    uint8_t depth = 0;                     ///< Depth of stored evaluation
    uint8_t age = 0;                       ///< Age of the entry
    
    /**
     * @brief Default constructor
     */
    TTResult() = default;
};

/**
 * @brief Main transposition table class
 * 
 * High-performance hash table for caching chess position evaluations.
 * Features:
 * - Thread-safe lock-free operations
 * - Configurable size (16MB to 32GB)
 * - Multi-bucket collision handling
 * - Advanced replacement strategies
 * - Comprehensive statistics
 * - Cache-optimized memory layout
 */
class TranspositionTable {
private:
    std::unique_ptr<TTBucket[]> table_;     ///< Hash table buckets
    size_t bucket_count_;                   ///< Number of buckets
    size_t size_mb_;                       ///< Table size in MB
    uint8_t age_;                          ///< Current search age (0-255)
    TTReplacementStrategy replacement_strategy_; ///< Replacement strategy
    
    // Thread-safe statistics
    mutable TTStats stats_;
    mutable std::shared_mutex table_mutex_;     ///< Mutex for thread-safe access
    
    // Constants
    static constexpr size_t MIN_SIZE_MB = 1;     ///< Minimum table size
    static constexpr size_t MAX_SIZE_MB = 32768; ///< Maximum table size (32GB)
    static constexpr int MATE_SCORE = 30000;     ///< Mate score threshold
    
public:
    /**
     * @brief Constructor with configurable size
     * 
     * @param size_mb Hash table size in megabytes (default: 64MB)
     */
    explicit TranspositionTable(size_t size_mb = 64);
    
    /**
     * @brief Destructor
     */
    ~TranspositionTable();
    
    // Disable copy constructor and assignment
    TranspositionTable(const TranspositionTable&) = delete;
    TranspositionTable& operator=(const TranspositionTable&) = delete;
    
    /**
     * @brief Probe transposition table for position
     * 
     * Thread-safe lookup operation that searches for a stored evaluation
     * of the given position.
     * 
     * @param zobrist_key 64-bit Zobrist hash of position
     * @param depth Minimum required search depth
     * @param alpha Alpha bound for cutoff detection
     * @param beta Beta bound for cutoff detection
     * @param ply Current ply from root for mate score adjustment
     * @return TTResult containing hit information and data
     */
    TTResult probe(uint64_t zobrist_key, int depth, int alpha, int beta, int ply);
    
    /**
     * @brief Store position evaluation in transposition table
     * 
     * Thread-safe store operation that saves a position evaluation
     * with appropriate replacement strategy.
     * 
     * @param zobrist_key 64-bit Zobrist hash of position
     * @param best_move Best move found for this position
     * @param score Position evaluation score
     * @param depth Search depth for this evaluation
     * @param ply Current ply from root for mate score storage
     * @param bound_type Type of bound (EXACT/LOWER/UPPER)
     */
    void store(uint64_t zobrist_key, const Move& best_move, int score, 
               int depth, int ply, TTBoundType bound_type);
    
    /**
     * @brief Resize transposition table
     * 
     * Changes the table size and clears all entries.
     * 
     * @param new_size_mb New size in megabytes
     * @return true if resize was successful
     */
    bool resize(size_t new_size_mb);
    
    /**
     * @brief Clear all entries in the table
     * 
     * Resets all entries to empty state and resets statistics.
     */
    void clear();
    
    /**
     * @brief Start new search iteration
     * 
     * Increments the age counter for the new search.
     * This helps with replacement strategy decisions.
     */
    void new_search();
    
    /**
     * @brief Get transposition table statistics
     * 
     * Returns current performance statistics including hit rates,
     * collision counts, and memory utilization metrics.
     * 
     * @return TTStats Current statistics snapshot
     */
    TTStats get_statistics() const;
    
    /**
     * @brief Reset statistics
     */
    void reset_stats();
    
    /**
     * @brief Get table size in MB
     * 
     * @return Current table size in megabytes
     */
    size_t get_size_mb() const;
    
    /**
     * @brief Get bucket count
     * 
     * @return Number of buckets in the table
     */
    size_t get_bucket_count() const;
    
    /**
     * @brief Get hash full percentage
     * 
     * Estimates how full the hash table is by sampling entries.
     * 
     * @return Percentage full (0-1000, where 1000 = 100.0%)
     */
    int get_hashfull() const;
    
    /**
     * @brief Set replacement strategy
     * 
     * @param strategy New replacement strategy to use
     */
    void set_replacement_strategy(TTReplacementStrategy strategy);
    
    /**
     * @brief Prefetch bucket for given key (optimization)
     * 
     * Hints to the processor to prefetch the cache line containing
     * the bucket for the given key. This can reduce memory latency.
     * 
     * @param zobrist_key Key to prefetch bucket for
     */
    void prefetch(uint64_t zobrist_key);

private:
    /**
     * @brief Calculate bucket index from Zobrist key
     * 
     * @param zobrist_hash 64-bit Zobrist hash
     * @return Bucket index
     */
    size_t get_bucket_index(uint64_t zobrist_key) const;
    
    /**
     * @brief Find best replacement candidate using current strategy
     * 
     * @param bucket Bucket to search in
     * @param depth Current search depth
     * @return Index of best entry to replace
     */
    int find_replacement_index(const TTBucket& bucket, int depth);
    
    /**
     * @brief Adjust mate scores for storage in TT
     * 
     * @param score Raw score
     * @param ply Current ply
     * @return Adjusted score for storage
     */
    int adjust_mate_score_to_tt(int score, int ply);
    
    /**
     * @brief Adjust mate scores when retrieving from TT
     * 
     * @param score Stored score
     * @param ply Current ply
     * @return Adjusted score for current position
     */
    int adjust_mate_score_from_tt(int score, int ply);
    
    /**
     * @brief Allocate aligned memory for hash table
     * 
     * @param bucket_count Number of buckets to allocate
     * @return true if allocation successful
     */
    bool allocate_memory(size_t bucket_count);
    
    /**
     * @brief Deallocate hash table memory
     */
    void deallocate_memory();
};

// Global transposition table instance
// Use function to get global transposition table instance (lazy initialization)
TranspositionTable& get_global_transposition_table();

#endif // TRANSPOSITION_TABLE_H