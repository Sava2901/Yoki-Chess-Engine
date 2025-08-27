#ifndef MOVE_H
#define MOVE_H

#include <string>
#include <vector>
#include <iostream>

/**
 * @brief Structure representing a chess move
 * 
 * Contains all information needed to represent and execute a chess move,
 * including source and destination squares, piece information, special move flags,
 * and captured pieces.
 * 
 * @note TODO: Look into using a more efficient representation for moves
 */
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
    
    /**
     * @brief Default constructor
     * 
     * Creates an empty move with all fields initialized to default values.
     */
    Move() : from_rank(0), from_file(0), to_rank(0), to_file(0), 
             piece('.'), captured_piece('.'), promotion_piece('.'),
             is_castling(false), is_en_passant(false) {}
    
    /**
     * @brief Constructor with basic move information
     * 
     * Creates a move with source and destination coordinates and the moving piece.
     * Other fields are set to default values.
     * 
     * @param fr Source rank (0-7)
     * @param ff Source file (0-7)
     * @param tr Destination rank (0-7)
     * @param tf Destination file (0-7)
     * @param p Character representing the moving piece
     */
    Move(int fr, int ff, int tr, int tf, char p) 
        : from_rank(fr), from_file(ff), to_rank(tr), to_file(tf), piece(p),
          captured_piece('.'), promotion_piece('.'), is_castling(false), is_en_passant(false) {}
    
    /**
     * @brief Constructor with complete move information
     * 
     * Creates a move with all possible information including captures,
     * promotions, and special move flags.
     * 
     * @param fr Source rank (0-7)
     * @param ff Source file (0-7)
     * @param tr Destination rank (0-7)
     * @param tf Destination file (0-7)
     * @param p Character representing the moving piece
     * @param cap Character representing captured piece ('.' if none)
     * @param prom Character representing promotion piece ('.' if none)
     * @param castle True if this is a castling move
     * @param ep True if this is an en passant capture
     */
    Move(int fr, int ff, int tr, int tf, char p, char cap, char prom = '.', bool castle = false, bool ep = false)
        : from_rank(fr), from_file(ff), to_rank(tr), to_file(tf), piece(p),
          captured_piece(cap), promotion_piece(prom), is_castling(castle), is_en_passant(ep) {}
    
    /**
     * @brief Convert move to algebraic notation
     * 
     * Converts the move to standard algebraic notation (e.g., "e2e4").
     * Includes promotion piece suffix if applicable.
     * 
     * @return String representation in algebraic notation
     */
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
    
    /**
     * @brief Check if this is a valid move (basic validation)
     * 
     * Performs basic validation to ensure the move has valid coordinates,
     * a valid piece, and the source and destination are different.
     * 
     * @return true if the move passes basic validation, false otherwise
     */
    bool is_valid() const {
        return from_rank >= 0 && from_rank < 8 && from_file >= 0 && from_file < 8 &&
               to_rank >= 0 && to_rank < 8 && to_file >= 0 && to_file < 8 &&
               piece != '.' && !(from_rank == to_rank && from_file == to_file);
    }

    /**
     * @brief Check if this move captures a piece
     * 
     * @return true if this move captures an opponent's piece, false otherwise
     */
    bool is_capture() const {
        return captured_piece != '.';
    }

    /**
     * @brief Check if this move is a pawn promotion
     * 
     * @return true if this move promotes a pawn, false otherwise
     */
    bool is_promotion() const {
        return promotion_piece != '.';
    }

    /**
     * @brief Print the move to standard output
     * 
     * Prints the move in algebraic notation to the console.
     */
    void print() const {
        std::cout << this->to_algebraic() << std::endl;
    }

    /**
     * @brief Equality comparison operator
     * 
     * Compares all fields of two moves to determine if they are identical.
     * 
     * @param other The move to compare with
     * @return true if all fields match, false otherwise
     */
    bool operator==(const Move& other) const {
        return from_rank == other.from_rank && from_file == other.from_file &&
               to_rank == other.to_rank && to_file == other.to_file &&
               piece == other.piece && captured_piece == other.captured_piece &&
               promotion_piece == other.promotion_piece && is_castling == other.is_castling &&
               is_en_passant == other.is_en_passant;
    }
};

/**
 * @brief Type alias for a list of moves
 * 
 * Convenient alias for std::vector<Move> used throughout the codebase
 * to represent collections of chess moves.
 */
using MoveList = std::vector<Move>;

#endif // MOVE_H