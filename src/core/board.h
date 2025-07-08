#ifndef YOKI_BOARD_H
#define YOKI_BOARD_H

#include <string>
#include <vector>
#include <cstdint>

namespace yoki {

// Piece types
enum PieceType {
    EMPTY = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6
};

// Colors
enum Color {
    WHITE = 0,
    BLACK = 1
};

// Square representation (0-63)
using Square = int;

// Move representation
struct Move {
    Square from;
    Square to;
    PieceType promotion = EMPTY;
    bool is_castling = false;
    bool is_en_passant = false;
    
    Move() = default;
    Move(Square f, Square t) : from(f), to(t) {}
    Move(Square f, Square t, PieceType promo) : from(f), to(t), promotion(promo) {}
    
    bool operator==(const Move& other) const {
        return from == other.from && to == other.to && promotion == other.promotion;
    }
    
    std::string to_string() const;
};

// Castling rights
struct CastlingRights {
    bool white_kingside = false;
    bool white_queenside = false;
    bool black_kingside = false;
    bool black_queenside = false;
};

// Board state
class Board {
public:
    Board();
    explicit Board(const std::string& fen);
    
    // Board setup
    void reset();
    bool load_fen(const std::string& fen);
    std::string to_fen() const;
    
    // Piece access
    PieceType get_piece_type(Square sq) const;
    Color get_piece_color(Square sq) const;
    bool is_empty(Square sq) const;
    
    // Move operations
    bool make_move(const Move& move);
    void unmake_move();
    bool is_legal_move(const Move& move) const;
    
    // Game state
    Color get_side_to_move() const { return side_to_move_; }
    bool is_in_check(Color color) const;
    bool is_checkmate() const;
    bool is_stalemate() const;
    bool is_draw() const;
    
    // Zobrist hashing
    uint64_t get_hash() const { return hash_; }
    
    // Utility
    void print() const;
    
private:
    // 8x8 mailbox representation
    PieceType pieces_[64];
    Color colors_[64];
    
    Color side_to_move_;
    CastlingRights castling_rights_;
    Square en_passant_square_;
    int halfmove_clock_;
    int fullmove_number_;
    
    uint64_t hash_;
    
    // Move history for unmake
    struct BoardState {
        PieceType captured_piece;
        Color captured_color;
        CastlingRights castling_rights;
        Square en_passant_square;
        int halfmove_clock;
        uint64_t hash;
    };
    std::vector<BoardState> history_;
    
    // Helper methods
    void update_hash();
    void clear_square(Square sq);
    void set_piece(Square sq, PieceType piece, Color color);
    bool is_valid_square(Square sq) const;
    Square get_king_square(Color color) const;
    bool is_attacked_by(Square sq, Color attacker) const;
};

// Utility functions
Square make_square(int file, int rank);
int get_file(Square sq);
int get_rank(Square sq);
std::string square_to_string(Square sq);
Square string_to_square(const std::string& str);

} // namespace yoki

#endif // YOKI_BOARD_H