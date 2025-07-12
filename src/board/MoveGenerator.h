#ifndef BITBOARD_MOVE_GENERATOR_H
#define BITBOARD_MOVE_GENERATOR_H

#include "Board.h"
#include "Move.h"
#include "Bitboard.h"
#include <vector>

// Magic bitboard structure for sliding piece attack generation
struct Magic {
    Bitboard mask;
    Bitboard magic;
    Bitboard* attacks;
    int shift;
};

// Magic bitboard arrays for rook and bishop attack generation
extern Magic bishop_magics[64];
extern Magic rook_magics[64];

/**
 * BitboardMoveGenerator - High-performance move generation using bitboards
 * This class provides O(1) sliding piece move generation using magic bitboards
 */
class MoveGenerator {
public:
    MoveGenerator();

    // Main move generation functions
    std::vector<Move> generate_all_moves(const Board& board);
    std::vector<Move> generate_legal_moves(Board& board);
    std::vector<Move> generate_captures(const Board& board);
    std::vector<Move> generate_quiet_moves(const Board& board);

    // Specific piece move generation
    void generate_pawn_moves(const Board& board, std::vector<Move>& moves, bool captures_only = false);
    void generate_knight_moves(const Board& board, std::vector<Move>& moves, bool captures_only = false);
    void generate_bishop_moves(const Board& board, std::vector<Move>& moves, bool captures_only = false);
    void generate_rook_moves(const Board& board, std::vector<Move>& moves, bool captures_only = false);
    void generate_queen_moves(const Board& board, std::vector<Move>& moves, bool captures_only = false);
    void generate_king_moves(const Board& board, std::vector<Move>& moves, bool captures_only = false);

    // Special moves
    void generate_castling_moves(const Board& board, std::vector<Move>& moves);
    void generate_en_passant_moves(const Board& board, std::vector<Move>& moves);

    // Check and legality testing
    bool is_in_check(const Board& board, Board::Color color);
    bool is_legal_move(Board& board, const Move& move);
    bool is_square_attacked(const Board& board, int square, Board::Color attacking_color);

    // Attack generation
    Bitboard get_attacked_squares(const Board& board, Board::Color color);
    Bitboard get_piece_attacks(const Board& board, int square, Board::PieceType piece_type, Board::Color color);
    
    // Magic bitboard attack generation
    static Bitboard get_bishop_attacks(int square, Bitboard occupancy);
    static Bitboard get_rook_attacks(int square, Bitboard occupancy);
    static Bitboard get_queen_attacks(int square, Bitboard occupancy);

    // Utility functions
    int count_moves(const Board& board);
    bool has_legal_moves(Board& board);

private:
    // Helper functions for move generation
    void add_moves_from_bitboard(Bitboard from_square, Bitboard to_squares,
                                Board::PieceType piece_type, Board::Color color,
                                const Board& board, std::vector<Move>& moves, bool captures_only = false);

    void add_pawn_moves(int from_square, Bitboard to_squares, Board::Color color,
                       const Board& board, std::vector<Move>& moves, bool is_capture = false);

    void add_promotion_moves(int from_square, int to_square, Board::Color color,
                            const Board& board, std::vector<Move>& moves, bool is_capture = false);

    // Castling helpers
    bool can_castle_kingside(const Board& board, Board::Color color);
    bool can_castle_queenside(const Board& board, Board::Color color);

    // Pin and discovery helpers
    Bitboard get_pinned_pieces(const Board& board, Board::Color color);
    Bitboard get_check_mask(const Board& board, Board::Color color);
    Bitboard get_between_squares(int sq1, int sq2);
    Bitboard get_attackers_to_square(const Board& board, int square, Board::Color attacking_color);

    // Move filtering and legality
    bool is_move_legal_in_check(const Board& board, const Move& move, Bitboard check_mask, Bitboard pinned_pieces);
    bool are_squares_aligned(int sq1, int sq2, int sq3);

    // Move ordering
    void order_moves(std::vector<Move>& moves, const Board& board);
    void order_captures(std::vector<Move>& moves, const Board& board);
    int get_move_score(const Move& move, const Board& board);
    int get_capture_score(const Move& move, const Board& board);
    int char_to_piece_type(char piece);

    // Utility functions
    char piece_type_to_char(Board::PieceType piece_type, Board::Color color);
    bool is_promotion_rank(int rank, Board::Color color);
    
    // Performance counters
    mutable uint64_t nodes_searched;
    mutable uint64_t moves_generated;
};

#endif // BITBOARD_MOVE_GENERATOR_H