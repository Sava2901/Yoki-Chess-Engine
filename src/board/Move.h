#ifndef MOVE_H
#define MOVE_H

#include <string>
#include <vector>
#include <iostream>
#include <cstdint>
#include "SmallVector.h"

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
    int from_rank;        ///< Source rank (0-7)
    int from_file;        ///< Source file (0-7)
    int to_rank;          ///< Destination rank (0-7)
    int to_file;          ///< Destination file (0-7)
    char piece;           ///< The piece being moved
    char captured_piece;  ///< The piece being captured (if any), '.' if none
    char promotion_piece; ///< The piece to promote to (if any), '.' if none
    bool is_castling;     ///< True if this is a castling move
    bool is_en_passant;   ///< True if this is an en passant capture
    
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
    
    /**
     * @brief Inequality comparison operator
     * 
     * @param other The move to compare with
     * @return true if moves are different, false if identical
     */
    bool operator!=(const Move& other) const {
        return !(*this == other);
    }
    
    /**
     * @brief Convert move to uint32_t for transposition table storage
     * 
     * Packs the move information into a 32-bit integer for efficient storage.
     * Format: from_square(6) | to_square(6) | piece(4) | captured_piece(4) | 
     *         promotion_piece(4) | flags(8)
     * 
     * @return 32-bit representation of the move
     */
    uint32_t to_uint32() const {
        uint32_t result = 0;
        
        // Pack coordinates (6 bits each)
        result |= (from_rank * 8 + from_file) & 0x3F;           // bits 0-5
        result |= ((to_rank * 8 + to_file) & 0x3F) << 6;        // bits 6-11
        
        // Pack piece types (4 bits each)
        result |= (piece_to_int(piece) & 0xF) << 12;             // bits 12-15
        result |= (piece_to_int(captured_piece) & 0xF) << 16;    // bits 16-19
        result |= (piece_to_int(promotion_piece) & 0xF) << 20;   // bits 20-23
        
        // Pack flags (8 bits)
        uint32_t flags = 0;
        if (is_castling) flags |= 1;
        if (is_en_passant) flags |= 2;
        result |= (flags & 0xFF) << 24;                          // bits 24-31
        
        return result;
    }
    
    /**
     * @brief Reconstruct move from uint32_t representation
     * 
     * Unpacks a move from its 32-bit representation stored in the transposition table.
     * 
     * @param packed The 32-bit packed move representation
     */
    void from_uint32(uint32_t packed) {
        // Unpack coordinates
        uint32_t from_square = packed & 0x3F;
        uint32_t to_square = (packed >> 6) & 0x3F;
        
        from_rank = from_square / 8;
        from_file = from_square % 8;
        to_rank = to_square / 8;
        to_file = to_square % 8;
        
        // Unpack pieces
        piece = int_to_piece((packed >> 12) & 0xF);
        captured_piece = int_to_piece((packed >> 16) & 0xF);
        promotion_piece = int_to_piece((packed >> 20) & 0xF);
        
        // Unpack flags
        uint32_t flags = (packed >> 24) & 0xFF;
        is_castling = (flags & 1) != 0;
        is_en_passant = (flags & 2) != 0;
    }
    
private:
    /**
     * @brief Convert piece character to integer for packing
     */
    static uint32_t piece_to_int(char piece) {
        switch (piece) {
            case '.': return 0;
            case 'P': return 1; case 'p': return 2;
            case 'N': return 3; case 'n': return 4;
            case 'B': return 5; case 'b': return 6;
            case 'R': return 7; case 'r': return 8;
            case 'Q': return 9; case 'q': return 10;
            case 'K': return 11; case 'k': return 12;
            default: return 0;
        }
    }
    
    /**
     * @brief Convert integer back to piece character for unpacking
     */
    static char int_to_piece(uint32_t value) {
        switch (value) {
            case 0: return '.';
            case 1: return 'P'; case 2: return 'p';
            case 3: return 'N'; case 4: return 'n';
            case 5: return 'B'; case 6: return 'b';
            case 7: return 'R'; case 8: return 'r';
            case 9: return 'Q'; case 10: return 'q';
            case 11: return 'K'; case 12: return 'k';
            default: return '.';
        }
    }
};

/**
 * @brief Type alias for a list of moves
 * 
 * Uses SmallVector optimization with fixed-size buffer for typical move counts
 * (~32 moves) to avoid heap allocations in the common case. Falls back to
 * heap storage for larger collections (max legal moves ~218 in chess).
 */
using MoveList = SmallVector<Move, 32>;

#endif // MOVE_H