#ifndef BITBOARD_H
#define BITBOARD_H

#include <cstdint>
#include <array>
#include <string>
#include <immintrin.h>  // For BMI2 and POPCNT intrinsics
// TODO: Check the functions for each technology

using Bitboard = uint64_t;

// Faster bitboard operations using compiler intrinsics
#ifdef __BMI2__
    #define pop_lsb(bb) _blsr_u64(bb)
#endif

#ifdef __POPCNT__
    #define count_bits(bb) _mm_popcnt_u64(bb)
#else
    #define count_bits(bb) __builtin_popcountll(bb)
#endif

#ifdef __BMI__
    #define get_lsb(bb) _tzcnt_u64(bb)
#else
    #define get_lsb(bb) __builtin_ctzll(bb)
#endif

// Bitboard constants
constexpr Bitboard EMPTY_BOARD = 0ULL;
constexpr Bitboard FULL_BOARD = 0xFFFFFFFFFFFFFFFFULL;

// File and rank masks
constexpr Bitboard FILE_A = 0x0101010101010101ULL;
constexpr Bitboard FILE_B = 0x0202020202020202ULL;
constexpr Bitboard FILE_C = 0x0404040404040404ULL;
constexpr Bitboard FILE_D = 0x0808080808080808ULL;
constexpr Bitboard FILE_E = 0x1010101010101010ULL;
constexpr Bitboard FILE_F = 0x2020202020202020ULL;
constexpr Bitboard FILE_G = 0x4040404040404040ULL;
constexpr Bitboard FILE_H = 0x8080808080808080ULL;

constexpr Bitboard RANK_1 = 0x00000000000000FFULL;
constexpr Bitboard RANK_2 = 0x000000000000FF00ULL;
constexpr Bitboard RANK_3 = 0x0000000000FF0000ULL;
constexpr Bitboard RANK_4 = 0x00000000FF000000ULL;
constexpr Bitboard RANK_5 = 0x000000FF00000000ULL;
constexpr Bitboard RANK_6 = 0x0000FF0000000000ULL;
constexpr Bitboard RANK_7 = 0x00FF000000000000ULL;
constexpr Bitboard RANK_8 = 0xFF00000000000000ULL;

// Square indices (0-63)
enum Square {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    NO_SQUARE = 64
};

class BitboardUtils {
public:
    /**
     * Initialize magic bitboard tables and precomputed attack tables.
     * Must be called once before using any attack generation functions.
     */
    static void init();
    
    // ========== Basic Bitboard Operations ==========
    
    /**
     * Check if a specific bit is set in the bitboard.
     * @param bb The bitboard to check
     * @param square The square index (0-63) to check
     * @return true if the bit at the given square is set, false otherwise
     */
    static bool get_bit(Bitboard bb, int square) {
        return bb & (1ULL << square);
    }
    
    /**
     * Set a specific bit in the bitboard.
     * @param bb Reference to the bitboard to modify
     * @param square The square index (0-63) to set
     */
    static void set_bit(Bitboard& bb, int square) {
        bb |= (1ULL << square);
    }
    
    /**
     * Clear a specific bit in the bitboard.
     * @param bb Reference to the bitboard to modify
     * @param square The square index (0-63) to clear
     */
    static void clear_bit(Bitboard& bb, int square) {
        bb &= ~(1ULL << square);
    }
    
    /**
     * Toggle a specific bit in the bitboard.
     * @param bb Reference to the bitboard to modify
     * @param square The square index (0-63) to toggle
     */
    static void toggle_bit(Bitboard& bb, int square) {
        bb ^= (1ULL << square);
    }
    
    // ========== Bit Manipulation Functions ==========
    
    /**
     * Count the number of set bits in the bitboard (population count).
     * Uses hardware POPCNT instruction when available for optimal performance.
     * @param bb The bitboard to count
     * @return The number of set bits (0-64)
     */
    static int popcount(Bitboard bb) {
        return count_bits(bb);
    }
    
    /**
     * Find the index of the least significant bit (rightmost set bit).
     * Uses hardware BMI instruction when available for optimal performance.
     * @param bb The bitboard to search (must not be zero)
     * @return The square index (0-63) of the least significant bit
     */
    static int lsb(Bitboard bb) {
        return get_lsb(bb);
    }
    
    /**
     * Find the index of the most significant bit (leftmost set bit).
     * @param bb The bitboard to search (must not be zero)
     * @return The square index (0-63) of the most significant bit
     */
    static int msb(Bitboard bb) {
        return 63 - __builtin_clzll(bb);
    }
    
    /**
     * Remove and return the least significant bit from the bitboard.
     * Uses hardware BMI2 instruction when available for optimal performance.
     * @param bb Reference to the bitboard to modify
     * @return The square index (0-63) of the removed bit
     */
    static Bitboard pop_lsb(Bitboard& bb) {
#ifdef __BMI2__
        int square = get_lsb(bb);
        bb = _blsr_u64(bb);
        return square;
#else
        int square = lsb(bb);
        bb &= bb - 1;
        return square;
#endif
    }
    
    // ========== Coordinate Conversion Functions ==========
    
    /**
     * Convert rank and file coordinates to a square index.
     * @param rank The rank (0-7, where 0 is rank 1)
     * @param file The file (0-7, where 0 is file A)
     * @return The square index (0-63)
     */
    static int square_index(int rank, int file) {
        return rank * 8 + file;
    }
    
    /**
     * Extract the rank from a square index.
     * @param square The square index (0-63)
     * @return The rank (0-7, where 0 is rank 1)
     */
    static int get_rank(int square) {
        return square / 8;
    }
    
    /**
     * Extract the file from a square index.
     * @param square The square index (0-63)
     * @return The file (0-7, where 0 is file A)
     */
    static int get_file(int square) {
        return square % 8;
    }
    
    // ========== Attack Generation Functions ==========
    
    /**
     * Generate rook attack bitboard for a given square and occupancy.
     * Uses magic bitboards for fast lookup.
     * @param square The square index (0-63) where the rook is located
     * @param occupancy Bitboard representing all occupied squares
     * @return Bitboard of squares the rook can attack
     */
    static Bitboard rook_attacks(int square, Bitboard occupancy);
    
    /**
     * Generate bishop attack bitboard for a given square and occupancy.
     * Uses magic bitboards for fast lookup.
     * @param square The square index (0-63) where the bishop is located
     * @param occupancy Bitboard representing all occupied squares
     * @return Bitboard of squares the bishop can attack
     */
    static Bitboard bishop_attacks(int square, Bitboard occupancy);
    
    /**
     * Generate queen attack bitboard for a given square and occupancy.
     * Combines rook and bishop attacks.
     * @param square The square index (0-63) where the queen is located
     * @param occupancy Bitboard representing all occupied squares
     * @return Bitboard of squares the queen can attack
     */
    static Bitboard queen_attacks(int square, Bitboard occupancy);
    
    // PEXT bitboard attacks (BMI2 optimized)
    #ifdef __BMI2__
    /**
     * Generate rook attacks using BMI2 PEXT instruction (faster on modern CPUs).
     * @param square The square index (0-63) where the rook is located
     * @param occupancy Bitboard representing all occupied squares
     * @return Bitboard of squares the rook can attack
     */
    static Bitboard rook_attacks_pext(int square, Bitboard occupancy);
    
    /**
     * Generate bishop attacks using BMI2 PEXT instruction (faster on modern CPUs).
     * @param square The square index (0-63) where the bishop is located
     * @param occupancy Bitboard representing all occupied squares
     * @return Bitboard of squares the bishop can attack
     */
    static Bitboard bishop_attacks_pext(int square, Bitboard occupancy);
    #endif
    
    /**
     * Generate knight attack bitboard for a given square.
     * Knight attacks are independent of occupancy.
     * @param square The square index (0-63) where the knight is located
     * @return Bitboard of squares the knight can attack
     */
    static Bitboard knight_attacks(int square);
    
    /**
     * Generate king attack bitboard for a given square.
     * King attacks are independent of occupancy.
     * @param square The square index (0-63) where the king is located
     * @return Bitboard of squares the king can attack
     */
    static Bitboard king_attacks(int square);
    
    /**
     * Generate pawn attack bitboard for a single pawn.
     * Pawn attacks are diagonal captures only, not forward moves.
     * @param square The square index (0-63) where the pawn is located
     * @param is_white true for white pawn, false for black pawn
     * @return Bitboard of squares the pawn can attack (capture)
     */
    static Bitboard pawn_attacks(int square, bool is_white);
    
    // ========== Utility Functions ==========
    
    /**
     * Convert a bitboard to a human-readable string representation.
     * Shows the board from white's perspective (rank 8 at top).
     * @param bb The bitboard to convert
     * @return String representation of the bitboard
     */
    static std::string bitboard_to_string(Bitboard bb);
    
    /**
     * Print a bitboard to console in a human-readable format.
     * Shows the board from white's perspective (rank 8 at top).
     * @param bb The bitboard to print
     */
    static void print_bitboard(Bitboard bb);
    
    /**
     * Alias for lsb() function - get least significant bit index.
     * @param bb The bitboard to search (must not be zero)
     * @return The square index (0-63) of the least significant bit
     */
    static int get_lsb_index(Bitboard bb) { return get_lsb(bb); }
    
    // ========== Magic Bitboard Data Accessors ==========
    
    /**
     * Get the magic number for rook attacks at a given square.
     * @param square The square index (0-63)
     * @return The magic number used for rook attack generation
     */
    static Bitboard get_rook_magic(int square) { return rook_magics[square]; }
    
    /**
     * Get the magic number for bishop attacks at a given square.
     * @param square The square index (0-63)
     * @return The magic number used for bishop attack generation
     */
    static Bitboard get_bishop_magic(int square) { return bishop_magics[square]; }
    
    /**
     * Get the shift value for rook magic bitboard indexing.
     * @param square The square index (0-63)
     * @return The shift value used in rook magic bitboard calculations
     */
    static int get_rook_shift(int square) { return rook_shifts[square]; }
    
    /**
     * Get the shift value for bishop magic bitboard indexing.
     * @param square The square index (0-63)
     * @return The shift value used in bishop magic bitboard calculations
     */
    static int get_bishop_shift(int square) { return bishop_shifts[square]; }
    
    /**
     * Get pointer to the rook attacks lookup table for a given square.
     * @param square The square index (0-63)
     * @return Pointer to the rook attacks table for the square
     */
    static Bitboard* get_rook_attacks_table(int square) { return rook_attacks_table[square]; }
    
    /**
     * Get pointer to the bishop attacks lookup table for a given square.
     * @param square The square index (0-63)
     * @return Pointer to the bishop attacks table for the square
     */
    static Bitboard* get_bishop_attacks_table(int square) { return bishop_attacks_table[square]; }
    
    /**
     * Get the rook movement mask for a given square (excludes edge squares).
     * @param square The square index (0-63)
     * @return Bitboard mask of relevant squares for rook movement
     */
    static Bitboard get_rook_mask(int square) { return rook_mask(square); }
    
    /**
     * Get the bishop movement mask for a given square (excludes edge squares).
     * @param square The square index (0-63)
     * @return Bitboard mask of relevant squares for bishop movement
     */
    static Bitboard get_bishop_mask(int square) { return bishop_mask(square); }
    
private:
    // Magic bitboard tables
    static std::array<Bitboard, 64> rook_magics;
    static std::array<Bitboard, 64> bishop_magics;
    static std::array<int, 64> rook_shifts;
    static std::array<int, 64> bishop_shifts;
    static std::array<Bitboard*, 64> rook_attacks_table;
    static std::array<Bitboard*, 64> bishop_attacks_table;
    
    // Pre-computed attack tables
    static std::array<Bitboard, 64> knight_attacks_table;
    static std::array<Bitboard, 64> king_attacks_table;
    static std::array<Bitboard, 64> white_pawn_attacks_table;
    static std::array<Bitboard, 64> black_pawn_attacks_table;
    
    // Mask generation
    static Bitboard rook_mask(int square);
    static Bitboard bishop_mask(int square);
    
    // Magic bitboard initialization helpers
    static void init_rook_attacks();
    static void init_bishop_attacks();
    static void init_knight_attacks();
    static void init_king_attacks();
    static void init_pawn_attacks();
    
    static Bitboard generate_rook_attacks_slow(int square, Bitboard occupancy);
    static Bitboard generate_bishop_attacks_slow(int square, Bitboard occupancy);
    
    static bool is_initialized;
};

#endif // BITBOARD_H