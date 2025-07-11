#ifndef MOVEGENERATOR_H
#define MOVEGENERATOR_H

#include "Move.h"
#include "Board.h"
#include <vector>
#include <array>

// Precomputed move directions for better performance
static constexpr std::array<std::pair<int, int>, 8> KNIGHT_MOVES = {{
    {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
    {1, -2}, {1, 2}, {2, -1}, {2, 1}
}};

static constexpr std::array<std::pair<int, int>, 8> KING_MOVES = {{
    {-1, -1}, {-1, 0}, {-1, 1},
    {0, -1},           {0, 1},
    {1, -1},  {1, 0},  {1, 1}
}};

static constexpr std::array<std::pair<int, int>, 4> ROOK_DIRECTIONS = {{
    {-1, 0}, {1, 0}, {0, -1}, {0, 1}
}};

static constexpr std::array<std::pair<int, int>, 4> BISHOP_DIRECTIONS = {{
    {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
}};

static constexpr std::array<std::pair<int, int>, 8> QUEEN_DIRECTIONS = {{
    {-1, -1}, {-1, 0}, {-1, 1},
    {0, -1},           {0, 1},
    {1, -1},  {1, 0},  {1, 1}
}};

class MoveGenerator {
public:
    // Generate all pseudo-legal moves (ignoring check)
    static MoveList generate_pseudo_legal_moves(const Board& board);
    
    // Generate all legal moves (filtering out moves that leave king in check)
    static MoveList generate_legal_moves(Board& board);
    
    // Check if a move is legal (doesn't leave own king in check)
    static bool is_legal_move(Board& board, const Move& move);
    
    // Check if the current player's king is in check
    static bool is_in_check(const Board& board, char color);
    
    // Check if the king at the given position is in check
    static bool is_in_check(const Board& board, char color, const std::pair<int, int>& king_pos);
    
    // Check if a square is under attack by the opponent
    static bool is_square_attacked(const Board& board, int rank, int file, char attacking_color);
    
private:
    // Generate moves for specific piece types
    static void generate_pawn_moves(const Board& board, int rank, int file, MoveList& moves);
    static void generate_rook_moves(const Board& board, int rank, int file, MoveList& moves);
    static void generate_knight_moves(const Board& board, int rank, int file, MoveList& moves);
    static void generate_bishop_moves(const Board& board, int rank, int file, MoveList& moves);
    static void generate_queen_moves(const Board& board, int rank, int file, MoveList& moves);
    static void generate_king_moves(const Board& board, int rank, int file, MoveList& moves);
    
    // Generate castling moves
    static void generate_castling_moves(const Board& board, MoveList& moves);
    

    
    // Optimized helper functions for sliding pieces
    template<size_t N>
    static void generate_sliding_moves(const Board& board, int rank, int file, 
                                     const std::array<std::pair<int, int>, N>& directions, 
                                     MoveList& moves);
    
    // Optimized helper functions (inline for better performance)
    static bool is_own_piece(char piece, char active_color) {
        return piece != '.' && get_piece_color(piece) == active_color;
    }
    
    static bool is_opponent_piece(char piece, char active_color) {
        return piece != '.' && get_piece_color(piece) != active_color;
    }
    
    static char get_piece_color(char piece) {
        return std::isupper(piece) ? 'w' : 'b';
    }
    
    // Fast square validation
    static bool is_valid_square(int rank, int file) {
        return (rank | file) >= 0 && (rank | file) < 8;
    }
    
    // Helper functions
    static void perform_castling_rook_move(Board& board, const Move& move, 
                                         int& rook_from_file, int& rook_to_file,
                                         char& original_rook_from, char& original_rook_to);
    static std::pair<int, int> find_king_position(const Board& board, char color);
};

#endif // MOVEGENERATOR_H