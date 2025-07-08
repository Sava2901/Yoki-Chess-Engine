#ifndef MOVEGENERATOR_H
#define MOVEGENERATOR_H

#include "Move.h"
#include "Board.h"
#include <vector>

class MoveGenerator {
public:
    // Generate all pseudo-legal moves (ignoring check)
    static MoveList generate_pseudo_legal_moves(const Board& board);
    
    // Generate all legal moves (filtering out moves that leave king in check)
    static MoveList generate_legal_moves(const Board& board);
    
    // Check if a move is legal (doesn't leave own king in check)
    static bool is_legal_move(const Board& board, const Move& move);
    
    // Check if the current player's king is in check
    static bool is_in_check(const Board& board, char color);
    
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
    
    // Generate en passant moves
    static void generate_en_passant_moves(const Board& board, MoveList& moves);
    
    // Helper functions for sliding pieces (rook, bishop, queen)
    static void generate_sliding_moves(const Board& board, int rank, int file, 
                                     const std::vector<std::pair<int, int>>& directions, 
                                     MoveList& moves);
    
    // Helper function to check if a piece belongs to the current player
    static bool is_own_piece(char piece, char active_color);
    
    // Helper function to check if a piece belongs to the opponent
    static bool is_opponent_piece(char piece, char active_color);
    
    // Helper function to get piece color
    static char get_piece_color(char piece);
    
    // Helper function to find the king position
    static std::pair<int, int> find_king_position(const Board& board, char color);
    
    // Helper function to make a move on a temporary board for legality checking
    static Board make_move_on_copy(const Board& board, const Move& move);
};

#endif // MOVEGENERATOR_H