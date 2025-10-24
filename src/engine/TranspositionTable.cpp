#include "../../include/engine/TranspositionTable.h"
#include "../../include/board/Board.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>

// Platform-specific includes for memory alignment and prefetch
#ifdef _WIN32
#include <windows.h>
#include <malloc.h>
#include <intrin.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

// Constants for mate score handling
constexpr int MATE_SCORE = 32000;
constexpr int MATE_BOUND = MATE_SCORE - 1000;

// Constructor
TranspositionTable::TranspositionTable(size_t size_mb) 
    : table_(nullptr), 
      size_mb_(0), 
      bucket_count_(0),
      age_(0),
      replacement_strategy_(TTReplacementStrategy::DEPTH_PREFERRED) {
    resize(size_mb);
}

// Destructor
TranspositionTable::~TranspositionTable() {
    if (table_) {
        deallocate_memory();
    }
}

// Resize the transposition table
bool TranspositionTable::resize(size_t size_mb) {
    // Validate size (must be power of 2 and within reasonable bounds)
    if (size_mb < 1 || size_mb > 32768) {
        return false;
    }
    
    // Round to nearest power of 2
    size_t target_size = 1;
    while (target_size < size_mb) {
        target_size <<= 1;
    }
    if (target_size > size_mb * 2) {
        target_size >>= 1;
    }
    
    // Calculate bucket count
    size_t total_bytes = target_size * 1024 * 1024;
    size_t new_bucket_count = total_bytes / sizeof(TTBucket);
    
    // Ensure bucket count is power of 2 for efficient modulo
    size_t bucket_power = 1;
    while (bucket_power < new_bucket_count) {
        bucket_power <<= 1;
    }
    if (bucket_power > new_bucket_count * 2) {
        bucket_power >>= 1;
    }
    new_bucket_count = bucket_power;
    
    // Deallocate old memory
    if (table_) {
        deallocate_memory();
    }
    
    // Allocate new memory
    if (!allocate_memory(new_bucket_count)) {
        return false;
    }
    
    size_mb_ = target_size;
    bucket_count_ = new_bucket_count;
    
    // Clear the table
    clear();
    
    return true;
}

// Clear the transposition table
void TranspositionTable::clear() {
    if (table_) {
        std::memset(table_.get(), 0, bucket_count_ * sizeof(TTBucket));
        
        // Reset statistics
        stats_.probes = 0;
        stats_.hits = 0;
        stats_.cutoffs = 0;
        stats_.collisions = 0;
        stats_.fill_rate = 0.0;
        stats_.hit_rate = 0.0;
    }
}

// Probe the transposition table
TTResult TranspositionTable::probe(uint64_t zobrist_key, int depth, int alpha, int beta, int ply) {
    std::shared_lock<std::shared_mutex> lock(table_mutex_);
    
    if (!table_) {
        stats_.probes++;
        return TTResult{};
    }
    
    stats_.probes++;
    size_t bucket_index = get_bucket_index(zobrist_key);
    TTBucket& bucket = table_[bucket_index];

    // Search through all entries in the bucket
    for (int i = 0; i < 4; ++i) {
        TTEntry& entry = bucket.entries[i];
        
        // Use atomic load to safely read entry data
        uint64_t entry_key;
        uint32_t entry_move;
        int16_t entry_score;
        uint8_t entry_depth;
        TTBoundType bound_type;
        uint8_t entry_age;
        
        // Atomically load all entry data with consistency check
        if (!entry.load_atomic(entry_key, entry_move, entry_score, entry_depth, bound_type, entry_age)) {
            continue; // Entry is empty or being modified
        }
        
        // Check if this entry matches our position
        if (entry_key == zobrist_key) {
            // Update age to mark as recently accessed
            entry.set_age(age_);
            
            // Adjust mate scores by ply distance
            int adjusted_score = adjust_mate_score_from_tt(entry_score, ply);
            
            TTResult result;
            result.found = true;
            result.move.from_uint32(entry_move);
            result.score = adjusted_score;
            result.depth = entry_depth;
            result.bound_type = bound_type;
            result.age = entry_age;
            
            
            // Check if we can use this entry for a cutoff
            if (entry_depth >= depth) {
                if (bound_type == TTBoundType::EXACT ||
                    (bound_type == TTBoundType::LOWER_BOUND && adjusted_score >= beta) ||
                    (bound_type == TTBoundType::UPPER_BOUND && adjusted_score <= alpha)) {
                    result.can_cutoff = true;
                    stats_.cutoffs++;
                    stats_.hits++;
                }
            }
            
            return result;
        }
    }
    
    return TTResult{};
}

// Store an entry in the transposition table
void TranspositionTable::store(uint64_t zobrist_key, const Move& best_move, int score, 
                              int depth, int ply, TTBoundType bound_type) {
    std::unique_lock<std::shared_mutex> lock(table_mutex_);
    
    if (!table_) {
        return;
    }
    
    size_t bucket_index = get_bucket_index(zobrist_key);
    TTBucket& bucket = table_[bucket_index];
    
    // Adjust mate scores for storage
    int adjusted_score = adjust_mate_score_to_tt(score, ply);
    
    // Look for existing entry or find replacement candidate
    int replace_index = -1;
    
    // First, check if we already have this position
    for (int i = 0; i < 4; ++i) {
        if (bucket.entries[i].key == zobrist_key) {
            replace_index = i;
            break;
        }
    }
    
    // If not found, find the best replacement candidate using improved policy
    if (replace_index == -1) {
        // Look for empty slot first
        for (int i = 0; i < 4; ++i) {
            if (bucket.entries[i].key == 0) {
                replace_index = i;
                break;
            }
        }
        
        // If no empty slot, apply improved replacement policy:
        // Only overwrite if different key AND new search is deeper
        if (replace_index == -1) {
            for (int i = 0; i < 4; ++i) {
                TTEntry& entry = bucket.entries[i];
                if (entry.key != zobrist_key && depth > entry.depth) {
                    replace_index = i;
                    break;
                }
            }
            
            // If still no candidate found, use the old replacement strategy as fallback
            if (replace_index == -1) {
                replace_index = find_replacement_index(bucket, depth);
            }
        }
        
        if (replace_index != -1 && bucket.entries[replace_index].key != 0) {
            stats_.collisions++;
        }
    }
    
    // Store the entry atomically to prevent race conditions
    if (replace_index != -1) {
        TTEntry& entry = bucket.entries[replace_index];
        entry.store_atomic(zobrist_key, best_move.to_uint32(), 
                          static_cast<int16_t>(std::clamp(adjusted_score, -32767, 32767)),
                          static_cast<uint8_t>(std::clamp(depth, 0, 255)), 
                          bound_type, age_);
    }
}

// Prefetch a bucket for the given hash
void TranspositionTable::prefetch(uint64_t zobrist_key) {
    if (!table_) {
        return;
    }
    
    size_t bucket_index = get_bucket_index(zobrist_key);
    
#ifdef __builtin_prefetch
    __builtin_prefetch(&table_[bucket_index], 0, 3);
#elif defined(_MSC_VER)
    _mm_prefetch(reinterpret_cast<const char*>(&table_[bucket_index]), _MM_HINT_T0);
#else
    (void)bucket_index; // Suppress unused variable warning
#endif
}

// Advance the search age
void TranspositionTable::new_search() {
    age_ = (age_ + 1) & 0xFF; // Keep age in 8-bit range
}

// Get hash full (permille)
int TranspositionTable::get_hashfull() const {
    if (!table_ || bucket_count_ == 0) {
        return 0;
    }
    
    // Sample a portion of the table to estimate fullness
    const size_t sample_size = (std::min)(bucket_count_, static_cast<size_t>(1000));
    size_t filled_entries = 0;
    size_t total_entries = 0;
    
    for (size_t i = 0; i < sample_size; ++i) {
        const TTBucket& bucket = table_[i];
        for (int j = 0; j < 4; ++j) {
            total_entries++;
            if (bucket.entries[j].key != 0) {
                filled_entries++;
            }
        }
    }
    
    if (total_entries == 0) {
        return 0;
    }
    
    return static_cast<int>((filled_entries * 1000) / total_entries);
}

// Get current statistics
TTStats TranspositionTable::get_statistics() const {
    TTStats stats = stats_;

    // Calculate derived statistics
    if (stats.probes > 0) {
        stats.hit_rate = static_cast<double>(stats.hits) / stats.probes;
    }
    
    if (bucket_count_ > 0) {
        stats.fill_rate = static_cast<double>(get_hashfull()) / 1000.0;
    }
    
    return stats;
}

// Reset statistics
void TranspositionTable::reset_stats() {
    stats_.probes = 0;
    stats_.hits = 0;
    stats_.cutoffs = 0;
    stats_.collisions = 0;
    stats_.fill_rate = 0.0;
    stats_.hit_rate = 0.0;
}

// Set replacement strategy
void TranspositionTable::set_replacement_strategy(TTReplacementStrategy strategy) {
    replacement_strategy_ = strategy;
}

// Get current size in MB
size_t TranspositionTable::get_size_mb() const {
    return size_mb_;
}

// Get bucket count
size_t TranspositionTable::get_bucket_count() const {
    return bucket_count_;
}

// Private helper methods

size_t TranspositionTable::get_bucket_index(uint64_t zobrist_key) const {
    return zobrist_key & (bucket_count_ - 1);
}

int TranspositionTable::find_replacement_index(const TTBucket& bucket, int /* depth */) {
    int best_index = 0;
    int best_score = INT_MAX;
    
    for (int i = 0; i < 4; ++i) {
        const TTEntry& entry = bucket.entries[i];
        
        // Empty slot has highest priority
        if (entry.key == 0) {
            return i;
        }
        
        int score = 0;
        
        switch (replacement_strategy_) {
            case TTReplacementStrategy::DEPTH_PREFERRED:
                // Prefer replacing entries with lower depth
                score = entry.depth * 256 + (255 - entry.get_age());
                break;
                
            case TTReplacementStrategy::AGE_PREFERRED:
                // Prefer replacing older entries
                score = (255 - entry.get_age()) * 256 + entry.depth;
                break;
                
            case TTReplacementStrategy::HYBRID:
                // Balanced approach considering both depth and age
                score = entry.depth * 128 + (255 - entry.get_age()) * 128;
                break;
        }
        
        if (score < best_score) {
            best_score = score;
            best_index = i;
        }
    }
    
    return best_index;
}

int TranspositionTable::adjust_mate_score_to_tt(int score, int ply) {
    if (score > MATE_BOUND) {
        return score + ply;
    } else if (score < -MATE_BOUND) {
        return score - ply;
    }
    return score;
}

int TranspositionTable::adjust_mate_score_from_tt(int score, int ply) {
    if (score > MATE_BOUND) {
        return score - ply;
    } else if (score < -MATE_BOUND) {
        return score + ply;
    }
    return score;
}

bool TranspositionTable::allocate_memory(size_t bucket_count) {
    try {
        table_ = std::make_unique<TTBucket[]>(bucket_count);
        return true;
    } catch (const std::bad_alloc&) {
        table_.reset();
        return false;
    }
}

void TranspositionTable::deallocate_memory() {
    table_.reset();
}

// Global transposition table instance - use lazy initialization to avoid static initialization order issues
TranspositionTable& get_global_transposition_table() {
    static TranspositionTable instance(128); // Default 128MB - lazy initialization
    static std::once_flag initialized;
    
    // Ensure thread-safe initialization
    std::call_once(initialized, []() {
        // Additional initialization if needed
    });
    
    return instance;
}