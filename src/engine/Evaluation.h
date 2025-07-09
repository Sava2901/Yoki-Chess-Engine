#ifndef EVALUATION_H
#define EVALUATION_H

#include "../board/Board.h"
#include <array>

class Evaluation {
public:
    // Main evaluation function
    static int evaluate_position(const Board& board);
    
    // Material evaluation
    static int evaluate_material(const Board& board);
    
    // Positional evaluation using piece-square tables
    static int evaluate_position_tables(const Board& board);
    
    // Piece mobility evaluation
    static int evaluate_mobility(const Board& board);
    
    // King safety evaluation
    static int evaluate_king_safety(const Board& board);
    
    // Pawn structure evaluation
    static int evaluate_pawn_structure(const Board& board);
    
    // Get piece value
    static int get_piece_value(char piece);
    
    // Check if position is endgame
    static bool is_endgame(const Board& board);
    
private:
    // Piece values in centipawns
    static const int PAWN_VALUE = 100;
    static const int KNIGHT_VALUE = 320;
    static const int BISHOP_VALUE = 330;
    static const int ROOK_VALUE = 500;
    static const int QUEEN_VALUE = 900;
    static const int KING_VALUE = 20000;
    
    // Piece-square tables for positional evaluation
    // Tables are from white's perspective (rank 0 = 8th rank, rank 7 = 1st rank)
    
    // Pawn piece-square table
    static const std::array<std::array<int, 8>, 8> PAWN_TABLE;
    
    // Knight piece-square table
    static const std::array<std::array<int, 8>, 8> KNIGHT_TABLE;
    
    // Bishop piece-square table
    static const std::array<std::array<int, 8>, 8> BISHOP_TABLE;
    
    // Rook piece-square table
    static const std::array<std::array<int, 8>, 8> ROOK_TABLE;
    
    // Queen piece-square table
    static const std::array<std::array<int, 8>, 8> QUEEN_TABLE;
    
    // King piece-square table (middlegame)
    static const std::array<std::array<int, 8>, 8> KING_MG_TABLE;
    
    // King piece-square table (endgame)
    static const std::array<std::array<int, 8>, 8> KING_EG_TABLE;
    
    // Helper functions
    static int get_piece_square_value(char piece, int rank, int file, bool is_endgame);
    static int flip_rank(int rank); // Flip rank for black pieces
    static bool is_white_piece(char piece);
    static bool is_black_piece(char piece);
    static char to_lower(char piece);
};

#endif // EVALUATION_H