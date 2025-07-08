#ifndef YOKI_MOVEGEN_H
#define YOKI_MOVEGEN_H

#include "board.h"
#include <vector>

namespace yoki {

class MoveGenerator {
public:
    explicit MoveGenerator(const Board& board);
    
    // Generate all legal moves for the current position
    std::vector<Move> generate_legal_moves() const;
    
    // Generate all pseudo-legal moves (may leave king in check)
    std::vector<Move> generate_pseudo_legal_moves() const;
    
    // Check if a specific move is legal
    bool is_move_legal(const Move& move) const;
    
    // Generate moves for a specific piece type
    std::vector<Move> generate_pawn_moves(Square from) const;
    std::vector<Move> generate_knight_moves(Square from) const;
    std::vector<Move> generate_bishop_moves(Square from) const;
    std::vector<Move> generate_rook_moves(Square from) const;
    std::vector<Move> generate_queen_moves(Square from) const;
    std::vector<Move> generate_king_moves(Square from) const;
    
    // Generate castling moves
    std::vector<Move> generate_castling_moves() const;
    
    // Check if a square is attacked by the given color
    bool is_square_attacked(Square sq, Color attacker) const;
    
private:
    const Board& board_;
    
    // Helper methods
    void add_move_if_valid(std::vector<Move>& moves, Square from, Square to) const;
    void add_promotion_moves(std::vector<Move>& moves, Square from, Square to) const;
    bool is_path_clear(Square from, Square to) const;
    bool is_valid_destination(Square to, Color moving_color) const;
    
    // Direction vectors for sliding pieces
    static const int knight_directions[8][2];
    static const int king_directions[8][2];
    static const int bishop_directions[4][2];
    static const int rook_directions[4][2];
};

// Utility functions for move validation
bool is_valid_move_format(const std::string& move_str);
Move parse_move_string(const std::string& move_str);
std::string move_to_algebraic(const Move& move, const Board& board);

} // namespace yoki

#endif // YOKI_MOVEGEN_H