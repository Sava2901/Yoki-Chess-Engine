#ifndef BITBOARD_H
#define BITBOARD_H

#include <cstdint>
#include <array>
#include <string>

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
    NO_SQUARE = 64
};

class BitboardUtils {
public:
    // Initialize magic bitboard tables
    static void init();
    
    // Basic bitboard operations
    static bool get_bit(Bitboard bb, int square) {
        return bb & (1ULL << square);
    }
    
    static void set_bit(Bitboard& bb, int square) {
        bb |= (1ULL << square);
    }
    
    static void clear_bit(Bitboard& bb, int square) {
        bb &= ~(1ULL << square);
    }
    
    static void toggle_bit(Bitboard& bb, int square) {
        bb ^= (1ULL << square);
    }
    
    // Population count (number of set bits)
    static int popcount(Bitboard bb) {
        return __builtin_popcountll(bb);
    }
    
    // Find first set bit (least significant bit)
    static int lsb(Bitboard bb) {
        return __builtin_ctzll(bb);
    }
    
    // Find most significant bit
    static int msb(Bitboard bb) {
        return 63 - __builtin_clzll(bb);
    }
    
    // Remove least significant bit
    static Bitboard pop_lsb(Bitboard& bb) {
        int square = lsb(bb);
        bb &= bb - 1;
        return square;
    }
    
    // Convert square coordinates to square index
    static int square_index(int rank, int file) {
        return rank * 8 + file;
    }
    
    // Convert square index to rank/file
    static int get_rank(int square) {
        return square / 8;
    }
    
    static int get_file(int square) {
        return square % 8;
    }
    
    // Sliding piece attacks
    static Bitboard rook_attacks(int square, Bitboard occupancy);
    static Bitboard bishop_attacks(int square, Bitboard occupancy);
    static Bitboard queen_attacks(int square, Bitboard occupancy);
    
    // Non-sliding piece attacks
    static Bitboard knight_attacks(int square);
    static Bitboard king_attacks(int square);
    static Bitboard pawn_attacks(int square, bool is_white);
    
    // Utility functions
    static std::string bitboard_to_string(Bitboard bb);
    static void print_bitboard(Bitboard bb);
    static int get_lsb_index(Bitboard bb) { return __builtin_ctzll(bb); }
    
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