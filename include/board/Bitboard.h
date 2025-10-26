#ifndef BITBOARD_H
#define BITBOARD_H

#include <cstdint>
#include <array>
#include <string>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>

// Architecture detection
#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__)
    constexpr bool Is64Bit = true;
#else
    constexpr bool Is64Bit = false;
#endif

// Compiler detection
#if defined(__GNUC__) || defined(__clang__)
    #define COMPILER_GCC_COMPATIBLE
#elif defined(_MSC_VER)
    #define COMPILER_MSVC
    #include <intrin.h>
#endif

// PEXT instruction availability (BMI2)
#if defined(__BMI2__) || (defined(_MSC_VER) && defined(__AVX2__))
    #define USE_PEXT
    #if defined(_MSC_VER)
        #include <immintrin.h>
        #define pext(a, b) _pext_u64(a, b)
    #else
        #include <x86intrin.h>
        #define pext(a, b) __builtin_ia32_pext_di(a, b)
    #endif
    constexpr bool HasPext = true;
#else
    constexpr bool HasPext = false;
#endif

// POPCNT instruction availability
#if defined(__POPCNT__) || (defined(_MSC_VER) && defined(__AVX__))
    #define USE_POPCNT
#endif

// Bitboard type definition
using Bitboard = uint64_t;

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
    NO_SQUARE = 64,
    SQUARE_NB = 64
};

// Increment operator for Square enum
constexpr Square& operator++(Square& s) { return s = Square(int(s) + 1); }
constexpr Square operator++(Square& s, int) { Square old = s; ++s; return old; }

// PieceType enum for magic bitboards indexing
enum PieceType : int {
    BISHOP = 0,
    ROOK = 1
};

// Direction enum for bitboard shifts
enum Direction : int {
    NORTH = 8,
    SOUTH = -8,
    EAST = 1,
    WEST = -1,
    NORTH_EAST = NORTH + EAST,
    NORTH_WEST = NORTH + WEST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST
};

// Forward declarations
namespace BitboardUtils {
    void init();
    std::string bitboard_to_string(Bitboard b);
}

// Global lookup tables
extern uint8_t PopCnt16[1 << 16];
extern uint8_t SquareDistance[SQUARE_NB][SQUARE_NB];

// Additional lookup tables for advanced features
extern Bitboard BetweenBB[SQUARE_NB][SQUARE_NB];
extern Bitboard LineBB[SQUARE_NB][SQUARE_NB];

// Magic structure for magic bitboards
struct Magic {
    Bitboard  mask;
    Bitboard* attacks;
#ifndef USE_PEXT
    Bitboard magic;
    unsigned shift;
#endif

    // Compute the attack's index using the 'magic bitboards' approach
    [[nodiscard]] unsigned index(Bitboard occupied) const {
#ifdef USE_PEXT
        return unsigned(pext(occupied, mask));
#else
        if (Is64Bit)
            return unsigned(((occupied & mask) * magic) >> shift);

        unsigned lo = unsigned(occupied) & unsigned(mask);
        unsigned hi = unsigned(occupied >> 32) & unsigned(mask >> 32);
        return (lo * unsigned(magic) ^ hi * unsigned(magic >> 32)) >> shift;
#endif
    }

    [[nodiscard]] Bitboard attacks_bb(Bitboard occupied) const {
        return attacks[index(occupied)]; 
    }
};

// Global magic arrays - [square][piece_type: 0=BISHOP, 1=ROOK]
extern Magic Magics[SQUARE_NB][2];

// Pre-computed attack tables
extern std::array<Bitboard, 64> knight_attacks_table;
extern std::array<Bitboard, 64> king_attacks_table;
extern std::array<Bitboard, 64> white_pawn_attacks_table;
extern std::array<Bitboard, 64> black_pawn_attacks_table;

// ========== Inline Bitboard Utility Functions ==========

// Constexpr helpers for square operations
constexpr Bitboard square_bb(Square s) {
    assert(s >= A1 && s <= H8);
    return (1ULL << s);
}

// Bitwise operators between Bitboard and Square
constexpr Bitboard  operator&(Bitboard b, Square s) { return b & square_bb(s); }
constexpr Bitboard  operator|(Bitboard b, Square s) { return b | square_bb(s); }
constexpr Bitboard  operator^(Bitboard b, Square s) { return b ^ square_bb(s); }
constexpr Bitboard& operator|=(Bitboard& b, Square s) { return b |= square_bb(s); }
constexpr Bitboard& operator^=(Bitboard& b, Square s) { return b ^= square_bb(s); }

constexpr Bitboard operator&(Square s, Bitboard b) { return b & s; }
constexpr Bitboard operator|(Square s, Bitboard b) { return b | s; }
constexpr Bitboard operator^(Square s, Bitboard b) { return b ^ s; }

constexpr Bitboard operator|(Square s1, Square s2) { return square_bb(s1) | s2; }

// Check if more than one bit is set
constexpr bool more_than_one(Bitboard b) { return b & (b - 1); }

// Get rank and file bitboards
constexpr Bitboard rank_bb(int r) { return RANK_1 << (8 * r); }
constexpr Bitboard file_bb(int f) { return FILE_A << f; }

// Convert rank and file to square index
constexpr int square_index(int rank, int file) { return rank * 8 + file; }

// Extract rank and file from square
constexpr int get_rank(int square) { return square / 8; }
constexpr int get_file(int square) { return square % 8; }

// Basic bit operations
inline bool get_bit(Bitboard bb, int square) {
    return bb & (1ULL << square);
}

inline void set_bit(Bitboard& bb, int square) {
    bb |= (1ULL << square);
}

inline void clear_bit(Bitboard& bb, int square) {
    bb &= ~(1ULL << square);
}

inline void toggle_bit(Bitboard& bb, int square) {
    bb ^= (1ULL << square);
}

// Population count (number of set bits)
inline int popcount(Bitboard b) {
#ifndef USE_POPCNT
    std::uint16_t indices[4];
    std::memcpy(indices, &b, sizeof(b));
    return PopCnt16[indices[0]] + PopCnt16[indices[1]] + PopCnt16[indices[2]] + PopCnt16[indices[3]];
#elif defined(COMPILER_MSVC)
    return int(_mm_popcnt_u64(b));
#else  // GCC or compatible compiler
    return __builtin_popcountll(b);
#endif
}

// Least significant bit index
inline int lsb(Bitboard b) {
    assert(b);
#if defined(COMPILER_GCC_COMPATIBLE)
    return __builtin_ctzll(b);
#elif defined(COMPILER_MSVC)
    #ifdef _WIN64
        unsigned long idx;
        _BitScanForward64(&idx, b);
        return int(idx);
    #else
        unsigned long idx;
        if (b & 0xffffffff) {
            _BitScanForward(&idx, int32_t(b));
            return int(idx);
        } else {
            _BitScanForward(&idx, int32_t(b >> 32));
            return int(idx + 32);
        }
    #endif
#else
    #error "Compiler not supported."
#endif
}

// Most significant bit index
inline int msb(Bitboard b) {
    assert(b);
#if defined(COMPILER_GCC_COMPATIBLE)
    return 63 ^ __builtin_clzll(b);
#elif defined(COMPILER_MSVC)
    #ifdef _WIN64
        unsigned long idx;
        _BitScanReverse64(&idx, b);
        return int(idx);
    #else
        unsigned long idx;
        if (b >> 32) {
            _BitScanReverse(&idx, int32_t(b >> 32));
            return int(idx + 32);
        } else {
            _BitScanReverse(&idx, int32_t(b));
            return int(idx);
        }
    #endif
#else
    #error "Compiler not supported."
#endif
}

// Least significant square bitboard
inline Bitboard least_significant_square_bb(Bitboard b) {
    assert(b);
    return b & -b;
}

// Pop least significant bit and return its index
inline int pop_lsb(Bitboard& b) {
    assert(b);
    const int s = lsb(b);
    b &= b - 1;
    return s;
}

// Alias for compatibility
inline int get_lsb_index(Bitboard bb) { return lsb(bb); }

// ========== Attack Generation Functions ==========

// Rook attacks using magic bitboards
inline Bitboard rook_attacks(int square, Bitboard occupancy) {
    return Magics[square][1].attacks_bb(occupancy);
}

// Bishop attacks using magic bitboards
inline Bitboard bishop_attacks(int square, Bitboard occupancy) {
    return Magics[square][0].attacks_bb(occupancy);
}

// Queen attacks (combination of rook and bishop)
inline Bitboard queen_attacks(int square, Bitboard occupancy) {
    return rook_attacks(square, occupancy) | bishop_attacks(square, occupancy);
}

// Knight attacks (precomputed)
inline Bitboard knight_attacks(int square) {
    return knight_attacks_table[square];
}

// King attacks (precomputed)
inline Bitboard king_attacks(int square) {
    return king_attacks_table[square];
}

// Pawn attacks (precomputed)
inline Bitboard pawn_attacks(int square, bool is_white) {
    return is_white ? white_pawn_attacks_table[square] : black_pawn_attacks_table[square];
}

// Get bitboard of squares between two squares (exclusive)
inline Bitboard between_squares(int sq1, int sq2) {
    return BetweenBB[sq1][sq2];
}

// Check if three squares are aligned (on same rank, file, or diagonal)
inline bool aligned(int sq1, int sq2, int sq3) {
    return LineBB[sq1][sq2] & (1ULL << sq3);
}

// ========== Bitboard Utilities Namespace ==========

namespace BitboardUtils {
/**
 * Convert a bitboard to a human-readable string representation.
 * @param bb The bitboard to convert
 * @return String representation of the bitboard
 */
std::string bitboard_to_string(Bitboard bb);

/**
 * Print a bitboard to console in a human-readable format.
 * @param bb The bitboard to print
 */
void print_bitboard(Bitboard bb);

// Helper functions for initialization (used internally)

/**
 * Initialize magic bitboard structures for sliding pieces.
 * @param pt Piece type (BISHOP or ROOK)
 * @param table Attack table storage
 * @param magics Reference to Magic array for storing results
 */
void init_magics(PieceType pt, Bitboard table[], Magic magics[][2]);
void init_rook_attacks();
void init_bishop_attacks();
void init_knight_attacks();
void init_king_attacks();
void init_pawn_attacks();

// Forwarding functions for backward compatibility
inline bool get_bit(Bitboard bb, int square) { return ::get_bit(bb, square); }
inline void set_bit(Bitboard& bb, int square) { ::set_bit(bb, square); }
inline void clear_bit(Bitboard& bb, int square) { ::clear_bit(bb, square); }
inline void toggle_bit(Bitboard& bb, int square) { ::toggle_bit(bb, square); }
inline int popcount(Bitboard b) { return ::popcount(b); }
inline int lsb(Bitboard b) { return ::lsb(b); }
inline int msb(Bitboard b) { return ::msb(b); }
inline int pop_lsb(Bitboard& b) { return ::pop_lsb(b); }
inline Bitboard least_significant_square_bb(Bitboard b) { return ::least_significant_square_bb(b); }
inline int get_lsb_index(Bitboard bb) { return ::get_lsb_index(bb); }
inline int square_index(int rank, int file) { return ::square_index(rank, file); }
inline int get_rank(int square) { return ::get_rank(square); }
inline int get_file(int square) { return ::get_file(square); }
inline Bitboard rook_attacks(int square, Bitboard occupancy) { return ::rook_attacks(square, occupancy); }
inline Bitboard bishop_attacks(int square, Bitboard occupancy) { return ::bishop_attacks(square, occupancy); }
inline Bitboard queen_attacks(int square, Bitboard occupancy) { return ::queen_attacks(square, occupancy); }
inline Bitboard knight_attacks(int square) { return ::knight_attacks(square); }
inline Bitboard king_attacks(int square) { return ::king_attacks(square); }
inline Bitboard pawn_attacks(int square, bool is_white) { return ::pawn_attacks(square, is_white); }
inline Bitboard between_squares(int sq1, int sq2) { return ::between_squares(sq1, sq2); }
inline bool aligned(int sq1, int sq2, int sq3) { return ::aligned(sq1, sq2, sq3); }

} // namespace BitboardUtils

#endif // BITBOARD_H