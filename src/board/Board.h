#ifndef BITBOARD_BOARD_H
#define BITBOARD_BOARD_H

#include "Bitboard.h"
#include "Move.h"
#include <string>
#include <array>
#include <cstdint>
#include <random>

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
    
    // Zobrist hashing
    uint64_t zobrist_hash;   ///< Current Zobrist hash of the position
    
    // Static Zobrist hash tables
    static uint64_t zobrist_pieces[2][6][64];  ///< [color][piece_type][square]
    static uint64_t zobrist_castling[16];      ///< [castling_rights]
    static uint64_t zobrist_en_passant[8];     ///< [file]
    static uint64_t zobrist_side_to_move;      ///< XOR when black to move
    static bool zobrist_initialized;           ///< Flag to ensure one-time initialization
    
    // King positions for quick access
    std::array<int, NUM_COLORS> king_positions{};
    
    // Piece mailbox for O(1) square access
    char piece_mailbox[64];
    
public:
    /**
     * @brief Default constructor for the Board class
     * 
     * Initializes a new chess board with all bitboards cleared and
     * game state variables set to default values. The board is empty
     * after construction and requires calling set_starting_position()
     * or set_from_fen() to set up a valid chess position.
     * 
     * Initial state:
     * - All piece bitboards are empty
     * - Active color is set to WHITE
     * - No castling rights
     * - No en passant square
     * - Move counters reset to 0
     * - King positions set to invalid (-1)
     * - Piece mailbox filled with empty squares ('.')
     */
    Board();
    
    /**
     * @brief Copy constructor for fast Board copying
     * 
     * Creates a deep copy of another Board instance using optimized
     * memory operations. This is essential for parallel search where
     * each thread needs its own Board copy to avoid contention.
     * 
     * Uses memcpy for fast copying of bitboard arrays and other
     * fixed-size data structures.
     * 
     * @param other The Board instance to copy from
     */
    Board(const Board& other);
    
    /**
     * @brief Assignment operator for Board copying
     * 
     * Assigns the state of another Board to this instance using
     * optimized memory operations.
     * 
     * @param other The Board instance to copy from
     * @return Reference to this Board instance
     */
    Board& operator=(const Board& other);
    
    /**
     * @brief Fast clone method for creating Board copies
     * 
     * Creates a new Board instance that is an exact copy of this one.
     * Optimized for performance in parallel search scenarios.
     * 
     * @return A new Board instance that is a copy of this one
     */
    Board clone() const;
    
    // Board setup
    /**
     * @brief Sets up the standard chess starting position
     * 
     * Configures the board to the standard chess starting position
     * as defined by FIDE rules. This includes:
     * - Placing all pieces in their initial squares
     * - Setting white as the active color
     * - Enabling all castling rights (KQkq)
     * - Clearing en passant square
     * - Resetting move counters
     * - Updating all internal data structures
     * 
     * The resulting position corresponds to the FEN:
     * "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
     */
    void set_starting_position();
    
    /**
     * @brief Sets the board position from a FEN (Forsyth-Edwards Notation) string
     * 
     * Parses a FEN string and configures the board accordingly. FEN notation
     * describes a chess position using six space-separated fields:
     * 1. Piece placement (rank 8 to rank 1, '/' separates ranks)
     * 2. Active color ('w' for white, 'b' for black)
     * 3. Castling availability (KQkq or '-' if none)
     * 4. En passant target square (algebraic notation or '-')
     * 5. Halfmove clock (moves since last capture or pawn move)
     * 6. Fullmove number (incremented after black's move)
     * 
     * The function validates the FEN format and updates all internal
     * data structures including bitboards, mailbox, and game state.
     * 
     * @param fen A valid FEN string representing the desired position
     * @throws std::invalid_argument if the FEN string is malformed
     */
    void set_from_fen(const std::string& fen);
    
    /**
     * @brief Convert the current board position to FEN notation
     * 
     * Generates a FEN (Forsyth-Edwards Notation) string representing
     * the current board state, including piece positions, active color,
     * castling rights, en passant square, and move counters.
     * 
     * @return A FEN string representing the current position
     */
    [[nodiscard]] std::string to_fen() const;
    
    // Piece manipulation
    /**
     * @brief Sets a piece at the specified rank and file coordinates
     * 
     * Places a piece character at the given board coordinates using
     * traditional chess notation (rank 1-8, file a-h converted to 0-7).
     * The piece character should follow standard notation:
     * - Uppercase for white pieces (K, Q, R, B, N, P)
     * - Lowercase for black pieces (k, q, r, b, n, p)
     * - '.' for empty squares
     * 
     * This function updates both the bitboard representation and the
     * piece mailbox. If placing a piece on an occupied square, the
     * existing piece is replaced.
     * 
     * @param rank The rank (row) coordinate (0-7, where 0 is rank 1)
     * @param file The file (column) coordinate (0-7, where 0 is file a)
     * @param piece The piece character to place ('K', 'Q', 'R', 'B', 'N', 'P', 'k', 'q', 'r', 'b', 'n', 'p', or '.')
     */
    void set_piece(int rank, int file, char piece);
    
    /**
     * @brief Gets the piece character at the specified rank and file coordinates
     * 
     * Retrieves the piece at the given board coordinates using traditional
     * chess notation. Returns the piece character following standard notation:
     * - Uppercase for white pieces (K, Q, R, B, N, P)
     * - Lowercase for black pieces (k, q, r, b, n, p)
     * - '.' for empty squares
     * 
     * @param rank The rank (row) coordinate (0-7, where 0 is rank 1)
     * @param file The file (column) coordinate (0-7, where 0 is file a)
     * @return The piece character at the specified coordinates
     */
    [[nodiscard]] char get_piece(int rank, int file) const;
    
    /**
     * @brief Removes a piece from the specified square
     * 
     * This function clears a piece from the given square by:
     * 1. Checking if a piece exists on the square
     * 2. Determining the piece type and color
     * 3. Clearing the corresponding bit in the piece bitboard
     * 4. Setting the mailbox entry to empty ('.')
     * 5. Updating the combined bitboards
     * 
     * If the square is already empty, no action is taken.
     * 
     * @param square The square index (0-63) to clear
     */
    void clear_square(int square);
    
    /**
     * @brief Places a piece of the specified type and color on the given square
     * 
     * This function places a piece on the board by:
     * 1. Setting the corresponding bit in the piece bitboard
     * 2. Updating the mailbox with the piece character
     * 3. Updating the king position if placing a king (using branchless logic)
     * 4. Updating the combined bitboards
     * 
     * This function assumes the target square is valid and does not perform
     * validation. It will overwrite any existing piece on the square.
     * 
     * @param square The square index (0-63) where to place the piece
     * @param piece_type The type of piece to place (PAWN, KNIGHT, BISHOP, etc.)
     * @param color The color of the piece (WHITE or BLACK)
     */
    void place_piece(int square, PieceType piece_type, Color color);
    
    // Bitboard access
    /**
     * @brief Returns the bitboard for a specific piece type and color
     * 
     * Retrieves the bitboard containing all pieces of the specified
     * type and color. Each bit in the returned bitboard represents
     * a square on the chess board, with set bits indicating the
     * presence of the requested piece type.
     * 
     * @param piece_type The type of piece (PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING)
     * @param color The color of the pieces (WHITE or BLACK)
     * @return Bitboard with bits set for squares containing the specified piece type and color
     */
    [[nodiscard]] Bitboard get_piece_bitboard(PieceType piece_type, Color color) const;
    
    /**
     * @brief Returns the combined bitboard for all pieces of a specific color
     * 
     * Retrieves a bitboard containing all pieces of the specified color,
     * regardless of piece type. This is useful for quickly determining
     * which squares are occupied by a particular side.
     * 
     * @param color The color of the pieces (WHITE or BLACK)
     * @return Bitboard with bits set for all squares occupied by pieces of the specified color
     */
    [[nodiscard]] Bitboard get_color_bitboard(Color color) const;
    
    /**
     * @brief Returns a bitboard containing all pieces on the board
     * 
     * Retrieves a bitboard with bits set for every square that contains
     * a piece, regardless of color or type. This is useful for occupancy
     * checks and move generation algorithms that need to know which
     * squares are occupied.
     * 
     * @return Bitboard with bits set for all occupied squares
     */
    [[nodiscard]] Bitboard get_all_pieces() const;
    
    // Game state access
    [[nodiscard]] Color get_active_color() const { return active_color; }
    void set_active_color(Color color) { active_color = color; }
    [[nodiscard]] char get_active_color_char() const { return active_color == WHITE ? 'w' : 'b'; }
    
    [[nodiscard]] uint8_t get_castling_rights() const { return castling_rights; }
    void set_castling_rights(uint8_t rights) { castling_rights = rights; }
    
    [[nodiscard]] int8_t get_en_passant_file() const { return en_passant_file; }
    
    /**
     * @brief Get the current Zobrist hash of the position
     * 
     * Returns the 64-bit Zobrist hash that uniquely identifies the current
     * board position, including piece placement, active color, castling rights,
     * and en passant availability.
     * 
     * @return 64-bit Zobrist hash of the current position
     */
    uint64_t get_zobrist_hash() const { return zobrist_hash; }
    void set_en_passant_file(int8_t file) { en_passant_file = file; }
    
    [[nodiscard]] int get_halfmove_clock() const { return halfmove_clock; }
    void set_halfmove_clock(int clock) { halfmove_clock = clock; }
    
    [[nodiscard]] int get_fullmove_number() const { return fullmove_number; }
    void set_fullmove_number(int number) { fullmove_number = number; }
    
    [[nodiscard]] int get_king_position(Color color) const { return king_positions[color]; }
    
    // Move operations
    /**
     * @brief Validates basic move properties without checking legality
     * 
     * Performs basic validation of a move's format and bounds checking
     * without considering the current board position or chess rules.
     * This includes:
     * - Verifying source and destination squares are within bounds (0-63)
     * - Checking that source and destination squares are different
     * - Validating promotion piece type if applicable
     * 
     * This function does NOT check:
     * - Whether there's a piece on the source square
     * - Whether the move follows piece movement rules
     * - Whether the move leaves the king in check
     * 
     * Use is_move_legal() for complete move validation.
     * 
     * @param move The move to validate
     * @return true if the move has valid basic properties, false otherwise
     */
    [[nodiscard]] bool is_move_valid(const Move& move) const;
    
    /**
     * @brief Check if a move is legal in the current position
     * 
     * Determines if a move is legal by generating all legal moves
     * and checking if the given move is among them. This ensures
     * the move doesn't leave the king in check.
     * 
     * @param move The move to check for legality
     * @return true if the move is legal, false otherwise
     */
    bool is_move_legal(const Move& move);
    
    /**
     * @brief Make a move on the board if it's legal
     * 
     * Attempts to make the given move on the board. If the move
     * is legal, it will be applied and undo data will be returned.
     * If the move is illegal, no changes are made and empty undo data is returned.
     * 
     * @param move The move to make
     * @return BitboardMoveUndoData for undoing the move, or empty data if move was illegal
     */
    BitboardMoveUndoData make_move(const Move& move);
    
    /**
     * @brief Apply a move to the board without legality checking
     * 
     * Directly applies the given move to the board, updating all
     * relevant bitboards, game state, and piece positions. This function
     * assumes the move is legal and does not perform validation.
     * 
     * @param move The move to apply
     * @return BitboardMoveUndoData containing information needed to undo the move
     */
    BitboardMoveUndoData apply_move(const Move& move);
    
    /**
     * @brief Undoes a previously applied move using the provided undo data
     * 
     * Restores the board to its state before the move was applied by:
     * - Moving the piece back to its original square
     * - Restoring any captured piece
     * - Reverting castling rights, en passant square, and move counters
     * - Handling special move types (castling, en passant, promotion)
     * - Updating all internal data structures
     * 
     * The undo data must correspond to the move that was most recently
     * applied to maintain board state consistency. This function assumes
     * the undo data is valid and does not perform validation.
     * 
     * @param undo_data The undo information returned by apply_move() or make_move()
     */
    void undo_move(const BitboardMoveUndoData& undo_data);
    
    // Attack and check detection
    /**
     * @brief Determines if a square is under attack by pieces of the specified color
     * 
     * Checks whether any piece of the attacking color can legally move to
     * the target square in the current position. This function considers
     * all piece types and their movement patterns:
     * - Pawns: Diagonal capture moves
     * - Knights: L-shaped moves
     * - Bishops: Diagonal sliding moves
     * - Rooks: Horizontal and vertical sliding moves
     * - Queens: Combination of bishop and rook moves
     * - Kings: Adjacent square moves
     * 
     * The function accounts for piece blocking for sliding pieces and
     * uses efficient bitboard operations for fast computation.
     * 
     * @param square The target square index (0-63) to check for attacks
     * @param attacking_color The color (WHITE or BLACK) of pieces to check for attacks
     * @return true if the square is attacked by the specified color, false otherwise
     */
    [[nodiscard]] bool is_square_attacked(int square, Color attacking_color) const;
    
    /**
     * @brief Checks if the king of the specified color is currently in check
     * 
     * Determines whether the king of the given color is under attack
     * by any piece of the opposing color. This is equivalent to calling
     * is_square_attacked() on the king's current position.
     * 
     * This function is commonly used to:
     * - Validate move legality (moves that leave own king in check are illegal)
     * - Detect checkmate and stalemate conditions
     * - Implement check extensions in search algorithms
     * 
     * @param color The color (WHITE or BLACK) whose king to check
     * @return true if the king is in check, false otherwise
     */
    [[nodiscard]] bool is_in_check(Color color) const;
    
    // Utility functions
    /**
     * @brief Prints a visual representation of the board to the console
     * 
     * Outputs a human-readable ASCII representation of the current board
     * position to standard output. The display includes:
     * - 8x8 grid showing piece positions using standard notation
     * - Rank and file labels for easy coordinate reference
     * - Clear visual separation between squares
     * 
     * This function is primarily used for debugging and development
     * purposes to quickly visualize the current board state.
     */
    void print() const;
    
    /**
     * @brief Converts the board to a string representation
     * 
     * Creates a string representation of the current board position
     * that can be used for display, logging, or serialization purposes.
     * The format may include:
     * - ASCII art representation of the board
     * - Piece positions in a structured format
     * - Additional game state information
     * 
     * @return A string representation of the current board state
     */
    [[nodiscard]] std::string to_string() const;
    
    // Convert between coordinate systems
    /**
     * @brief Converts a piece character to its corresponding piece type
     * 
     * Extracts the piece type from a standard chess piece character,
     * ignoring case (which indicates color). The mapping is:
     * - 'P'/'p' -> PAWN
     * - 'N'/'n' -> KNIGHT
     * - 'B'/'b' -> BISHOP
     * - 'R'/'r' -> ROOK
     * - 'Q'/'q' -> QUEEN
     * - 'K'/'k' -> KING
     * 
     * @param piece The piece character to convert
     * @return The corresponding PieceType enumeration value
     * @throws std::invalid_argument if the character is not a valid piece
     */
    static PieceType char_to_piece_type(char piece);
    
    /**
     * @brief Determines the color of a piece from its character representation
     * 
     * Extracts the piece color based on the case of the piece character:
     * - Uppercase letters (K, Q, R, B, N, P) represent WHITE pieces
     * - Lowercase letters (k, q, r, b, n, p) represent BLACK pieces
     * 
     * @param piece The piece character to analyze
     * @return WHITE for uppercase characters, BLACK for lowercase characters
     * @throws std::invalid_argument if the character is not a valid piece
     */
    static Color char_to_color(char piece);
    
    /**
     * @brief Converts a piece type and color to its character representation
     * 
     * Creates the standard chess piece character from the given piece type
     * and color combination. The resulting character follows the convention:
     * - WHITE pieces: uppercase (K, Q, R, B, N, P)
     * - BLACK pieces: lowercase (k, q, r, b, n, p)
     * 
     * This function is the inverse of char_to_piece_type() and char_to_color().
     * 
     * @param piece_type The type of piece (PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING)
     * @param color The color of the piece (WHITE or BLACK)
     * @return The corresponding piece character
     */
    static char piece_to_char(PieceType piece_type, Color color);
    
private:
    // Internal helper functions
    
    /**
     * @brief Updates the combined bitboards after piece movements
     * 
     * This function recalculates the combined bitboards that are derived from
     * the individual piece bitboards. It updates:
     * 1. Color bitboards (white_pieces, black_pieces) by OR-ing all piece types for each color
     * 2. All pieces bitboard by OR-ing both color bitboards
     * 
     * This function should be called after any operation that modifies the
     * individual piece bitboards to maintain consistency in the board representation.
     * 
     * Time complexity: O(1) - fixed number of operations regardless of board state
     */
    void update_combined_bitboards();
    
    // Zobrist hashing functionality
    
    /**
     * @brief Initialize Zobrist hash tables with random 64-bit values
     * 
     * This method initializes the static Zobrist hash tables used for
     * position hashing. It should be called once at program startup.
     */
    static void init_zobrist_tables();
    
    /**
     * @brief Calculate the full Zobrist hash for the current position
     * 
     * Computes the complete Zobrist hash by XORing hash values for:
     * - All pieces on their squares
     * - Active color
     * - Castling rights
     * - En passant file (if any)
     * 
     * @return 64-bit Zobrist hash of the current position
     */
    uint64_t calculate_zobrist_hash() const;
    
    /**
     * @brief Update Zobrist hash incrementally when making a move
     * 
     * Updates the current Zobrist hash by XORing out old values and
     * XORing in new values for the changed position elements.
     * 
     * @param move The move being applied
     * @param old_castling_rights Previous castling rights
     * @param old_en_passant_file Previous en passant file (-1 if none)
     */
    void update_zobrist_hash(const Move& move, uint8_t old_castling_rights, int old_en_passant_file);
    
    /**
     * @brief Updates the cached king position for the specified color
     * 
     * This function finds the king's position by examining the king bitboard
     * and caches the result in the king_positions array for fast access.
     * If no king is found (which should not happen in normal play), the
     * position is set to -1.
     * 
     * This function is typically called after moves that might affect the
     * king position or during board initialization.
     * 
     * @param color The color (WHITE or BLACK) whose king position to update
     */
    void update_king_position(Color color);
    
    /**
     * @brief Returns a bitboard of all pieces of the specified color that attack the given square
     * 
     * This function calculates which pieces of the attacking color can attack the target square.
     * It considers all piece types and their movement patterns:
     * - Pawns: Uses pawn attack patterns (diagonal captures)
     * - Knights: Uses knight movement patterns (L-shaped moves)
     * - Bishops/Queens: Uses diagonal sliding attacks with occupancy
     * - Rooks/Queens: Uses rank/file sliding attacks with occupancy
     * - Kings: Uses adjacent square attacks
     * 
     * The function uses bitboard operations for efficiency and considers piece
     * blocking for sliding pieces (bishops, rooks, queens).
     * 
     * @param square The target square index (0-63) to check for attackers
     * @param attacking_color The color (WHITE or BLACK) of pieces to check for attacks
     * @return Bitboard with bits set for each piece that attacks the target square
     */
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
    uint64_t zobrist_hash;
    
    BitboardMoveUndoData() : captured_piece('.'), castling_rights(0), 
                            en_passant_file(-1), halfmove_clock(0), zobrist_hash(0) {}
};

#endif // BITBOARD_BOARD_H