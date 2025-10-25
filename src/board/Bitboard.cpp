#include "../../include/board/Bitboard.h"
#include <iostream>
#include <iomanip>
#include <random>
#include <bitset>
#include <initializer_list>
#include <cstring>

// Global lookup tables
Magic Magics[64][2];
std::array<Bitboard, 64> knight_attacks_table;
std::array<Bitboard, 64> king_attacks_table;
std::array<Bitboard, 64> white_pawn_attacks_table;
std::array<Bitboard, 64> black_pawn_attacks_table;
uint8_t PopCnt16[1 << 16];
uint8_t SquareDistance[64][64];

// Internal state tracking
static bool is_initialized = false;

// Attack tables storage 
static Bitboard rook_table[0x19000];   // To store rook attacks
static Bitboard bishop_table[0x1480];  // To store bishop attacks

// PRNG for magic number generation
class PRNG {
    uint64_t s;
    
    uint64_t rand64() {
        s ^= s >> 12;
        s ^= s << 25;
        s ^= s >> 27;
        return s * 2685821657736338717LL;
    }
    
public:
    PRNG(uint64_t seed) : s(seed) {}
    
    template<typename T>
    T sparse_rand() {
        return T(rand64() & rand64() & rand64());
    }
};

// Helper function to compute sliding attacks on-the-fly
namespace {

Bitboard safe_destination(Square s, int step) {
    Square to = Square(int(s) + step);
    int from_rank = get_rank(int(s));
    int from_file = get_file(int(s));
    int to_rank = get_rank(int(to));
    int to_file = get_file(int(to));
    
    if (int(to) < 0 || int(to) >= 64) return 0;
    
    int rank_dist = std::abs(to_rank - from_rank);
    int file_dist = std::abs(to_file - from_file);
    int distance = std::max(rank_dist, file_dist);
    
    return distance <= 2 ? (1ULL << int(to)) : 0ULL;
}

Bitboard sliding_attack(PieceType pt, Square sq, Bitboard occupied) {
    Bitboard attacks = 0;
    int directions[4];
    
    if (pt == ROOK) {
        // NORTH, SOUTH, EAST, WEST
        directions[0] = 8;
        directions[1] = -8;
        directions[2] = 1;
        directions[3] = -1;
    } else {
        // NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST
        directions[0] = 9;
        directions[1] = -7;
        directions[2] = -9;
        directions[3] = 7;
    }
    
    for (int i = 0; i < 4; i++) {
        int step = directions[i];
        Square s = sq;
        
        while (true) {
            Bitboard dest = safe_destination(s, step);
            if (!dest) break;
            
            s = Square(int(s) + step);
            attacks |= (1ULL << int(s));
            
            if (occupied & (1ULL << int(s))) break;
        }
    }
    
    return attacks;
}

}  // anonymous namespace

void BitboardUtils::init() {
    if (is_initialized) return;
    
    // Initialize PopCnt16 lookup table
    for (unsigned i = 0; i < (1 << 16); ++i)
        PopCnt16[i] = uint8_t(std::bitset<16>(i).count());
    
    // Initialize SquareDistance lookup table
    for (Square s1 = A1; s1 <= H8; ++s1)
        for (Square s2 = A1; s2 <= H8; ++s2) {
            int file_dist = std::abs(get_file(int(s1)) - get_file(int(s2)));
            int rank_dist = std::abs(get_rank(int(s1)) - get_rank(int(s2)));
            SquareDistance[s1][s2] = std::max(file_dist, rank_dist);
        }
    
    // Initialize magic bitboards
    init_magics(ROOK, rook_table, Magics);
    init_magics(BISHOP, bishop_table, Magics);
    
    init_knight_attacks();
    init_king_attacks();
    init_pawn_attacks();
    
    is_initialized = true;
}

void BitboardUtils::init_magics(PieceType pt, Bitboard table[], Magic magics[][2]) {
    // Optimal PRNG seeds to pick the correct magics in the shortest time
    int seeds[][8] = {{8977, 44560, 54343, 38998, 5731, 95205, 104912, 17020},
                      {728, 10316, 55013, 32803, 12281, 15100, 16645, 255}};

    Bitboard occupancy[4096];
    Bitboard reference[4096];
    int epoch[4096] = {};
    int cnt = 0;
    int size = 0;

    for (Square s = A1; s <= H8; ++s) {
        // Board edges are not considered in the relevant occupancies
        Bitboard rank1 = 0xFFULL;
        Bitboard rank8 = 0xFF00000000000000ULL;
        Bitboard fileA = 0x0101010101010101ULL;
        Bitboard fileH = 0x8080808080808080ULL;
        
        Bitboard rank_bb_s = rank1 << (8 * get_rank(int(s)));
        Bitboard file_bb_s = fileA << get_file(int(s));
        
        Bitboard edges = ((rank1 | rank8) & ~rank_bb_s) | ((fileA | fileH) & ~file_bb_s);

        // Given a square 's', the mask is the bitboard of sliding attacks from
        // 's' computed on an empty board. The index must be big enough to contain
        // all the attacks for each possible subset of the mask.
        Magic& m = magics[s][pt];
        m.mask = sliding_attack(pt, s, 0) & ~edges;
        
#ifndef USE_PEXT
        m.shift = 64 - popcount(m.mask);
#endif

        // Set the offset for the attacks table of the square
        m.attacks = (s == A1) ? table : magics[int(s) - 1][pt].attacks + size;
        size = 0;

        // Use Carry-Rippler trick to enumerate all subsets of mask
        Bitboard b = 0;
        do {
            occupancy[size] = b;
            reference[size] = sliding_attack(pt, s, b);
            
#ifdef USE_PEXT
            m.attacks[pext(b, m.mask)] = reference[size];
#endif
            size++;
            b = (b - m.mask) & m.mask;
        } while (b);

#ifndef USE_PEXT
        // Find a magic for square 's' picking up an (almost) random number
        // until we find the one that passes the verification test.
        PRNG rng(seeds[pt][get_rank(int(s))]);

        for (int i = 0; i < size;) {
            // Generate magic candidate
            for (m.magic = 0; popcount((m.magic * m.mask) >> 56) < 6;)
                m.magic = rng.sparse_rand<Bitboard>();

            for (++cnt, i = 0; i < size; ++i) {
                unsigned idx = m.index(occupancy[i]);

                if (epoch[idx] < cnt) {
                    epoch[idx] = cnt;
                    m.attacks[idx] = reference[i];
                } else if (m.attacks[idx] != reference[i]) {
                    break;
                }
            }
        }
#endif
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