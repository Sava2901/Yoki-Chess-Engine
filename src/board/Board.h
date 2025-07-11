#ifndef BITBOARD_BOARD_H
#define BITBOARD_BOARD_H

#include "Bitboard.h"
#include "Move.h"
#include <string>
#include <array>

// Forward declaration
struct BitboardMoveUndoData;

/**
 * BitboardBoard - A chess board representation using bitboards
 * This class provides a high-performance alternative to the traditional 8x8 array board
 */
class Board {
public:
    // Piece types for bitboard indexing
    enum PieceType {
        PAWN = 0,
        KNIGHT = 1,
        BISHOP = 2,
        ROOK = 3,
        QUEEN = 4,
        KING = 5,
        NUM_PIECE_TYPES = 6
    };
    
    // Colors
    enum Color {
        WHITE = 0,
        BLACK = 1,
        NUM_COLORS = 2
    };
    
private:
    // Bitboards for each piece type and color
    std::array<std::array<Bitboard, NUM_PIECE_TYPES>, NUM_COLORS> piece_bitboards{};
    
    // Combined bitboards for all pieces of each color
    std::array<Bitboard, NUM_COLORS> color_bitboards{};
    
    // Combined bitboard for all pieces
    Bitboard all_pieces;
    
    // Game state
    Color active_color;
    uint8_t castling_rights; // KQkq format
    int8_t en_passant_file;  // -1 if no en passant
    int halfmove_clock;
    int fullmove_number;
    
    // King positions for quick access
    std::array<int, NUM_COLORS> king_positions{};
    
public:
    Board();
    
    // Board setup
    void set_starting_position();
    void set_from_fen(const std::string& fen);
    [[nodiscard]] std::string to_fen() const;
    
    // Piece manipulation
    void set_piece(int rank, int file, char piece);
    [[nodiscard]] char get_piece(int rank, int file) const;
    void clear_square(int square);
    void place_piece(int square, PieceType piece_type, Color color);
    
    // Bitboard access
    [[nodiscard]] Bitboard get_piece_bitboard(PieceType piece_type, Color color) const;
    [[nodiscard]] Bitboard get_color_bitboard(Color color) const;
    [[nodiscard]] Bitboard get_all_pieces() const;
    
    // Game state access
    [[nodiscard]] Color get_active_color() const { return active_color; }
    void set_active_color(Color color) { active_color = color; }
    [[nodiscard]] char get_active_color_char() const { return active_color == WHITE ? 'w' : 'b'; }
    
    [[nodiscard]] uint8_t get_castling_rights() const { return castling_rights; }
    void set_castling_rights(uint8_t rights) { castling_rights = rights; }
    
    [[nodiscard]] int8_t get_en_passant_file() const { return en_passant_file; }
    void set_en_passant_file(int8_t file) { en_passant_file = file; }
    
    [[nodiscard]] int get_halfmove_clock() const { return halfmove_clock; }
    void set_halfmove_clock(int clock) { halfmove_clock = clock; }
    
    [[nodiscard]] int get_fullmove_number() const { return fullmove_number; }
    void set_fullmove_number(int number) { fullmove_number = number; }
    
    [[nodiscard]] int get_king_position(Color color) const { return king_positions[color]; }
    
    // Move operations
    bool make_move(const Move& move);
    void undo_move(const Move& move, const BitboardMoveUndoData& undo_data);
    
    // Attack and check detection
    [[nodiscard]] bool is_square_attacked(int square, Color attacking_color) const;
    [[nodiscard]] bool is_in_check(Color color) const;
    
    // Utility functions
    void print() const;
    [[nodiscard]] std::string to_string() const;
    
    // Convert between coordinate systems
    static PieceType char_to_piece_type(char piece);
    static Color char_to_color(char piece);
    static char piece_to_char(PieceType piece_type, Color color);
    
private:
    // Internal helper functions
    void update_combined_bitboards();
    void update_king_position(Color color);
    [[nodiscard]] Bitboard get_attackers_to_square(int square, Color attacking_color) const;
};

/**
 * Undo data structure for bitboard moves
 */
struct BitboardMoveUndoData {
    Move move;
    char captured_piece;
    uint8_t castling_rights;
    int8_t en_passant_file;
    int halfmove_clock;
    
    BitboardMoveUndoData() : captured_piece('.'), castling_rights(0), 
                            en_passant_file(-1), halfmove_clock(0) {}
};

#endif // BITBOARD_BOARD_H