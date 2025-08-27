#ifndef BITBOARD_MOVE_GENERATOR_H
#define BITBOARD_MOVE_GENERATOR_H

#include "Board.h"
#include "Move.h"
#include "Bitboard.h"
#include <vector>

/**
 * @brief Magic bitboard structure for sliding piece attack generation
 * 
 * Contains the magic bitboard data needed for efficient sliding piece
 * attack generation using the magic bitboard technique.
 */
struct Magic {
    Bitboard mask;
    Bitboard magic;
    Bitboard* attacks;
    int shift;
};

/**
 * @brief Magic bitboard arrays for bishop attack generation
 * 
 * Pre-computed magic bitboard data for all 64 squares, used for
 * efficient sliding piece attack generation.
 */
extern Magic bishop_magics[64];

/**
 * @brief Magic bitboard arrays for rook attack generation
 * 
 * Pre-computed magic bitboard data for all 64 squares, used for
 * efficient sliding piece attack generation.
 */
extern Magic rook_magics[64];

/**
 * BitboardMoveGenerator - High-performance move generation using bitboards
 * This class provides O(1) sliding piece move generation using magic bitboards
 */
class MoveGenerator {
public:
    /**
     * @brief Constructor for MoveGenerator
     * 
     * Initializes the move generator with performance counters set to zero.
     */
    MoveGenerator();

    // Main move generation functions
    /**
     * @brief Generate all pseudo-legal moves for the current position
     * 
     * Generates all possible moves for the active player without checking
     * for legality (moves that would leave the king in check).
     * 
     * @param board The current board position
     * @return Vector containing all pseudo-legal moves
     */
    std::vector<Move> generate_all_moves(const Board& board);
    /**
     * @brief Generate all legal moves for the current position
     * 
     * Generates all legal moves for the active player, filtering out
     * moves that would leave the king in check.
     * 
     * @param board The current board position (non-const for move testing)
     * @return Vector containing all legal moves
     */
    std::vector<Move> generate_legal_moves(Board& board);
    /**
     * @brief Generate all capture moves for the current position
     * 
     * Generates only moves that capture opponent pieces, including
     * en passant captures.
     * 
     * @param board The current board position
     * @return Vector containing all capture moves
     */
    std::vector<Move> generate_captures(const Board& board);
    /**
     * @brief Generate all quiet (non-capture) moves for the current position
     * 
     * Generates moves that do not capture pieces, including castling
     * and pawn pushes.
     * 
     * @param board The current board position
     * @return Vector containing all quiet moves
     */
    std::vector<Move> generate_quiet_moves(const Board& board);

    // Specific piece move generation
    /**
     * @brief Generate all pawn moves for the current position
     * 
     * Generates pawn moves including single/double pushes, captures,
     * and promotions.
     * 
     * @param board The current board position
     * @param moves Vector to append generated moves to
     * @param captures_only If true, only generate capture moves
     */
    void generate_pawn_moves(const Board& board, std::vector<Move>& moves, bool captures_only = false);
    /**
     * @brief Generate all knight moves for the current position
     * 
     * Generates all possible knight moves using pre-computed attack patterns.
     * 
     * @param board The current board position
     * @param moves Vector to append generated moves to
     * @param captures_only If true, only generate capture moves
     */
    void generate_knight_moves(const Board& board, std::vector<Move>& moves, bool captures_only = false);
    /**
     * @brief Generate all bishop moves for the current position
     * 
     * Generates diagonal sliding moves using magic bitboards for
     * efficient attack generation.
     * 
     * @param board The current board position
     * @param moves Vector to append generated moves to
     * @param captures_only If true, only generate capture moves
     */
    void generate_bishop_moves(const Board& board, std::vector<Move>& moves, bool captures_only = false);
    /**
     * @brief Generate all rook moves for the current position
     * 
     * Generates horizontal and vertical sliding moves using magic bitboards
     * for efficient attack generation.
     * 
     * @param board The current board position
     * @param moves Vector to append generated moves to
     * @param captures_only If true, only generate capture moves
     */
    void generate_rook_moves(const Board& board, std::vector<Move>& moves, bool captures_only = false);
    /**
     * @brief Generate all queen moves for the current position
     * 
     * Generates queen moves by combining rook and bishop attack patterns
     * using magic bitboards.
     * 
     * @param board The current board position
     * @param moves Vector to append generated moves to
     * @param captures_only If true, only generate capture moves
     */
    void generate_queen_moves(const Board& board, std::vector<Move>& moves, bool captures_only = false);
    /**
     * @brief Generate all king moves for the current position
     * 
     * Generates king moves to adjacent squares, excluding squares
     * that would put the king in check.
     * 
     * @param board The current board position
     * @param moves Vector to append generated moves to
     * @param captures_only If true, only generate capture moves
     */
    void generate_king_moves(const Board& board, std::vector<Move>& moves, bool captures_only = false);

    // Special moves
    /**
     * @brief Generate castling moves for the current position
     * 
     * Generates kingside and queenside castling moves if the castling
     * conditions are met (king and rook haven't moved, no pieces between,
     * king not in check, king doesn't pass through check).
     * 
     * @param board The current board position
     * @param moves Vector to append generated moves to
     */
    void generate_castling_moves(const Board& board, std::vector<Move>& moves);
    /**
     * @brief Generate en passant capture moves for the current position
     * 
     * Generates en passant captures if an opponent pawn just moved
     * two squares and is adjacent to one of our pawns.
     * 
     * @param board The current board position
     * @param moves Vector to append generated moves to
     */
    void generate_en_passant_moves(const Board& board, std::vector<Move>& moves);

    // Check and legality testing
    /**
     * @brief Check if the specified color's king is in check
     * 
     * Determines if the king of the given color is currently under
     * attack by any opponent piece.
     * 
     * @param board The current board position
     * @param color The color whose king to check
     * @return True if the king is in check, false otherwise
     */
    bool is_in_check(const Board& board, Board::Color color);
    /**
     * @brief Check if a move is legal in the current position
     * 
     * Tests if the given move is legal by making it on the board
     * and checking if it leaves the king in check.
     * 
     * @param board The current board position (modified during testing)
     * @param move The move to test for legality
     * @return True if the move is legal, false otherwise
     */
    bool is_legal_move(Board& board, const Move& move);
    /**
     * @brief Check if a square is attacked by the specified color
     * 
     * Determines if any piece of the given color can attack the
     * specified square.
     * 
     * @param board The current board position
     * @param square The square to check (0-63)
     * @param attacking_color The color of pieces to check for attacks
     * @return True if the square is attacked, false otherwise
     */
    bool is_square_attacked(const Board& board, int square, Board::Color attacking_color);

    // Attack generation
    /**
     * @brief Get all squares attacked by the specified color
     * 
     * Returns a bitboard containing all squares that are under attack
     * by pieces of the given color.
     * 
     * @param board The current board position
     * @param color The color of attacking pieces
     * @return Bitboard with attacked squares set to 1
     */
    Bitboard get_attacked_squares(const Board& board, Board::Color color);
    /**
     * @brief Get attack pattern for a specific piece on a square
     * 
     * Returns the attack bitboard for the specified piece type
     * on the given square, considering current board occupancy.
     * 
     * @param board The current board position
     * @param square The square the piece is on (0-63)
     * @param piece_type The type of piece
     * @param color The color of the piece
     * @return Bitboard with attacked squares set to 1
     */
    Bitboard get_piece_attacks(const Board& board, int square, Board::PieceType piece_type, Board::Color color);
    
    // Magic bitboard attack generation
    /**
     * @brief Get bishop attack pattern using magic bitboards
     * 
     * Uses pre-computed magic bitboard data to efficiently generate
     * bishop attacks for the given square and occupancy.
     * 
     * @param square The square the bishop is on (0-63)
     * @param occupancy Bitboard representing all occupied squares
     * @return Bitboard with bishop attack squares set to 1
     */
    static Bitboard get_bishop_attacks(int square, Bitboard occupancy);
    /**
     * @brief Get rook attack pattern using magic bitboards
     * 
     * Uses pre-computed magic bitboard data to efficiently generate
     * rook attacks for the given square and occupancy.
     * 
     * @param square The square the rook is on (0-63)
     * @param occupancy Bitboard representing all occupied squares
     * @return Bitboard with rook attack squares set to 1
     */
    static Bitboard get_rook_attacks(int square, Bitboard occupancy);
    /**
     * @brief Get queen attack pattern using magic bitboards
     * 
     * Combines bishop and rook attack patterns to generate
     * queen attacks for the given square and occupancy.
     * 
     * @param square The square the queen is on (0-63)
     * @param occupancy Bitboard representing all occupied squares
     * @return Bitboard with queen attack squares set to 1
     */
    static Bitboard get_queen_attacks(int square, Bitboard occupancy);

    // Utility functions
    /**
     * @brief Count the total number of legal moves in the position
     * 
     * Generates all legal moves and returns the count. Useful for
     * perft testing and position analysis.
     * 
     * @param board The current board position
     * @return Number of legal moves available
     */
    int count_moves(const Board& board);
    /**
     * @brief Check if the current position has any legal moves
     * 
     * Efficiently determines if there are any legal moves available
     * without generating the complete move list.
     * 
     * @param board The current board position (non-const for move testing)
     * @return True if legal moves exist, false if checkmate/stalemate
     */
    bool has_legal_moves(Board& board);

private:
    // Helper functions for move generation
    /**
     * @brief Add moves from a bitboard to the move list
     * 
     * Helper function that converts bitboard move patterns into Move objects
     * and adds them to the provided move vector.
     * 
     * @param from_square Bitboard with the source square set
     * @param to_squares Bitboard with destination squares set
     * @param piece_type Type of piece making the move
     * @param color Color of the piece making the move
     * @param board Current board position for capture detection
     * @param moves Vector to append generated moves to
     * @param captures_only If true, only add capture moves
     */
    /**
     * @brief Add moves from a bitboard of target squares
     * 
     * Helper function that converts a bitboard of target squares
     * into Move objects and adds them to the move list.
     * 
     * @param from_square The source square bitboard
     * @param to_squares Bitboard of target squares
     * @param piece_type The type of piece making the moves
     * @param color The color of the piece
     * @param board The current board position
     * @param moves Vector to append generated moves to
     * @param captures_only If true, only add capture moves
     */
    void add_moves_from_bitboard(Bitboard from_square, Bitboard to_squares,
                                Board::PieceType piece_type, Board::Color color,
                                const Board& board, std::vector<Move>& moves, bool captures_only = false);

    /**
     * @brief Add pawn moves with special handling for promotions
     * 
     * Helper function specifically for pawn moves that handles promotion
     * logic when pawns reach the back rank.
     * 
     * @param from_square Source square of the pawn (0-63)
     * @param to_squares Bitboard with destination squares set
     * @param color Color of the pawn
     * @param board Current board position
     * @param moves Vector to append generated moves to
     * @param is_capture True if this is a capture move
     */
    void add_pawn_moves(int from_square, Bitboard to_squares, Board::Color color,
                       const Board& board, std::vector<Move>& moves, bool is_capture = false);

    /**
     * @brief Add all promotion moves for a pawn reaching the back rank
     * 
     * Generates moves for all four promotion pieces (queen, rook, bishop, knight)
     * when a pawn reaches the promotion rank.
     * 
     * @param from_square Source square of the promoting pawn (0-63)
     * @param to_square Destination square (0-63)
     * @param color Color of the promoting pawn
     * @param board Current board position
     * @param moves Vector to append generated moves to
     * @param is_capture True if this is a capture promotion
     */
    void add_promotion_moves(int from_square, int to_square, Board::Color color,
                            const Board& board, std::vector<Move>& moves, bool is_capture = false);

    // Castling helpers
    /**
     * @brief Check if kingside castling is possible
     * 
     * Verifies all conditions for kingside castling: king and rook haven't moved,
     * no pieces between them, king not in check, king doesn't pass through check.
     * 
     * @param board The current board position
     * @param color The color attempting to castle
     * @return True if kingside castling is legal, false otherwise
     */
    bool can_castle_kingside(const Board& board, Board::Color color);
    /**
     * @brief Check if queenside castling is possible
     * 
     * Verifies all conditions for queenside castling: king and rook haven't moved,
     * no pieces between them, king not in check, king doesn't pass through check.
     * 
     * @param board The current board position
     * @param color The color attempting to castle
     * @return True if queenside castling is legal, false otherwise
     */
    bool can_castle_queenside(const Board& board, Board::Color color);

    // Pin and discovery helpers
    /**
     * @brief Get all pieces that are pinned to the king
     * 
     * Returns a bitboard of pieces that cannot move because doing so
     * would expose the king to check.
     * 
     * @param board The current board position
     * @param color The color whose pinned pieces to find
     * @return Bitboard with pinned pieces set to 1
     */
    Bitboard get_pinned_pieces(const Board& board, Board::Color color);
    /**
     * @brief Get the check mask for the current position
     * 
     * Returns a bitboard indicating squares that must be blocked or
     * the attacking piece captured to get out of check.
     * 
     * @param board The current board position
     * @param color The color that is in check
     * @return Bitboard with blocking/capture squares set to 1
     */
    Bitboard get_check_mask(const Board& board, Board::Color color);
    /**
     * @brief Get squares between two given squares
     * 
     * Returns a bitboard with all squares on the line between
     * two squares (exclusive of the endpoints).
     * 
     * @param sq1 First square (0-63)
     * @param sq2 Second square (0-63)
     * @return Bitboard with squares between sq1 and sq2 set to 1
     */
    Bitboard get_between_squares(int sq1, int sq2);
    /**
     * @brief Get all pieces attacking a specific square
     * 
     * Returns a bitboard of all pieces of the specified color
     * that can attack the given square.
     * 
     * @param board The current board position
     * @param square The target square (0-63)
     * @param attacking_color The color of attacking pieces
     * @return Bitboard with attacking pieces set to 1
     */
    Bitboard get_attackers_to_square(const Board& board, int square, Board::Color attacking_color);

    // Move filtering and legality
    /**
     * @brief Check if a move is legal when the king is in check
     * 
     * Specialized legality check for positions where the king is in check.
     * Uses check mask and pinned pieces for efficient validation.
     * 
     * @param board The current board position
     * @param move The move to validate
     * @param check_mask Bitboard indicating blocking/capture squares
     * @param pinned_pieces Bitboard of pieces pinned to the king
     * @return True if the move is legal, false otherwise
     */
    bool is_move_legal_in_check(const Board& board, const Move& move, Bitboard check_mask, Bitboard pinned_pieces);
    /**
     * @brief Check if three squares are aligned on the same rank, file, or diagonal
     * 
     * Utility function to determine if three squares lie on the same line,
     * useful for pin and discovery attack detection.
     * 
     * @param sq1 First square (0-63)
     * @param sq2 Second square (0-63)
     * @param sq3 Third square (0-63)
     * @return True if all three squares are aligned, false otherwise
     */
    bool are_squares_aligned(int sq1, int sq2, int sq3);

    // Move ordering
    /**
     * @brief Order moves for better search performance
     * 
     * Sorts moves to improve alpha-beta pruning efficiency by placing
     * potentially better moves first.
     * 
     * @param moves Vector of moves to order
     * @param board The current board position
     */
    void order_moves(std::vector<Move>& moves, const Board& board);
    /**
     * @brief Order capture moves by Most Valuable Victim - Least Valuable Attacker
     * 
     * Sorts capture moves to prioritize capturing high-value pieces
     * with low-value pieces.
     * 
     * @param moves Vector of capture moves to order
     * @param board The current board position
     */
    void order_captures(std::vector<Move>& moves, const Board& board);
    /**
     * @brief Get a heuristic score for move ordering
     * 
     * Calculates a score for the given move to aid in move ordering
     * for search optimization.
     * 
     * @param move The move to score
     * @param board The current board position
     * @return Heuristic score for the move
     */
    int get_move_score(const Move& move, const Board& board);
    /**
     * @brief Get a score for capture moves using MVV-LVA
     * 
     * Calculates Most Valuable Victim - Least Valuable Attacker score
     * for capture move ordering.
     * 
     * @param move The capture move to score
     * @param board The current board position
     * @return MVV-LVA score for the capture
     */
    int get_capture_score(const Move& move, const Board& board);
    /**
     * @brief Convert character to piece type enumeration
     * 
     * Helper function to convert piece characters to PieceType enum values.
     * 
     * @param piece Character representing the piece
     * @return Corresponding PieceType enum value
     */
    int char_to_piece_type(char piece);

    // Utility functions
    /**
     * @brief Convert piece type and color to character representation
     * 
     * Helper function to convert PieceType enum and Color to the
     * corresponding character representation.
     * 
     * @param piece_type The type of piece
     * @param color The color of the piece
     * @return Character representing the piece
     */
    char piece_type_to_char(Board::PieceType piece_type, Board::Color color);
    /**
     * @brief Check if a rank is a promotion rank for the given color
     * 
     * Determines if pawns of the specified color promote when reaching
     * the given rank.
     * 
     * @param rank The rank to check (0-7)
     * @param color The color of the pawn
     * @return True if it's a promotion rank, false otherwise
     */
    bool is_promotion_rank(int rank, Board::Color color);
    
    // Performance counters
    /**
     * @brief Counter for nodes searched during move generation
     * 
     * Mutable counter used for performance analysis and debugging.
     */
    mutable uint64_t nodes_searched;
    /**
     * @brief Counter for total moves generated
     * 
     * Mutable counter used for performance analysis and debugging.
     */
    mutable uint64_t moves_generated;
};

#endif // BITBOARD_MOVE_GENERATOR_H