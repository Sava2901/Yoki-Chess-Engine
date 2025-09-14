#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#include "../board/Move.h"
#include <cstdint>
#include <atomic>
#include <memory>
#include <mutex>

/**
 * @enum TTEntryType
 * @brief Types of transposition table entries for alpha-beta bounds
 */
enum class TTEntryType : uint8_t {
    EXACT = 0,       ///< Exact score (PV node)
    LOWER_BOUND = 1, ///< Beta cutoff (fail-high)
    UPPER_BOUND = 2  ///< Alpha cutoff (fail-low)
};

/**
 * @struct TTEntry
 * @brief Transposition table entry structure
 * 
 * Stores position information for the transposition table including
 * Zobrist key, search depth, evaluation score, bound type, best move,
 * and age for replacement policy.
 */
struct TTEntry {
    uint64_t key;        ///< Zobrist hash key for position verification
    int16_t depth;       ///< Search depth when this entry was stored
    int16_t score;       ///< Evaluation score (centipawns or mate score)
    uint8_t type;        ///< Entry type (TTEntryType cast to uint8_t)
    uint32_t move;       ///< Best move (encoded as uint32_t)
    uint8_t age;         ///< Age for replacement policy
    
    /**
     * @brief Default constructor - initializes entry as empty
     */
    TTEntry() : key(0), depth(-1), score(0), type(static_cast<uint8_t>(TTEntryType::EXACT)), move(0), age(0) {}
    
    /**
     * @brief Constructor with all parameters
     */
    TTEntry(uint64_t k, int16_t d, int16_t s, TTEntryType t, uint32_t m, uint8_t a)
        : key(k), depth(d), score(s), type(static_cast<uint8_t>(t)), move(m), age(a) {}
    
    /**
     * @brief Check if this entry is valid (has been initialized)
     */
    bool is_valid() const { return depth >= 0; }
    
    /**
     * @brief Get the entry type as enum
     */
    TTEntryType get_type() const { return static_cast<TTEntryType>(type); }
};

/**
 * @class TranspositionTable
 * @brief Lock-light transposition table with Zobrist hashing
 * 
 * Implements a high-performance transposition table for chess search
 * with the following features:
 * - Power-of-two sizing for fast indexing
 * - Lockless reads with atomic writes
 * - Replacement policy (replace shallower or older entries)
 * - Thread-safe access for multi-threaded search
 * - Age-based entry management
 */
class TranspositionTable {
public:
    /**
     * @brief Constructor with specified size
     * @param size_mb Size of the table in megabytes (will be rounded to power of 2)
     */
    explicit TranspositionTable(size_t size_mb = 64);
    
    /**
     * @brief Destructor
     */
    ~TranspositionTable() = default;
    
    /**
     * @brief Probe the transposition table for a position
     * @param key Zobrist hash key of the position
     * @param depth Current search depth
     * @param alpha Alpha bound
     * @param beta Beta bound
     * @param ply Current ply from root (for mate score adjustment)
     * @return Pointer to TTEntry if found and usable, nullptr otherwise
     */
    const TTEntry* probe(uint64_t key, int depth, int alpha, int beta, int ply) const;
    
    /**
     * @brief Store an entry in the transposition table
     * @param key Zobrist hash key of the position
     * @param depth Search depth
     * @param score Evaluation score
     * @param type Entry type (EXACT, LOWER_BOUND, UPPER_BOUND)
     * @param best_move Best move found (0 if none)
     * @param ply Current ply from root (for mate score adjustment)
     */
    void store(uint64_t key, int depth, int score, TTEntryType type, uint32_t best_move, int ply);
    
    /**
     * @brief Get the best move from TT for move ordering
     * @param key Zobrist hash key of the position
     * @return Best move if found, invalid move otherwise
     */
    Move get_pv_move(uint64_t key) const;
    
    /**
     * @brief Clear the entire transposition table
     */
    void clear();
    
    /**
     * @brief Increment the age counter (called at the start of each search)
     */
    void new_search() { current_age = (current_age + 1) & 0xFF; }
    
    /**
     * @brief Get table statistics
     */
    size_t get_size() const { return table_size; }
    size_t get_usage() const;
    
private:
    std::unique_ptr<std::atomic<TTEntry>[]> table;  ///< The hash table
    size_t table_size;                              ///< Size of the table (power of 2)
    size_t index_mask;                              ///< Mask for fast indexing (size - 1)
    uint8_t current_age;                            ///< Current age counter
    
    /**
     * @brief Get table index from hash key
     */
    size_t get_index(uint64_t key) const { return key & index_mask; }
    
    /**
     * @brief Adjust mate scores for storage/retrieval
     */
    int adjust_mate_score_for_storage(int score, int ply) const;
    int adjust_mate_score_for_retrieval(int score, int ply) const;
    
    /**
     * @brief Check if we should replace an existing entry
     */
    bool should_replace(const TTEntry& existing, int new_depth, uint8_t new_age) const;
};

#endif // TRANSPOSITION_TABLE_H