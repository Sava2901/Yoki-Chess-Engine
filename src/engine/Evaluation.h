#ifndef EVALUATION_H
#define EVALUATION_H

#include "../board/Board.h"
#include "../board/Move.h"
#include <cstdint>
#include <unordered_map>

// Forward declarations
struct BitboardMoveUndoData;

// Game phase constants
enum GamePhase {
    OPENING = 0,
    MIDDLEGAME = 1,
    ENDGAME = 2
};

// Evaluation constants
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
    
    // King safety - enhanced
    constexpr int KING_SAFETY_BONUS = 15;
    constexpr int OPEN_FILE_NEAR_KING_PENALTY = -25;
    constexpr int SEMI_OPEN_FILE_NEAR_KING_PENALTY = -12;
    constexpr int KING_ATTACK_WEIGHT = 20;
    constexpr int KING_ZONE_ATTACK_BONUS = 8;
    
    // Mobility - refined
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
    
    // Endgame specific
    constexpr int KING_ACTIVITY_BONUS = 5;
    constexpr int OPPOSITION_BONUS = 20;
    constexpr int CENTRALIZATION_BONUS = 10;
    constexpr int KING_NEAR_ENEMY_PAWNS_BONUS = 6;
    constexpr int CONNECTED_PASSED_PAWNS_BONUS = 20;
}

// Incremental evaluation data
struct IncrementalEvalData {
    int material_balance;
    int positional_balance;
    int pawn_structure_score;
    int king_safety_score;
    int mobility_score;
    GamePhase game_phase;
    int phase_value;
    
    IncrementalEvalData() : material_balance(0), positional_balance(0), 
                           pawn_structure_score(0), king_safety_score(0),
                           mobility_score(0), game_phase(OPENING), phase_value(0) {}
};

// Zobrist hashing structure
struct ZobristKeys {
    uint64_t piece_keys[2][6][64];  // [color][piece_type][square]
    uint64_t castling_keys[16];     // For all castling combinations
    uint64_t en_passant_keys[8];    // For en passant files
    uint64_t side_to_move_key;      // For side to move
    
    ZobristKeys();
    void initialize();
};

// Pawn hash table entry
struct PawnHashEntry {
    uint64_t key;
    int score;
    uint8_t passed_pawns_white;
    uint8_t passed_pawns_black;
    uint8_t isolated_pawns_white;
    uint8_t isolated_pawns_black;
    
    PawnHashEntry() : key(0), score(0), passed_pawns_white(0), 
                     passed_pawns_black(0), isolated_pawns_white(0), 
                     isolated_pawns_black(0) {}
};

class Evaluation {
public:
    Evaluation();
    ~Evaluation() = default;
    
    // Main evaluation functions
    int evaluate(const Board& board);
    int evaluate_incremental(const Board& board, const Move& move, const BitboardMoveUndoData& undo_data);
    
    // Incremental evaluation management
    void initialize_incremental_eval(const Board& board);
    void update_incremental_eval(const Board& board, const Move& move, const BitboardMoveUndoData& undo_data);
    void undo_incremental_eval(const Board& board, const Move& move, const BitboardMoveUndoData& undo_data);
    
    // Zobrist hashing
    uint64_t compute_zobrist_hash(const Board& board) const;
    uint64_t update_zobrist_hash(uint64_t current_hash, const Move& move, const BitboardMoveUndoData& undo_data) const;
    
    // Component evaluations
    int evaluate_material(const Board& board) const;
    int evaluate_piece_square_tables(const Board& board) const;
    int evaluate_pawn_structure(const Board& board);
    int evaluate_king_safety(const Board& board) const;
    int evaluate_mobility(const Board& board) const;
    int evaluate_piece_coordination(const Board& board) const;
    int evaluate_endgame_factors(const Board& board) const;
    int evaluate_development(const Board& board) const;
    int evaluate_development_for_color(const Board& board, Board::Color color) const;
    
    // Tapered evaluation (interpolates between opening and endgame)
    int tapered_eval(int opening_score, int endgame_score, int phase_value) const;
    
    // Game phase detection
    GamePhase get_game_phase(const Board& board) const;
    int get_phase_value(const Board& board) const;
    
    // Utility functions
    void clear_pawn_hash_table();
    void print_evaluation_breakdown(const Board& board);
    
private:
    // Piece-Square Tables (PST) - 1D arrays for cache efficiency
    static constexpr int PST[2][6][64] = {
        // White pieces
        {
            // Pawns
            {
                 0,  0,  0,  0,  0,  0,  0,  0,
                50, 50, 50, 50, 50, 50, 50, 50,
                10, 10, 20, 30, 30, 20, 10, 10,
                 5,  5, 10, 25, 25, 10,  5,  5,
                 0,  0,  0, 20, 20,  0,  0,  0,
                 5, -5,-10,  0,  0,-10, -5,  5,
                 5, 10, 10,-20,-20, 10, 10,  5,
                 0,  0,  0,  0,  0,  0,  0,  0
            },
            // Knights
            {
                -50,-40,-30,-30,-30,-30,-40,-50,
                -40,-20,  0,  0,  0,  0,-20,-40,
                -30,  0, 10, 15, 15, 10,  0,-30,
                -30,  5, 15, 20, 20, 15,  5,-30,
                -30,  0, 15, 20, 20, 15,  0,-30,
                -30,  5, 10, 15, 15, 10,  5,-30,
                -40,-20,  0,  5,  5,  0,-20,-40,
                -50,-40,-30,-30,-30,-30,-40,-50
            },
            // Bishops
            {
                -20,-10,-10,-10,-10,-10,-10,-20,
                -10,  0,  0,  0,  0,  0,  0,-10,
                -10,  0,  5, 10, 10,  5,  0,-10,
                -10,  5,  5, 10, 10,  5,  5,-10,
                -10,  0, 10, 10, 10, 10,  0,-10,
                -10, 10, 10, 10, 10, 10, 10,-10,
                -10,  5,  0,  0,  0,  0,  5,-10,
                -20,-10,-10,-10,-10,-10,-10,-20
            },
            // Rooks
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
            // Queens
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
            // Kings (middlegame)
            {
                -30,-40,-40,-50,-50,-40,-40,-30,
                -30,-40,-40,-50,-50,-40,-40,-30,
                -30,-40,-40,-50,-50,-40,-40,-30,
                -30,-40,-40,-50,-50,-40,-40,-30,
                -20,-30,-30,-40,-40,-30,-30,-20,
                -10,-20,-20,-20,-20,-20,-20,-10,
                 20, 20,  0,  0,  0,  0, 20, 20,
                 20, 30, 10,  0,  0, 10, 30, 20
            }
        },
        // Black pieces (mirrored)
        {
            // Pawns
            {
                 0,  0,  0,  0,  0,  0,  0,  0,
                 5, 10, 10,-20,-20, 10, 10,  5,
                 5, -5,-10,  0,  0,-10, -5,  5,
                 0,  0,  0, 20, 20,  0,  0,  0,
                 5,  5, 10, 25, 25, 10,  5,  5,
                10, 10, 20, 30, 30, 20, 10, 10,
                50, 50, 50, 50, 50, 50, 50, 50,
                 0,  0,  0,  0,  0,  0,  0,  0
            },
            // Knights
            {
                -50,-40,-30,-30,-30,-30,-40,-50,
                -40,-20,  0,  5,  5,  0,-20,-40,
                -30,  5, 10, 15, 15, 10,  5,-30,
                -30,  0, 15, 20, 20, 15,  0,-30,
                -30,  5, 15, 20, 20, 15,  5,-30,
                -30,  0, 10, 15, 15, 10,  0,-30,
                -40,-20,  0,  0,  0,  0,-20,-40,
                -50,-40,-30,-30,-30,-30,-40,-50
            },
            // Bishops
            {
                -20,-10,-10,-10,-10,-10,-10,-20,
                -10,  5,  0,  0,  0,  0,  5,-10,
                -10, 10, 10, 10, 10, 10, 10,-10,
                -10,  0, 10, 10, 10, 10,  0,-10,
                -10,  5,  5, 10, 10,  5,  5,-10,
                -10,  0,  5, 10, 10,  5,  0,-10,
                -10,  0,  0,  0,  0,  0,  0,-10,
                -20,-10,-10,-10,-10,-10,-10,-20
            },
            // Rooks
            {
                 0,  0,  0,  5,  5,  0,  0,  0,
                -5,  0,  0,  0,  0,  0,  0, -5,
                -5,  0,  0,  0,  0,  0,  0, -5,
                -5,  0,  0,  0,  0,  0,  0, -5,
                -5,  0,  0,  0,  0,  0,  0, -5,
                -5,  0,  0,  0,  0,  0,  0, -5,
                 5, 10, 10, 10, 10, 10, 10,  5,
                 0,  0,  0,  0,  0,  0,  0,  0
            },
            // Queens
            {
                -20,-10,-10, -5, -5,-10,-10,-20,
                -10,  0,  5,  0,  0,  0,  0,-10,
                -10,  5,  5,  5,  5,  5,  0,-10,
                  0,  0,  5,  5,  5,  5,  0, -5,
                 -5,  0,  5,  5,  5,  5,  0, -5,
                -10,  0,  5,  5,  5,  5,  0,-10,
                -10,  0,  0,  0,  0,  0,  0,-10,
                -20,-10,-10, -5, -5,-10,-10,-20
            },
            // Kings (middlegame)
            {
                 20, 30, 10,  0,  0, 10, 30, 20,
                 20, 20,  0,  0,  0,  0, 20, 20,
                -10,-20,-20,-20,-20,-20,-20,-10,
                -20,-30,-30,-40,-40,-30,-30,-20,
                -30,-40,-40,-50,-50,-40,-40,-30,
                -30,-40,-40,-50,-50,-40,-40,-30,
                -30,-40,-40,-50,-50,-40,-40,-30,
                -30,-40,-40,-50,-50,-40,-40,-30
            }
        }
    };
    
    // Endgame king piece-square table
    static constexpr int KING_ENDGAME_PST[64] = {
        -50,-40,-30,-20,-20,-30,-40,-50,
        -30,-20,-10,  0,  0,-10,-20,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-30,  0,  0,  0,  0,-30,-30,
        -50,-30,-30,-30,-30,-30,-30,-50
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
    
    // Helper functions
    int get_piece_square_value(Board::PieceType piece_type, Board::Color color, int square, GamePhase phase) const;
    int evaluate_pawn_structure_for_color(const Board& board, Board::Color color) const;
    int evaluate_king_safety_for_color(const Board& board, Board::Color color) const;
    int evaluate_mobility_for_color(const Board& board, Board::Color color) const;
    int evaluate_piece_coordination_for_color(const Board& board, Board::Color color) const;
    int evaluate_endgame_factors_for_color(const Board& board, Board::Color color) const;
    int count_attackers_to_king_zone(const Board& board, Board::Color attacking_color, Board::Color king_color) const;
    bool is_file_open(const Board& board, int file) const;
    bool is_file_semi_open(const Board& board, int file, Board::Color color) const;
    
    // Utility functions
    static int square_to_index(int rank, int file) { return rank * 8 + file; }
    static int mirror_square(int square) { return square ^ 56; } // Flip rank
    bool is_passed_pawn(const Board& board, int square, Board::Color color) const;
    static bool is_isolated_pawn(const Board& board, int square, Board::Color color);
    static bool is_doubled_pawn(const Board& board, int square, Board::Color color);
    static bool is_backward_pawn(const Board& board, int square, Board::Color color);
    static bool is_pawn_chain(const Board& board, int square, Board::Color color);
    static int get_passed_pawn_rank_bonus(int square, Board::Color color);
    static int distance_between_squares(int sq1, int sq2);
    static bool is_in_king_zone(int square, int king_square);
    
    // Initialization functions
    void init_passed_pawn_masks();
    void init_pawn_masks();
    
    // Branch-free evaluation helpers
    static int side_sign(Board::Color color) { return color == Board::WHITE ? 1 : -1; }
};

#endif // EVALUATION_H