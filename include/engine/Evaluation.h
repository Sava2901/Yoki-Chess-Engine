#ifndef EVALUATION_H
#define EVALUATION_H

#include "../board/Board.h"
#include "../board/Move.h"
#include <cstdint>
#include <unordered_map>

// Forward declarations
struct BitboardMoveUndoData;

/**
 * @enum GamePhase
 * @brief Represents the current phase of the chess game
 */
enum GamePhase {
    OPENING = 0,     ///< Opening phase with many pieces on board
    MIDDLEGAME = 1,  ///< Middlegame phase with tactical complexity
    ENDGAME = 2      ///< Endgame phase with few pieces remaining
};

/**
 * @namespace EvalConstants
 * @brief Contains all evaluation constants and parameters
 * 
 * This namespace defines material values, positional bonuses/penalties,
 * and various evaluation factors used throughout the evaluation function.
 * All values are in centipawns (1/100th of a pawn).
 */
namespace EvalConstants {
    // Material values (centipawns) - refined values
    constexpr int PAWN_VALUE = 100;
    constexpr int KNIGHT_VALUE = 325;
    constexpr int BISHOP_VALUE = 335;
    constexpr int ROOK_VALUE = 500;
    constexpr int QUEEN_VALUE = 975;
    constexpr int KING_VALUE = 20000;
    
    // Phase values for determining game phase
    constexpr int KNIGHT_PHASE = 1;
    constexpr int BISHOP_PHASE = 1;
    constexpr int ROOK_PHASE = 2;
    constexpr int QUEEN_PHASE = 4;
    constexpr int TOTAL_PHASE = KNIGHT_PHASE * 4 + BISHOP_PHASE * 4 + ROOK_PHASE * 4 + QUEEN_PHASE * 2;
    
    // Pawn structure bonuses/penalties - enhanced
    constexpr int ISOLATED_PAWN_PENALTY = -12;
    constexpr int DOUBLED_PAWN_PENALTY = -18;
    constexpr int BACKWARD_PAWN_PENALTY = -8;
    constexpr int PASSED_PAWN_BONUS = 25;
    constexpr int CONNECTED_PAWNS_BONUS = 8;
    constexpr int PAWN_CHAIN_BONUS = 12;
    constexpr int ADVANCED_PASSED_PAWN_BONUS = 15; // Per rank advanced
    
    // King safety pawn penalties
    constexpr int PAWN_STORM_AGAINST_KING_PENALTY = -20;
    constexpr int WEAKENED_KING_SHELTER_PENALTY = -15;
    
    // Development limiting pawn penalties
    constexpr int BISHOP_BLOCKING_PAWN_PENALTY = -25;
    constexpr int KNIGHT_BLOCKING_PAWN_PENALTY = -20;
    constexpr int CENTER_PAWN_PREMATURE_ADVANCE_PENALTY = -15;
    
    // King safety
    constexpr int OPEN_FILE_NEAR_KING_PENALTY = -20;
    constexpr int SEMI_OPEN_FILE_NEAR_KING_PENALTY = -10;

    // Additional king safety factors
    constexpr int KING_ON_OPEN_FILE_PENALTY = -30;
    constexpr int KING_EXPOSED_PENALTY = -25;
    constexpr int KING_PAWN_SHIELD = 15;
    constexpr int PAWN_SHIELD_BONUS = 8;
    constexpr int FIANCHETTO_BONUS = 12;
    constexpr int KING_ACTIVITY_PENALTY = -8; // In middlegame
    constexpr int PIN_ON_KING_PENALTY = -15;
    constexpr int BACK_RANK_WEAKNESS_PENALTY = -30;
    constexpr int KING_ESCAPE_SQUARES_BONUS = 4; // Per escape square
    constexpr int KING_ACTIVITY_BONUS = 6; // For king centralization in endgame
    
    // Mobility
    constexpr int KNIGHT_MOBILITY_BONUS = 4;
    constexpr int BISHOP_MOBILITY_BONUS = 3;
    constexpr int ROOK_MOBILITY_BONUS = 2;
    constexpr int QUEEN_MOBILITY_BONUS = 1;
    
    // Development and tempo
    constexpr int EARLY_QUEEN_DEVELOPMENT_PENALTY = -15;
    constexpr int PIECE_DEVELOPMENT_BONUS = 8;
    constexpr int TEMPO_BONUS = 10;
    constexpr int DEVELOPMENT_BONUS = 5;
    constexpr int CASTLING_BONUS = 20;
    
    // Piece coordination
    constexpr int KNIGHT_OUTPOST_BONUS = 18;
    constexpr int BISHOP_PAIR_BONUS = 30;
    constexpr int ROOK_ON_OPEN_FILE_BONUS = 20;
    constexpr int ROOK_ON_SEMI_OPEN_FILE_BONUS = 10;
    constexpr int ROOK_ON_SEVENTH_BONUS = 25;
    constexpr int ROOK_COORDINATION_BONUS = 12;
    constexpr int QUEEN_ROOK_BATTERY_BONUS = 15;
    
    // Endgame factors
    constexpr int OPPOSITION_BONUS = 20;
    constexpr int CENTRALIZATION_BONUS = 10;
    constexpr int KING_NEAR_ENEMY_PAWNS_BONUS = 6;
    constexpr int CONNECTED_PASSED_PAWNS_BONUS = 20;
}

/**
 * @struct IncrementalEvalData
 * @brief Stores incremental evaluation data for efficient position updates
 * 
 * This structure maintains evaluation components that can be updated
 * incrementally as moves are made and unmade, avoiding full re-evaluation.
 */
struct IncrementalEvalData {
    int material_balance;      ///< Current material balance (White - Black)
    int positional_balance;    ///< Positional evaluation balance
    int pawn_structure_score;  ///< Pawn structure evaluation score
    int king_safety_score;     ///< King safety evaluation score
    int mobility_score;        ///< Piece mobility evaluation score
    GamePhase game_phase;      ///< Current game phase
    int phase_value;          ///< Numeric phase value for tapered evaluation
    
    /**
     * @brief Default constructor - initializes all values to zero/opening
     */
    IncrementalEvalData() : material_balance(0), positional_balance(0), 
                           pawn_structure_score(0), king_safety_score(0),
                           mobility_score(0), game_phase(OPENING), phase_value(0) {}
};

/**
 * @struct ZobristKeys
 * @brief Zobrist hashing keys for position hashing and transposition tables
 * 
 * Contains precomputed random keys for all chess position components
 * to enable fast position hashing for transposition tables and repetition detection.
 */
struct ZobristKeys {
    uint64_t piece_keys[2][6][64];  ///< Keys for pieces: [color][piece_type][square]
    uint64_t castling_keys[16];     ///< Keys for all castling right combinations
    uint64_t en_passant_keys[8];    ///< Keys for en passant target files
    uint64_t side_to_move_key;      ///< Key for side to move
    
    /**
     * @brief Constructor - calls initialize()
     */
    ZobristKeys();
    
    /**
     * @brief Initializes all Zobrist keys with random values
     */
    void initialize();
};

/**
 * @struct PawnHashEntry
 * @brief Hash table entry for caching pawn structure evaluations
 * 
 * Stores pawn structure evaluation results to avoid recalculating
 * expensive pawn structure analysis for identical pawn configurations.
 */
struct PawnHashEntry {
    uint64_t key;                   ///< Zobrist hash key for this pawn structure
    int score;                      ///< Cached pawn structure evaluation score
    uint8_t passed_pawns_white;     ///< Bitboard of White's passed pawns
    uint8_t passed_pawns_black;     ///< Bitboard of Black's passed pawns
    uint8_t isolated_pawns_white;   ///< Bitboard of White's isolated pawns
    uint8_t isolated_pawns_black;   ///< Bitboard of Black's isolated pawns
    
    /**
     * @brief Default constructor - initializes all values to zero
     */
    PawnHashEntry() : key(0), score(0), passed_pawns_white(0), 
                     passed_pawns_black(0), isolated_pawns_white(0), 
                     isolated_pawns_black(0) {}
};

/**
 * @class Evaluation
 * @brief Comprehensive chess position evaluation engine with incremental updates
 * 
 * This class provides a sophisticated evaluation system that assesses chess positions
 * using multiple factors including material, positional values, pawn structure,
 * king safety, piece mobility, and coordination. Supports both full evaluation
 * and incremental updates for performance optimization.
 */
class Evaluation {
public:
    /**
     * @brief Constructor - initializes evaluation tables and data structures
     */
    Evaluation();
    
    /**
     * @brief Default destructor
     */
    ~Evaluation() = default;
    
    /**
     * @brief Evaluates a chess position comprehensively
     * @param board The board position to evaluate
     * @return Evaluation score in centipawns (positive favors White, negative favors Black)
     */
    int evaluate(const Board& board);
    
    /**
     * @brief Performs incremental evaluation update based on a move
     * @param board The current board position
     * @param move The move that was made
     * @param undo_data Move undo information for incremental updates
     * @return Updated evaluation score in centipawns
     */
    int evaluate_incremental(const Board& board, const Move& move, const BitboardMoveUndoData& undo_data);
    
    /**
     * @brief Initializes incremental evaluation data for a board position
     * @param board The board position to initialize evaluation data for
     */
    void initialize_incremental_eval(const Board& board);
    
    /**
     * @brief Updates incremental evaluation data after a move
     * @param board The current board position
     * @param move The move that was made
     * @param undo_data Move undo information for tracking changes
     */
    void update_incremental_eval(const Board& board, const Move& move, const BitboardMoveUndoData& undo_data);
    
    /**
     * @brief Undoes incremental evaluation changes when unmaking a move
     * @param board The current board position
     * @param move The move to undo
     * @param undo_data Move undo information for reverting changes
     */
    void undo_incremental_eval(const Board& board, const Move& move, const BitboardMoveUndoData& undo_data);
    
    /**
     * @brief Computes Zobrist hash for a board position
     * @param board The board position to hash
     * @return 64-bit Zobrist hash value
     */
    uint64_t compute_zobrist_hash(const Board& board) const;
    
    /**
     * @brief Updates Zobrist hash incrementally after a move
     * @param current_hash The current hash value
     * @param move The move that was made
     * @param undo_data Move undo information for hash updates
     * @return Updated 64-bit Zobrist hash value
     */
    uint64_t update_zobrist_hash(uint64_t current_hash, const Move& move, const BitboardMoveUndoData& undo_data) const;
    
    /**
     * @brief Evaluates material balance between both sides
     * @param board The board position to evaluate
     * @return Material evaluation score in centipawns
     */
    int evaluate_material(const Board& board) const;
    
    /**
     * @brief Evaluates piece positioning using piece-square tables
     * @param board The board position to evaluate
     * @return Positional evaluation score based on piece placement
     */
    int evaluate_piece_square_tables(const Board& board) const;
    
    /**
     * @brief Evaluates pawn structure quality (isolated, doubled, passed, etc.)
     * @param board The board position to evaluate
     * @return Pawn structure evaluation score
     */
    int evaluate_pawn_structure(const Board& board);
    
    /**
     * @brief Evaluates king safety (pawn shield, open files, attacks)
     * @param board The board position to evaluate
     * @return King safety evaluation score
     */
    int evaluate_king_safety(const Board& board) const;
    
    /**
     * @brief Evaluates piece mobility and activity
     * @param board The board position to evaluate
     * @return Mobility evaluation score
     */
    int evaluate_mobility(const Board& board) const;
    
    /**
     * @brief Evaluates piece coordination and tactical patterns
     * @param board The board position to evaluate
     * @return Piece coordination evaluation score
     */
    int evaluate_piece_coordination(const Board& board) const;
    
    /**
     * @brief Evaluates endgame-specific factors
     * @param board The board position to evaluate
     * @return Endgame evaluation adjustments
     */
    int evaluate_endgame_factors(const Board& board) const;
    
    /**
     * @brief Evaluates development and tempo factors
     * @param board The board position to evaluate
     * @return Development evaluation score
     */
    int evaluate_development(const Board& board) const;
    
    /**
     * @brief Evaluates development for a specific color
     * @param board The board position to evaluate
     * @param color The color to evaluate development for
     * @return Development evaluation score for the specified color
     */
    int evaluate_development_for_color(const Board& board, Board::Color color) const;
    
    /**
     * @brief Evaluates penalties for pawns that limit piece development
     * @param board The board position to evaluate
     * @param color The color to evaluate for
     * @return Penalty score for development-limiting pawn moves
     */
    int evaluate_development_limiting_pawn_penalties(const Board& board, Board::Color color) const;
    
    /**
     * @brief Evaluates pawn shield protection around the king
     * @param board The board position to evaluate
     * @param color The color of the king to evaluate
     * @return Pawn shield evaluation score
     */
    int evaluate_pawn_shield(const Board& board, Board::Color color) const;
    
    /**
     * @brief Evaluates open files near the king for safety threats
     * @param board The board position to evaluate
     * @param color The color of the king to evaluate
     * @return Open files safety evaluation score
     */
    int evaluate_open_files_near_king(const Board& board, Board::Color color) const;
    
    /**
     * @brief Evaluates the inherent safety of the king's position
     * @param board The board position to evaluate
     * @param color The color of the king to evaluate
     * @return King position safety score
     */
    int evaluate_king_position_safety(const Board& board, Board::Color color) const;
    
    /**
     * @brief Evaluates pawn storms directed at the king
     * @param board The board position to evaluate
     * @param color The color of the king to evaluate
     * @return Pawn storm evaluation score
     */
    int evaluate_pawn_storms(const Board& board, Board::Color color) const;
    
    /**
     * @brief Evaluates piece cover and protection around the king
     * @param board The board position to evaluate
     * @param color The color of the king to evaluate
     * @return Piece cover evaluation score
     */
    int evaluate_piece_cover(const Board& board, Board::Color color) const;
    
    /**
     * @brief Evaluates attacking pieces in proximity to the king
     * @param board The board position to evaluate
     * @param color The color of the king to evaluate
     * @return Attacking pieces threat evaluation score
     */
    int evaluate_attacking_pieces_nearby(const Board& board, Board::Color color) const;
    
    /**
     * @brief Evaluates king mobility and escape square availability
     * @param board The board position to evaluate
     * @param color The color of the king to evaluate
     * @return King mobility evaluation score
     */
    int evaluate_king_mobility_and_escape(const Board& board, Board::Color color) const;
    
    /**
     * @brief Evaluates tactical threats directed at the king
     * @param board The board position to evaluate
     * @param color The color of the king to evaluate
     * @return Tactical threats evaluation score
     */
    int evaluate_tactical_threats_to_king(const Board& board, Board::Color color) const;
    
    /**
     * @brief Evaluates attack maps and pressure zones around the king
     * @param board The board position to evaluate
     * @param color The color of the king to evaluate
     * @return Attack pressure evaluation score
     */
    int evaluate_attack_maps_pressure_zones(const Board& board, Board::Color color) const;
    
    // III. Game Phase Adjustments
    
    // IV. Dynamic Considerations
    
    /**
     * @brief Interpolates between opening and endgame scores based on game phase
     * @param opening_score The opening evaluation score
     * @param endgame_score The endgame evaluation score
     * @param phase_value The current game phase value
     * @return Tapered evaluation score
     */
    int tapered_eval(int opening_score, int endgame_score, int phase_value) const;
    
    /**
     * @brief Determines the current game phase
     * @param board The board position to analyze
     * @return The current game phase (OPENING, MIDDLEGAME, or ENDGAME)
     */
    GamePhase get_game_phase(const Board& board) const;
    
    /**
     * @brief Calculates numerical phase value for tapered evaluation
     * @param board The board position to analyze
     * @return Phase value for interpolation calculations
     */
    int get_phase_value(const Board& board) const;
    
    /**
     * @brief Clears the pawn hash table cache
     */
    void clear_pawn_hash_table();
    
    /**
     * @brief Prints detailed evaluation breakdown for debugging
     * @param board The board position to analyze
     */
    void print_evaluation_breakdown(const Board& board);
    
private:
    // Piece-Square Tables (PST) - Separate tables for each game phase
    // PST[color][piece][phase][square] where phase: 0=opening, 1=middlegame, 2=endgame
    static constexpr int PST[6][3][64] = {
        // Pawns [opening, middlegame, endgame]
        {
            // Opening - Focus on development and center control
            {
                0,  0,  0,  0,  0,  0,  0,  0,
                50, 50, 50, 50, 50, 50, 50, 50,
                10, 10, 20, 35, 35, 20, 10, 10,
                5,  5, 15, 30, 30, 15,  5,  5,
                0,  0, 10, 25, 25, 10,  0,  0,
                5, -5, -5, 15, 15, -5, -5,  5,
                5, 10, 10,-10,-10, 10, 10,  5,
                0,  0,  0,  0,  0,  0,  0,  0
            },
            // Middlegame - Balanced approach
            {
                0,  0,  0,  0,  0,  0,  0,  0,
                50, 50, 50, 50, 50, 50, 50, 50,
                10, 10, 20, 35, 35, 20, 10, 10,
                5,  5, 15, 30, 30, 15,  5,  5,
                0,  0,  5, 25, 25,  5,  0,  0,
                5, -5, -5, 10, 10, -5, -5,  5,
                5, 10, 10,-15,-15, 10, 10,  5,
                0,  0,  0,  0,  0,  0,  0,  0
            },
            // Endgame - Passed pawns become crucial
            {
                0,  0,  0,  0,  0,  0,  0,  0,
                80, 80, 80, 80, 80, 80, 80, 80,
                50, 50, 50, 50, 50, 50, 50, 50,
                30, 30, 30, 30, 30, 30, 30, 30,
                15, 15, 15, 15, 15, 15, 15, 15,
                5,  5,  5,  5,  5,  5,  5,  5,
                0,  0,  0,  0,  0,  0,  0,  0,
                0,  0,  0,  0,  0,  0,  0,  0
            }
        },
        // Knights [opening, middlegame, endgame]
        {
            // Opening - Rapid development
            {
                -50,-40,-30,-30,-30,-30,-40,-50,
                -40,-20,  0,  5,  5,  0,-20,-40,
                -30,  5, 15, 20, 20, 15,  5,-30,
                -30, 10, 20, 25, 25, 20, 10,-30,
                -30,  5, 20, 25, 25, 20,  5,-30,
                -30,  5, 15, 20, 20, 15,  5,-30,
                -40,-20,  5, 10, 10,  5,-20,-40,
                -50,-40,-20,-20,-20,-20,-40,-50
            },
            // Middlegame - Central outposts
            {
                -50,-40,-30,-30,-30,-30,-40,-50,
                -40,-20,  0,  5,  5,  0,-20,-40,
                -30,  5, 15, 20, 20, 15,  5,-30,
                -30, 10, 20, 30, 30, 20, 10,-30,
                -30,  5, 20, 30, 30, 20,  5,-30,
                -30,  5, 15, 20, 20, 15,  5,-30,
                -40,-20,  5, 10, 10,  5,-20,-40,
                -50,-40,-20,-20,-20,-20,-40,-50
            },
            // Endgame - Less effective, avoid edges
            {
                -50,-40,-30,-30,-30,-30,-40,-50,
                -40,-20,  0,  0,  0,  0,-20,-40,
                -30,  0, 10, 15, 15, 10,  0,-30,
                -30,  5, 15, 20, 20, 15,  5,-30,
                -30,  5, 15, 20, 20, 15,  5,-30,
                -30,  0, 10, 15, 15, 10,  0,-30,
                -40,-20,  0,  0,  0,  0,-20,-40,
                -50,-40,-30,-30,-30,-30,-40,-50
            }
        },
        // Bishops [opening, middlegame, endgame]
        {
            // Opening - Quick development, control diagonals
            {
                -20,-10,-10,-10,-10,-10,-10,-20,
                -10,  5,  0,  0,  0,  0,  5,-10,
                -10, 10, 10, 15, 15, 10, 10,-10,
                -10,  5, 15, 20, 20, 15,  5,-10,
                -10, 10, 15, 20, 20, 15, 10,-10,
                -10, 15, 15, 15, 15, 15, 15,-10,
                -10, 10,  5,  5,  5,  5, 10,-10,
                -20,-10,-10,-10,-10,-10,-10,-20
            },
            // Middlegame - Long diagonals, outposts
            {
                -20,-10,-10,-10,-10,-10,-10,-20,
                -10,  5,  0,  0,  0,  0,  5,-10,
                -10, 10, 10, 15, 15, 10, 10,-10,
                -10,  5, 15, 25, 25, 15,  5,-10,
                -10, 10, 15, 25, 25, 15, 10,-10,
                -10, 15, 15, 15, 15, 15, 15,-10,
                -10, 10,  5,  5,  5,  5, 10,-10,
                -20,-10,-10,-10,-10,-10,-10,-20
            },
            // Endgame - Centralization important
            {
                -20,-10,-10,-10,-10,-10,-10,-20,
                -10,  0,  5,  5,  5,  5,  0,-10,
                -10,  5, 10, 15, 15, 10,  5,-10,
                -10,  5, 15, 20, 20, 15,  5,-10,
                -10,  5, 15, 20, 20, 15,  5,-10,
                -10,  5, 10, 15, 15, 10,  5,-10,
                -10,  0,  5,  5,  5,  5,  0,-10,
                -20,-10,-10,-10,-10,-10,-10,-20
            }
        },
        // Rooks [opening, middlegame, endgame]
        {
            // Opening - Avoid early development
            {
                -5, -5, -5, -5, -5, -5, -5, -5,
                0,  5,  5,  5,  5,  5,  5,  0,
                -5,  0,  0,  0,  0,  0,  0, -5,
                -5,  0,  0,  0,  0,  0,  0, -5,
                -5,  0,  0,  0,  0,  0,  0, -5,
                -5,  0,  0,  0,  0,  0,  0, -5,
                -5,  0,  0,  0,  0,  0,  0, -5,
                0,  0,  0,  5,  5,  0,  0,  0
            },
            // Middlegame - Open files, 7th rank
            {
                0,  0,  0,  0,  0,  0,  0,  0,
                5, 10, 10, 10, 10, 10, 10,  5,
                -5,  0,  0,  0,  0,  0,  0, -5,
                -5,  0,  0,  0,  0,  0,  0, -5,
                -5,  0,  0,  0,  0,  0,  0, -5,
                -5,  0,  0,  0,  0,  0,  0, -5,
                -5,  0,  0,  0,  0,  0,  0, -5,
                0,  0,  0,  5,  5,  0,  0,  0
            },
            // Endgame - Active, centralized
            {
                5,  5,  5,  5,  5,  5,  5,  5,
                10, 10, 10, 10, 10, 10, 10, 10,
                0,  5,  5,  5,  5,  5,  5,  0,
                0,  5,  5,  5,  5,  5,  5,  0,
                0,  5,  5,  5,  5,  5,  5,  0,
                0,  5,  5,  5,  5,  5,  5,  0,
                0,  5,  5,  5,  5,  5,  5,  0,
                5,  5,  5, 10, 10,  5,  5,  5
            }
        },
        // Queens [opening, middlegame, endgame]
        {
            // Opening - Avoid early development
            {
                -20,-10,-10, -5, -5,-10,-10,-20,
                -10,  0,  0,  0,  0,  0,  0,-10,
                -10,  0,  5,  5,  5,  5,  0,-10,
                -5,  0,  5,  5,  5,  5,  0, -5,
                0,  0,  5,  5,  5,  5,  0, -5,
                -10,  5,  5,  5,  5,  5,  0,-10,
                -10,  0,  5,  0,  0,  0,  0,-10,
                -20,-10,-10, -5, -5,-10,-10,-20
            },
            // Middlegame - Central control
            {
                -20,-10,-10, -5, -5,-10,-10,-20,
                -10,  0,  0,  0,  0,  0,  0,-10,
                -10,  0,  5,  5,  5,  5,  0,-10,
                -5,  0,  5, 10, 10,  5,  0, -5,
                0,  0,  5, 10, 10,  5,  0, -5,
                -10,  5,  5,  5,  5,  5,  0,-10,
                -10,  0,  5,  0,  0,  0,  0,-10,
                -20,-10,-10, -5, -5,-10,-10,-20
            },
            // Endgame - Active, centralized
            {
                -20,-10,-10, -5, -5,-10,-10,-20,
                -10,  0,  5,  5,  5,  5,  0,-10,
                -10,  5, 10, 10, 10, 10,  5,-10,
                -5,  5, 10, 15, 15, 10,  5, -5,
                -5,  5, 10, 15, 15, 10,  5, -5,
                -10,  5, 10, 10, 10, 10,  5,-10,
                -10,  0,  5,  5,  5,  5,  0,-10,
                -20,-10,-10, -5, -5,-10,-10,-20
            }
        },
        // Kings [opening, middlegame, endgame]
        {
            // Opening - Safety first, castle early
            {
                -30,-40,-40,-50,-50,-40,-40,-30,
                -30,-40,-40,-50,-50,-40,-40,-30,
                -30,-40,-40,-50,-50,-40,-40,-30,
                -30,-40,-40,-50,-50,-40,-40,-30,
                -20,-30,-30,-40,-40,-30,-30,-20,
                -10,-20,-20,-20,-20,-20,-20,-10,
                20, 20,  0,  0,  0,  0, 20, 20,
                20, 30, 10,  0,  0, 10, 30, 20
            },
            // Middlegame - Stay safe
            {
                -30,-40,-40,-50,-50,-40,-40,-30,
                -30,-40,-40,-50,-50,-40,-40,-30,
                -30,-40,-40,-50,-50,-40,-40,-30,
                -30,-40,-40,-50,-50,-40,-40,-30,
                -20,-30,-30,-40,-40,-30,-30,-20,
                -10,-20,-20,-20,-20,-20,-20,-10,
                20, 20,  0,  0,  0,  0, 20, 20,
                20, 30, 10,  0,  0, 10, 30, 20
            },
            // Endgame - Centralize and be active
            {
                -50,-40,-30,-20,-20,-30,-40,-50,
                -30,-20,-10,  0,  0,-10,-20,-30,
                -30,-10, 20, 30, 30, 20,-10,-30,
                -30,-10, 30, 40, 40, 30,-10,-30,
                -30,-10, 30, 40, 40, 30,-10,-30,
                -30,-10, 20, 30, 30, 20,-10,-30,
                -30,-30,  0,  0,  0,  0,-30,-30,
                -50,-30,-30,-30,-30,-30,-30,-50
            }
        }
    };
    

    
    // Material values array for quick lookup
    static constexpr int MATERIAL_VALUES[6] = {
        EvalConstants::PAWN_VALUE,
        EvalConstants::KNIGHT_VALUE,
        EvalConstants::BISHOP_VALUE,
        EvalConstants::ROOK_VALUE,
        EvalConstants::QUEEN_VALUE,
        EvalConstants::KING_VALUE
    };
    
    // Phase values for pieces
    static constexpr int PHASE_VALUES[6] = {
        0, // Pawn
        EvalConstants::KNIGHT_PHASE,
        EvalConstants::BISHOP_PHASE,
        EvalConstants::ROOK_PHASE,
        EvalConstants::QUEEN_PHASE,
        0  // King
    };
    
    // Member variables
    IncrementalEvalData incremental_data;
    ZobristKeys zobrist_keys;
    std::unordered_map<uint64_t, PawnHashEntry> pawn_hash_table;
    
    // Precomputed masks for efficient pawn evaluation
    Bitboard passed_pawn_masks[64][2]; // [square][color]
    static Bitboard isolated_pawn_masks[64];   // [square] - adjacent files
    static Bitboard file_masks[8];             // [file] - entire file
    
    /**
     * @brief Gets piece-square table value for a specific piece and position
     * @param piece_type The type of piece
     * @param color The color of the piece
     * @param square The square position
     * @param phase The current game phase
     * @return Piece-square table value
     */
    int get_piece_square_value(Board::PieceType piece_type, Board::Color color, int square, GamePhase phase) const;
    
    /**
     * @brief Evaluates pawn structure for a specific color
     * @param board The board position to evaluate
     * @param color The color to evaluate
     * @return Pawn structure evaluation score for the color
     */
    int evaluate_pawn_structure_for_color(const Board& board, Board::Color color) const;
    
    /**
     * @brief Evaluates king safety for a specific color
     * @param board The board position to evaluate
     * @param color The color to evaluate
     * @return King safety evaluation score for the color
     */
    int evaluate_king_safety_for_color(const Board& board, Board::Color color) const;
    
    /**
     * @brief Evaluates piece mobility for a specific color
     * @param board The board position to evaluate
     * @param color The color to evaluate
     * @return Mobility evaluation score for the color
     */
    int evaluate_mobility_for_color(const Board& board, Board::Color color) const;
    
    /**
     * @brief Evaluates piece coordination for a specific color
     * @param board The board position to evaluate
     * @param color The color to evaluate
     * @return Piece coordination evaluation score for the color
     */
    int evaluate_piece_coordination_for_color(const Board& board, Board::Color color) const;
    
    /**
     * @brief Evaluates endgame factors for a specific color
     * @param board The board position to evaluate
     * @param color The color to evaluate
     * @return Endgame factors evaluation score for the color
     */
    int evaluate_endgame_factors_for_color(const Board& board, Board::Color color) const;
    
    /**
     * @brief Counts attacking pieces in the king zone
     * @param board The board position to analyze
     * @param attacking_color The color of attacking pieces
     * @param king_color The color of the king being attacked
     * @return Number of attacking pieces in king zone
     */
    int count_attackers_to_king_zone(const Board& board, Board::Color attacking_color, Board::Color king_color) const;
    
    /**
     * @brief Checks if a file is completely open (no pawns)
     * @param board The board position to check
     * @param file The file to check (0-7)
     * @return True if the file is open
     */
    bool is_file_open(const Board& board, int file) const;
    
    /**
     * @brief Checks if a file is semi-open for a color (no pawns of that color)
     * @param board The board position to check
     * @param file The file to check (0-7)
     * @param color The color to check for
     * @return True if the file is semi-open for the color
     */
    bool is_file_semi_open(const Board& board, int file, Board::Color color) const;
    
    /**
     * @brief Converts rank and file to square index
     * @param rank The rank (0-7)
     * @param file The file (0-7)
     * @return Square index (0-63)
     */
    static int square_to_index(int rank, int file) { return rank * 8 + file; }
    
    /**
     * @brief Mirrors a square vertically (flips rank)
     * @param square The square to mirror
     * @return Mirrored square index
     */
    static int mirror_square(int square) { return square ^ 56; }
    
    /**
     * @brief Checks if a pawn is a passed pawn
     * @param board The board position to check
     * @param square The pawn's square
     * @param color The pawn's color
     * @return True if the pawn is passed
     */
    bool is_passed_pawn(const Board& board, int square, Board::Color color) const;
    
    /**
     * @brief Checks if a pawn is isolated (no friendly pawns on adjacent files)
     * @param board The board position to check
     * @param square The pawn's square
     * @param color The pawn's color
     * @return True if the pawn is isolated
     */
    static bool is_isolated_pawn(const Board& board, int square, Board::Color color);
    
    /**
     * @brief Checks if a pawn is doubled (another friendly pawn on same file)
     * @param board The board position to check
     * @param square The pawn's square
     * @param color The pawn's color
     * @return True if the pawn is doubled
     */
    static bool is_doubled_pawn(const Board& board, int square, Board::Color color);
    
    /**
     * @brief Checks if a pawn is backward (cannot advance safely)
     * @param board The board position to check
     * @param square The pawn's square
     * @param color The pawn's color
     * @return True if the pawn is backward
     */
    static bool is_backward_pawn(const Board& board, int square, Board::Color color);
    
    /**
     * @brief Checks if a pawn is part of a pawn chain
     * @param board The board position to check
     * @param square The pawn's square
     * @param color The pawn's color
     * @return True if the pawn is in a chain
     */
    static bool is_pawn_chain(const Board& board, int square, Board::Color color);
    
    /**
     * @brief Gets bonus score for passed pawn based on rank
     * @param square The pawn's square
     * @param color The pawn's color
     * @return Rank-based bonus score
     */
    static int get_passed_pawn_rank_bonus(int square, Board::Color color);
    
    /**
     * @brief Calculates Manhattan distance between two squares
     * @param sq1 First square
     * @param sq2 Second square
     * @return Manhattan distance
     */
    static int distance_between_squares(int sq1, int sq2);
    
    /**
     * @brief Checks if a square is in the king's safety zone
     * @param square The square to check
     * @param king_square The king's square
     * @return True if square is in king zone
     */
    static bool is_in_king_zone(int square, int king_square);
    
    /**
     * @brief Evaluates pin threats and tactical vulnerabilities
     * @param board The board position to evaluate
     * @param color The color to evaluate pins for
     * @return Pin evaluation score
     */
    int evaluate_pins(const Board& board, Board::Color color) const;

    /**
     * @brief Initializes passed pawn masks for efficient evaluation
     */
    void init_passed_pawn_masks();
    
    /**
     * @brief Initializes pawn-related bitboard masks
     */
    void init_pawn_masks();
    
    /**
     * @brief Returns evaluation sign multiplier for a color
     * @param color The color to get sign for
     * @return 1 for White, -1 for Black
     */
    static int side_sign(Board::Color color) { return color == Board::WHITE ? 1 : -1; }
};

#endif // EVALUATION_H