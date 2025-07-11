#include "Bitboard.h"
#include <iostream>
#include <iomanip>
#include <random>

// Static member initialization
std::array<Bitboard, 64> BitboardUtils::rook_magics;
std::array<Bitboard, 64> BitboardUtils::bishop_magics;
std::array<int, 64> BitboardUtils::rook_shifts;
std::array<int, 64> BitboardUtils::bishop_shifts;
std::array<Bitboard*, 64> BitboardUtils::rook_attacks_table;
std::array<Bitboard*, 64> BitboardUtils::bishop_attacks_table;
std::array<Bitboard, 64> BitboardUtils::knight_attacks_table;
std::array<Bitboard, 64> BitboardUtils::king_attacks_table;
std::array<Bitboard, 64> BitboardUtils::white_pawn_attacks_table;
std::array<Bitboard, 64> BitboardUtils::black_pawn_attacks_table;
bool BitboardUtils::is_initialized = false;

// Magic numbers for rook attacks (pre-computed)
static constexpr std::array<Bitboard, 64> ROOK_MAGICS = {
    0x0080001020400080ULL, 0x0040001000200040ULL, 0x0080081000200080ULL, 0x0080040800100080ULL,
    0x0080020400080080ULL, 0x0080010200040080ULL, 0x0080008001000200ULL, 0x0080002040800100ULL,
    0x0000800020400080ULL, 0x0000400020005000ULL, 0x0000801000200080ULL, 0x0000800800100080ULL,
    0x0000800400080080ULL, 0x0000800200040080ULL, 0x0000800100020080ULL, 0x0000800040800100ULL,
    0x0000208000400080ULL, 0x0000404000201000ULL, 0x0000808010002000ULL, 0x0000808008001000ULL,
    0x0000808004000800ULL, 0x0000808002000400ULL, 0x0000010100020004ULL, 0x0000020000408104ULL,
    0x0000208080004000ULL, 0x0000200040005000ULL, 0x0000100080200080ULL, 0x0000080080100080ULL,
    0x0000040080080080ULL, 0x0000020080040080ULL, 0x0000010080800200ULL, 0x0000800080004100ULL,
    0x0000204000800080ULL, 0x0000200040401000ULL, 0x0000100080802000ULL, 0x0000080080801000ULL,
    0x0000040080800800ULL, 0x0000020080800400ULL, 0x0000020001010004ULL, 0x0000800040800100ULL,
    0x0000204000808000ULL, 0x0000200040008080ULL, 0x0000100020008080ULL, 0x0000080010008080ULL,
    0x0000040008008080ULL, 0x0000020004008080ULL, 0x0000010002008080ULL, 0x0000004081020004ULL,
    0x0000204000800080ULL, 0x0000200040008080ULL, 0x0000100020008080ULL, 0x0000080010008080ULL,
    0x0000040008008080ULL, 0x0000020004008080ULL, 0x0000800100020080ULL, 0x0000800041000080ULL,
    0x00FFFCDDFCED714AULL, 0x007FFCDDFCED714AULL, 0x003FFFCDFFD88096ULL, 0x0000040810002101ULL,
    0x0001000204080011ULL, 0x0001000204000801ULL, 0x0001000082000401ULL, 0x0001FFFAABFAD1A2ULL
};

// Magic numbers for bishop attacks (pre-computed)
static constexpr std::array<Bitboard, 64> BISHOP_MAGICS = {
    0x0002020202020200ULL, 0x0002020202020000ULL, 0x0004010202000000ULL, 0x0004040080000000ULL,
    0x0001104000000000ULL, 0x0000821040000000ULL, 0x0000410410400000ULL, 0x0000104104104000ULL,
    0x0000040404040400ULL, 0x0000020202020200ULL, 0x0000040102020000ULL, 0x0000040400800000ULL,
    0x0000011040000000ULL, 0x0000008210400000ULL, 0x0000004104104000ULL, 0x0000002082082000ULL,
    0x0004000808080800ULL, 0x0002000404040400ULL, 0x0001000202020200ULL, 0x0000800802004000ULL,
    0x0000800400A00000ULL, 0x0000200100884000ULL, 0x0000400082082000ULL, 0x0000200041041000ULL,
    0x0002080010101000ULL, 0x0001040008080800ULL, 0x0000208004010400ULL, 0x0000404004010200ULL,
    0x0000840000802000ULL, 0x0000404002011000ULL, 0x0000808001041000ULL, 0x0000404000820800ULL,
    0x0001041000202000ULL, 0x0000820800101000ULL, 0x0000104400080800ULL, 0x0000020080080080ULL,
    0x0000404040040100ULL, 0x0000808100020100ULL, 0x0001010100020800ULL, 0x0000808080010400ULL,
    0x0000820820004000ULL, 0x0000410410002000ULL, 0x0000082088001000ULL, 0x0000002011000800ULL,
    0x0000080100400400ULL, 0x0001010101000200ULL, 0x0002020202000400ULL, 0x0001010101000200ULL,
    0x0000410410400000ULL, 0x0000208208200000ULL, 0x0000002084100000ULL, 0x0000000020880000ULL,
    0x0000001002020000ULL, 0x0000040408020000ULL, 0x0004040404040000ULL, 0x0002020202020000ULL,
    0x0000104104104000ULL, 0x0000002082082000ULL, 0x0000000020841000ULL, 0x0000000000208800ULL,
    0x0000000010020200ULL, 0x0000000404080200ULL, 0x0000040404040400ULL, 0x0002020202020200ULL
};

// Shift values for magic bitboards
static const std::array<int, 64> ROOK_SHIFTS = {
    52, 53, 53, 53, 53, 53, 53, 52,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    52, 53, 53, 53, 53, 53, 53, 52
};

static const std::array<int, 64> BISHOP_SHIFTS = {
    58, 59, 59, 59, 59, 59, 59, 58,
    59, 59, 59, 59, 59, 59, 59, 59,
    59, 59, 57, 57, 57, 57, 59, 59,
    59, 59, 57, 55, 55, 57, 59, 59,
    59, 59, 57, 55, 55, 57, 59, 59,
    59, 59, 57, 57, 57, 57, 59, 59,
    59, 59, 59, 59, 59, 59, 59, 59,
    58, 59, 59, 59, 59, 59, 59, 58
};

// Attack tables storage
static Bitboard rook_table[102400];
static Bitboard bishop_table[5248];

void BitboardUtils::init() {
    if (is_initialized) return;
    
    // Copy pre-computed magic numbers
    rook_magics = ROOK_MAGICS;
    bishop_magics = BISHOP_MAGICS;
    rook_shifts = ROOK_SHIFTS;
    bishop_shifts = BISHOP_SHIFTS;
    
    init_rook_attacks();
    init_bishop_attacks();
    init_knight_attacks();
    init_king_attacks();
    init_pawn_attacks();
    
    is_initialized = true;
}

void BitboardUtils::init_rook_attacks() {
    int table_index = 0;
    
    for (int square = 0; square < 64; square++) {
        rook_attacks_table[square] = &rook_table[table_index];
        
        Bitboard mask = rook_mask(square);
        int shift = rook_shifts[square];
        int num_bits = popcount(mask);
        
        for (int i = 0; i < (1 << num_bits); i++) {
            Bitboard occupancy = 0;
            Bitboard temp_mask = mask;
            
            // Generate occupancy variation
            for (int bit = 0; bit < num_bits; bit++) {
                int lsb_square = pop_lsb(temp_mask);
                if (i & (1 << bit)) {
                    set_bit(occupancy, lsb_square);
                }
            }
            
            int magic_index = (occupancy * rook_magics[square]) >> shift;
            rook_attacks_table[square][magic_index] = generate_rook_attacks_slow(square, occupancy);
        }
        
        table_index += (1 << (64 - shift));
    }
}

void BitboardUtils::init_bishop_attacks() {
    int table_index = 0;
    
    for (int square = 0; square < 64; square++) {
        bishop_attacks_table[square] = &bishop_table[table_index];
        
        Bitboard mask = bishop_mask(square);
        int shift = bishop_shifts[square];
        int num_bits = popcount(mask);
        
        for (int i = 0; i < (1 << num_bits); i++) {
            Bitboard occupancy = 0;
            Bitboard temp_mask = mask;
            
            // Generate occupancy variation
            for (int bit = 0; bit < num_bits; bit++) {
                int lsb_square = pop_lsb(temp_mask);
                if (i & (1 << bit)) {
                    set_bit(occupancy, lsb_square);
                }
            }
            
            int magic_index = (occupancy * bishop_magics[square]) >> shift;
            bishop_attacks_table[square][magic_index] = generate_bishop_attacks_slow(square, occupancy);
        }
        
        table_index += (1 << (64 - shift));
    }
}

void BitboardUtils::init_knight_attacks() {
    for (int square = 0; square < 64; square++) {
        Bitboard attacks = 0;
        int rank = get_rank(square);
        int file = get_file(square);
        
        // Knight move offsets
        int knight_moves[8][2] = {
            {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
            {1, -2}, {1, 2}, {2, -1}, {2, 1}
        };
        
        for (auto & knight_move : knight_moves) {
            int new_rank = rank + knight_move[0];
            int new_file = file + knight_move[1];
            
            if (new_rank >= 0 && new_rank < 8 && new_file >= 0 && new_file < 8) {
                set_bit(attacks, square_index(new_rank, new_file));
            }
        }
        
        knight_attacks_table[square] = attacks;
    }
}

void BitboardUtils::init_king_attacks() {
    for (int square = 0; square < 64; square++) {
        Bitboard attacks = 0;
        int rank = get_rank(square);
        int file = get_file(square);
        
        // King move offsets
        int king_moves[8][2] = {
            {-1, -1}, {-1, 0}, {-1, 1},
            {0, -1},           {0, 1},
            {1, -1},  {1, 0},  {1, 1}
        };
        
        for (auto & king_move : king_moves) {
            int new_rank = rank + king_move[0];
            int new_file = file + king_move[1];
            
            if (new_rank >= 0 && new_rank < 8 && new_file >= 0 && new_file < 8) {
                set_bit(attacks, square_index(new_rank, new_file));
            }
        }
        
        king_attacks_table[square] = attacks;
    }
}

void BitboardUtils::init_pawn_attacks() {
    for (int square = 0; square < 64; square++) {
        Bitboard white_attacks = 0;
        Bitboard black_attacks = 0;
        int rank = get_rank(square);
        int file = get_file(square);
        
        // White pawn attacks (moving up the board)
        if (rank < 7) {
            if (file > 0) set_bit(white_attacks, square_index(rank + 1, file - 1));
            if (file < 7) set_bit(white_attacks, square_index(rank + 1, file + 1));
        }
        
        // Black pawn attacks (moving down the board)
        if (rank > 0) {
            if (file > 0) set_bit(black_attacks, square_index(rank - 1, file - 1));
            if (file < 7) set_bit(black_attacks, square_index(rank - 1, file + 1));
        }
        
        white_pawn_attacks_table[square] = white_attacks;
        black_pawn_attacks_table[square] = black_attacks;
    }
}

Bitboard BitboardUtils::rook_mask(int square) {
    Bitboard mask = 0;
    int rank = get_rank(square);
    int file = get_file(square);
    
    // Horizontal mask (excluding edges)
    for (int f = 1; f < 7; f++) {
        if (f != file) {
            set_bit(mask, square_index(rank, f));
        }
    }
    
    // Vertical mask (excluding edges)
    for (int r = 1; r < 7; r++) {
        if (r != rank) {
            set_bit(mask, square_index(r, file));
        }
    }
    
    return mask;
}

Bitboard BitboardUtils::bishop_mask(int square) {
    Bitboard mask = 0;
    int rank = get_rank(square);
    int file = get_file(square);
    
    // Diagonal directions
    int directions[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    
    for (int dir = 0; dir < 4; dir++) {
        int r = rank + directions[dir][0];
        int f = file + directions[dir][1];
        
        while (r > 0 && r < 7 && f > 0 && f < 7) {
            set_bit(mask, square_index(r, f));
            r += directions[dir][0];
            f += directions[dir][1];
        }
    }
    
    return mask;
}

Bitboard BitboardUtils::generate_rook_attacks_slow(int square, Bitboard occupancy) {
    Bitboard attacks = 0;
    int rank = get_rank(square);
    int file = get_file(square);
    
    // Horizontal directions
    for (int f = file + 1; f < 8; f++) {
        int target_square = square_index(rank, f);
        set_bit(attacks, target_square);
        if (get_bit(occupancy, target_square)) break;
    }
    
    for (int f = file - 1; f >= 0; f--) {
        int target_square = square_index(rank, f);
        set_bit(attacks, target_square);
        if (get_bit(occupancy, target_square)) break;
    }
    
    // Vertical directions
    for (int r = rank + 1; r < 8; r++) {
        int target_square = square_index(r, file);
        set_bit(attacks, target_square);
        if (get_bit(occupancy, target_square)) break;
    }
    
    for (int r = rank - 1; r >= 0; r--) {
        int target_square = square_index(r, file);
        set_bit(attacks, target_square);
        if (get_bit(occupancy, target_square)) break;
    }
    
    return attacks;
}

Bitboard BitboardUtils::generate_bishop_attacks_slow(int square, Bitboard occupancy) {
    Bitboard attacks = 0;
    int rank = get_rank(square);
    int file = get_file(square);
    
    // Diagonal directions
    int directions[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    
    for (int dir = 0; dir < 4; dir++) {
        int r = rank + directions[dir][0];
        int f = file + directions[dir][1];
        
        while (r >= 0 && r < 8 && f >= 0 && f < 8) {
            int target_square = square_index(r, f);
            set_bit(attacks, target_square);
            if (get_bit(occupancy, target_square)) break;
            r += directions[dir][0];
            f += directions[dir][1];
        }
    }
    
    return attacks;
}

Bitboard BitboardUtils::rook_attacks(int square, Bitboard occupancy) {
    occupancy &= rook_mask(square);
    int magic_index = (occupancy * rook_magics[square]) >> rook_shifts[square];
    return rook_attacks_table[square][magic_index];
}

Bitboard BitboardUtils::bishop_attacks(int square, Bitboard occupancy) {
    occupancy &= bishop_mask(square);
    int magic_index = (occupancy * bishop_magics[square]) >> bishop_shifts[square];
    return bishop_attacks_table[square][magic_index];
}

Bitboard BitboardUtils::queen_attacks(int square, Bitboard occupancy) {
    return rook_attacks(square, occupancy) | bishop_attacks(square, occupancy);
}

Bitboard BitboardUtils::knight_attacks(int square) {
    return knight_attacks_table[square];
}

Bitboard BitboardUtils::king_attacks(int square) {
    return king_attacks_table[square];
}

Bitboard BitboardUtils::pawn_attacks(int square, bool is_white) {
    return is_white ? white_pawn_attacks_table[square] : black_pawn_attacks_table[square];
}

std::string BitboardUtils::bitboard_to_string(Bitboard bb) {
    std::string result;
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int square = square_index(rank, file);
            result += get_bit(bb, square) ? '1' : '0';
            result += ' ';
        }
        result += '\n';
    }
    return result;
}

void BitboardUtils::print_bitboard(Bitboard bb) {
    std::cout << bitboard_to_string(bb) << std::endl;
}