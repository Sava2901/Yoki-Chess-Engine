#include "TranspositionTable.h"
#include <algorithm>
#include <cstring>

static constexpr int MATE_BOUND = 29000;

TranspositionTable::TranspositionTable(size_t size_mb) : current_age(0) {
    // Calculate number of entries (each TTEntry is about 16 bytes)
    size_t target_entries = (size_mb * 1024 * 1024) / sizeof(TTEntry);
    
    // Round down to nearest power of 2
    table_size = 1;
    while (table_size <= target_entries / 2) {
        table_size <<= 1;
    }
    
    // Create index mask for fast modulo operation
    index_mask = table_size - 1;
    
    // Allocate the table and mutexes
    table = std::make_unique<TTEntry[]>(table_size);
    mutexes = std::make_unique<std::mutex[]>(table_size);
    
    // Initialize all entries
    clear();
}

const TTEntry* TranspositionTable::probe(uint64_t key, int depth, int alpha, int beta, int ply) const {
    size_t index = get_index(key);
    
    // Thread-safe read with mutex
    std::lock_guard<std::mutex> lock(mutexes[index]);
    const TTEntry& entry = table[index];
    
    // Check if the entry matches our position
    if (entry.key != key || !entry.is_valid()) {
        return nullptr;
    }
    
    // Check if the entry has sufficient depth
    if (entry.depth < depth) {
        return nullptr;
    }
    
    // Adjust mate scores from storage format
    int score = adjust_mate_score_for_retrieval(entry.score, ply);
    
    // Check if the entry can be used based on bound type
    switch (static_cast<TTEntryType>(entry.type)) {
        case TTEntryType::EXACT:
            // Exact score can always be used
            return &entry;
            
        case TTEntryType::LOWER_BOUND:
            // Lower bound (beta cutoff) - can use if score >= beta
            if (score >= beta) {
                return &entry;
            }
            break;
            
        case TTEntryType::UPPER_BOUND:
            // Upper bound (alpha cutoff) - can use if score <= alpha
            if (score <= alpha) {
                return &entry;
            }
            break;
    }
    
    return nullptr;
}

void TranspositionTable::store(uint64_t key, int depth, int score, TTEntryType type, uint32_t best_move, int ply) {
    size_t index = get_index(key);
    
    // Adjust mate scores for storage
    int adjusted_score = adjust_mate_score_for_storage(score, ply);
    
    // Create new entry
    TTEntry new_entry(key, static_cast<int16_t>(depth), static_cast<int16_t>(adjusted_score), 
                      type, best_move, current_age);
    
    // Thread-safe write with mutex
    std::lock_guard<std::mutex> lock(mutexes[index]);
    const TTEntry& existing = table[index];
    
    // Check if we should replace the existing entry
    if (!existing.is_valid() || should_replace(existing, depth, current_age)) {
        // Store the new entry
        table[index] = new_entry;
    }
}

Move TranspositionTable::get_pv_move(uint64_t key) const {
    size_t index = get_index(key);
    
    // Thread-safe read with mutex
    std::lock_guard<std::mutex> lock(mutexes[index]);
    const TTEntry& entry = table[index];

    // Check if entry matches and has a move
    if (entry.key == key && entry.is_valid() && entry.move != 0) {
        // Convert uint32_t back to Move
        Move move;
        move.from_uint32(entry.move);
        return move;
    }
    
    // Return invalid move if not found
    return {};
}

void TranspositionTable::clear() {
    // Clear all entries
    for (size_t i = 0; i < table_size; ++i) {
        std::lock_guard<std::mutex> lock(mutexes[i]);
        table[i] = TTEntry();
    }
    current_age = 0;
}

size_t TranspositionTable::get_usage() const {
    size_t used_entries = 0;
    
    // Sample a portion of the table to estimate usage
    size_t sample_size = std::min(table_size, static_cast<size_t>(1000));
    
    for (size_t i = 0; i < sample_size; ++i) {
        std::lock_guard<std::mutex> lock(mutexes[i]);
        const TTEntry& entry = table[i];
        if (entry.is_valid()) {
            used_entries++;
        }
    }
    
    // Extrapolate to full table
    return (used_entries * table_size) / sample_size;
}

int TranspositionTable::adjust_mate_score_for_storage(int score, int ply) const {
    if (score > MATE_BOUND) {
        // Mate in X moves - adjust to be relative to current position
        return score + ply;
    } else if (score < -MATE_BOUND) {
        // Mated in X moves - adjust to be relative to current position
        return score - ply;
    }
    return score;
}

int TranspositionTable::adjust_mate_score_for_retrieval(int score, int ply) const {
    if (score > MATE_BOUND) {
        // Mate in X moves - adjust back to be relative to root
        return score - ply;
    } else if (score < -MATE_BOUND) {
        // Mated in X moves - adjust back to be relative to root
        return score + ply;
    }
    return score;
}

bool TranspositionTable::should_replace(const TTEntry& existing, int new_depth, uint8_t new_age) const {
    // Always replace if existing entry is invalid
    if (!existing.is_valid()) {
        return true;
    }
    
    // Replace if new entry has greater depth
    if (new_depth > existing.depth) {
        return true;
    }
    
    // Replace if same depth but newer age
    if (new_depth == existing.depth && new_age != existing.age) {
        return true;
    }
    
    // Replace if existing entry is significantly older (age wraps around)
    uint8_t age_diff = (new_age - existing.age) & 0xFF;
    if (age_diff < 128 && age_diff > 4) {  // Existing entry is 4+ searches old
        return true;
    }
    
    return false;
}