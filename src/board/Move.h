#ifndef MOVE_H
#define MOVE_H

#include <string>
#include <vector>

// Move structure to represent a chess move
// TODO: Look into using a more efficient representation for moves
struct Move {
    int from_rank;
    int from_file;
    int to_rank;
    int to_file;
    char piece;           // The piece being moved
    char captured_piece;  // The piece being captured (if any), '.' if none
    char promotion_piece; // The piece to promote to (if any), '.' if none
    bool is_castling;     // True if this is a castling move
    bool is_en_passant;   // True if this is an en passant capture
    
    // Default constructor
    Move() : from_rank(0), from_file(0), to_rank(0), to_file(0), 
             piece('.'), captured_piece('.'), promotion_piece('.'),
             is_castling(false), is_en_passant(false) {}
    
    // Constructor with basic move information
    Move(int fr, int ff, int tr, int tf, char p) 
        : from_rank(fr), from_file(ff), to_rank(tr), to_file(tf), piece(p),
          captured_piece('.'), promotion_piece('.'), is_castling(false), is_en_passant(false) {}
    
    // Constructor with all information
    Move(int fr, int ff, int tr, int tf, char p, char cap, char prom = '.', bool castle = false, bool ep = false)
        : from_rank(fr), from_file(ff), to_rank(tr), to_file(tf), piece(p),
          captured_piece(cap), promotion_piece(prom), is_castling(castle), is_en_passant(ep) {}
    
    // Convert move to algebraic notation (e.g., "e2e4")
    std::string to_algebraic() const {
        std::string result = "";
        result += static_cast<char>('a' + from_file);
        result += static_cast<char>('1' + from_rank);
        result += static_cast<char>('a' + to_file);
        result += static_cast<char>('1' + to_rank);
        
        // Add promotion piece if applicable
        if (promotion_piece != '.') {
            result += std::tolower(promotion_piece);
        }
        
        return result;
    }
    
    // Check if this is a valid move (basic validation)
    bool is_valid() const {
        return from_rank >= 0 && from_rank < 8 && from_file >= 0 && from_file < 8 &&
               to_rank >= 0 && to_rank < 8 && to_file >= 0 && to_file < 8 &&
               piece != '.' && !(from_rank == to_rank && from_file == to_file);
    }
    
    // Equality operator
    bool operator==(const Move& other) const {
        return from_rank == other.from_rank && from_file == other.from_file &&
               to_rank == other.to_rank && to_file == other.to_file &&
               piece == other.piece && captured_piece == other.captured_piece &&
               promotion_piece == other.promotion_piece && is_castling == other.is_castling &&
               is_en_passant == other.is_en_passant;
    }
};

// Type alias for a list of moves
using MoveList = std::vector<Move>;

#endif // MOVE_H