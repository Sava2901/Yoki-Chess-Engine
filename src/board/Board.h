#ifndef BOARD_H
#define BOARD_H

#include <string>
#include <vector>
#include "../board/FENUtils.h"

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
    
    // Game state access
    char get_active_color() const { return active_color; }
    const std::string& get_castling_rights() const { return castling_rights; }
    const std::string& get_en_passant_target() const { return en_passant_target; }
    int get_halfmove_clock() const { return halfmove_clock; }
    int get_fullmove_number() const { return fullmove_number; }
    
    // Game state modification
    void set_active_color(char color) { active_color = color; }
    void set_castling_rights(const std::string& rights) { castling_rights = rights; }
    void set_en_passant_target(const std::string& target) { en_passant_target = target; }
    void set_halfmove_clock(int clock) { halfmove_clock = clock; }
    void set_fullmove_number(int number) { fullmove_number = number; }
    
private:
    char board[8][8];  // 8x8 chess board representation
    
    // Game state variables (moved from Engine)
    char active_color;
    std::string castling_rights;
    std::string en_passant_target;
    int halfmove_clock;
    int fullmove_number;
    
    // Helper methods
    void initialize_empty_board();
    void initialize_starting_position();
    char file_to_char(int file) const;  // 0-7 -> a-h
    int char_to_file(char file) const;  // a-h -> 0-7
    char rank_to_char(int rank) const;  // 0-7 -> 8-1
    int char_to_rank(char rank) const;  // 8-1 -> 0-7
};

#endif // BOARD_H