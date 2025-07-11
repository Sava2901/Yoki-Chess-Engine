#ifndef BOARD_H
#define BOARD_H

#include <string>
#include <cstdint>
#include "Move.h"

// Optimized castling rights using bitflags
enum CastlingRights : uint8_t {
    NO_CASTLING = 0,
    WHITE_KINGSIDE = 1,
    WHITE_QUEENSIDE = 2,
    BLACK_KINGSIDE = 4,
    BLACK_QUEENSIDE = 8,
    WHITE_CASTLING = WHITE_KINGSIDE | WHITE_QUEENSIDE,
    BLACK_CASTLING = BLACK_KINGSIDE | BLACK_QUEENSIDE,
    ALL_CASTLING = WHITE_CASTLING | BLACK_CASTLING
};

// Square indices for faster access
constexpr int SQUARE_INDEX(int rank, int file) { return rank * 8 + file; }
constexpr int RANK_FROM_INDEX(int index) { return index / 8; }
constexpr int FILE_FROM_INDEX(int index) { return index % 8; }

class Board {
public:
    Board();
    ~Board();
    
    // Board setup and manipulation
    void clear();
    void set_from_fen(const std::string& piece_placement);
    void set_position(const std::string& fen);
    void print() const;
    
    // Piece access
    char get_piece(int rank, int file) const;
    void set_piece(int rank, int file, char piece);
    
    // Board state queries
    bool is_empty(int rank, int file) const;
    bool is_valid_square(int rank, int file) const;
    
    // FEN generation
    std::string to_fen_piece_placement() const;
    std::string to_fen() const;
    
    // Game state access (optimized)
    char get_active_color() const { return active_color; }
    uint8_t get_castling_rights_bits() const { return castling_rights_bits; }
    bool can_castle_kingside(char color) const {
        return color == 'w' ? (castling_rights_bits & WHITE_KINGSIDE) : (castling_rights_bits & BLACK_KINGSIDE);
    }
    bool can_castle_queenside(char color) const {
        return color == 'w' ? (castling_rights_bits & WHITE_QUEENSIDE) : (castling_rights_bits & BLACK_QUEENSIDE);
    }
    int8_t get_en_passant_file() const { return en_passant_file; }
    uint16_t get_halfmove_clock() const { return halfmove_clock; }
    uint16_t get_fullmove_number() const { return fullmove_number; }
    
    // Legacy string methods for compatibility
    std::string get_castling_rights() const;
    std::string get_en_passant_target() const;
    
    // Game state modification (optimized)
    void set_active_color(char color) { active_color = color; }
    void set_castling_rights_bits(uint8_t rights) { castling_rights_bits = rights; }
    void set_en_passant_file(int8_t file) { en_passant_file = file; }
    void set_halfmove_clock(uint16_t clock) { halfmove_clock = clock; }
    void set_fullmove_number(uint16_t number) { fullmove_number = number; }
    
    // Legacy string methods for compatibility
    void set_castling_rights(const std::string& rights);
    void set_en_passant_target(const std::string& target);
    
    // Board state for undo functionality
    struct BoardState {
        char active_color;
        uint8_t castling_rights_bits;
        int8_t en_passant_file;
        uint16_t halfmove_clock;
        uint16_t fullmove_number;
        char captured_piece;
        
        BoardState() : active_color('w'), castling_rights_bits(ALL_CASTLING), en_passant_file(-1),
                      halfmove_clock(0), fullmove_number(1), captured_piece('.') {}
    };
    
    // Move validation and execution
    bool is_valid_move(const Move& move) const;
    bool make_move(const Move& move);
    bool make_move(const Move& move, BoardState& previous_state);
    void undo_move(const Move& move, const BoardState& previous_state);
    
    // Move creation from algebraic notation
    Move create_move_from_algebraic(const std::string& algebraic) const;
    
    // Get current board state for undo
    BoardState get_board_state() const;
    
private:
    char board[8][8];  // 8x8 chess board representation
    
    // Game state variables (optimized)
    char active_color;
    uint8_t castling_rights_bits;  // Using bitflags instead of string
    int8_t en_passant_file;        // -1 if no en passant, 0-7 for file
    uint16_t halfmove_clock;
    uint16_t fullmove_number;
    
    // Helper methods (optimized with inline)
    void initialize_empty_board();
    void initialize_starting_position();
    
    // Optimized coordinate conversion (constexpr for compile-time evaluation)
    static constexpr char file_to_char(int file) { return 'a' + file; }
    static constexpr int char_to_file(char file) { return file - 'a'; }
    static constexpr char rank_to_char(int rank) { return '8' - rank; }
    static constexpr int char_to_rank(char rank) { return '8' - rank; }
    
    // Fast piece access using array indexing
    char get_piece_fast(int square_index) const { return board[square_index >> 3][square_index & 7]; }
    void set_piece_fast(int square_index, char piece) { board[square_index >> 3][square_index & 7] = piece; }
    
    // Optimized castling rights manipulation
    void remove_castling_rights(uint8_t rights_to_remove) { castling_rights_bits &= ~rights_to_remove; }
    void add_castling_rights(uint8_t rights_to_add) { castling_rights_bits |= rights_to_add; }
};

#endif // BOARD_H