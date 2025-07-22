#include "Evaluation.h"
#include "../board/MoveGenerator.h"
#include <random>
#include <iostream>
#include <iomanip>
#include <algorithm>

// TODO: Add relative piece values for different positions and future prospects
// TODO: Check if there is a need for precomputed bitboard masks: Isolated Pawn Detection, Backward Pawns, Outposts,
// King Shield / Pawn Storm, Rook Open/Semi-Open Files, Pawn Spans and Front Spans, Pawn Spans and Front Spans, Center Control

// Static member definitions
Bitboard Evaluation::isolated_pawn_masks[64];
Bitboard Evaluation::file_masks[8];
static bool pawn_masks_initialized = false;

// Branchless abs for 32-bit int 
static int abs_int(int x) { 
    int mask = x >> 31;         // all 1s if x < 0, else all 0s 
    return (x + mask) ^ mask;   // flip bits and add 1 if negative 
} 

// Branchless max for 32-bit int 
static int max_int(int a, int b) { 
    int diff = a - b; 
    int mask = diff >> 31;      // all 1s if diff < 0, else 0 
    return a - (diff & mask);   // if a < b, subtract diff else subtract 0 
}

// Branchless min for 32-bit int 
static int min_int(int a, int b) { 
    int diff = a - b; 
    int mask = diff >> 31;      // all 1s if diff < 0, else 0 
    return b + (diff & mask);   // if a < b, add diff else add 0 
}

// Helper function to get file mask
static Bitboard get_file_mask(int file) {
    static const Bitboard file_masks[8] = {
        FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H
    };
    return (file >= 0 && file < 8) ? file_masks[file] : 0ULL;
}

// Zobrist key initialization
ZobristKeys::ZobristKeys() {
    initialize();
}

void ZobristKeys::initialize() {
    std::mt19937_64 rng(0x1234567890ABCDEF); // Fixed seed for reproducibility
    std::uniform_int_distribution<uint64_t> dist;
    
    // Initialize piece keys
    for (int color = 0; color < 2; ++color) {
        for (int piece = 0; piece < 6; ++piece) {
            for (int square = 0; square < 64; ++square) {
                piece_keys[color][piece][square] = dist(rng);
            }
        }
    }
    
    // Initialize castling keys
    for (int i = 0; i < 16; ++i) {
        castling_keys[i] = dist(rng);
    }
    
    // Initialize en passant keys
    for (int i = 0; i < 8; ++i) {
        en_passant_keys[i] = dist(rng);
    }
    
    // Initialize side to move key
    side_to_move_key = dist(rng);
}

// Constructor
Evaluation::Evaluation() {
    pawn_hash_table.reserve(65536); // Reserve space for pawn hash table
    init_pawn_masks(); // Initialize other pawn masks
}

// TODO: Tune constants and piece-square tables for better representation of piece values and positions
// Main evaluation function
int Evaluation::evaluate(const Board& board) {
    int score = 0;
    
    // Get game phase
    GamePhase phase = get_game_phase(board);
    
    // Material evaluation
    int material_score = evaluate_material(board);
    
    // Positional evaluation (piece-square tables)
    int positional_score = evaluate_piece_square_tables(board);
    // TODO: Check if there is a need for improvements regarding weak color complex, weak pawns
    
    // Pawn structure evaluation
    int pawn_score = evaluate_pawn_structure(board);
    
    // King safety evaluation
    int king_safety_score = evaluate_king_safety(board);
    
    // Mobility evaluation
    int mobility_score = evaluate_mobility(board);
    
    // Piece coordination evaluation
    int coordination_score = evaluate_piece_coordination(board);
    
    // Development evaluation (mainly for opening)
    int development_score = (phase == OPENING) ? evaluate_development(board) : 0;
    
    // Endgame factors
    int endgame_score = (phase == ENDGAME) ? evaluate_endgame_factors(board) : 0;
    
    // Combine all scores
    score = material_score + positional_score + pawn_score + king_safety_score +
            mobility_score + coordination_score + development_score + endgame_score;
    
    // Add tempo bonus for side to move (from white's perspective)
    if (board.get_active_color() == Board::WHITE) {
        score += EvalConstants::TEMPO_BONUS;
    } else {
        score -= EvalConstants::TEMPO_BONUS;
    }

    return score;
}

// Incremental evaluation
int Evaluation::evaluate_incremental(const Board& board, const Move& move, const BitboardMoveUndoData& undo_data) {
    // Update incremental data based on the move
    update_incremental_eval(board, move, undo_data);
    
    // Recalculate components that were marked for recalculation (set to 0)
    if (incremental_data.pawn_structure_score == 0) {
        incremental_data.pawn_structure_score = evaluate_pawn_structure(board);
    }
    
    if (incremental_data.king_safety_score == 0) {
        incremental_data.king_safety_score = evaluate_king_safety(board);
    }
    
    if (incremental_data.mobility_score == 0) {
        incremental_data.mobility_score = evaluate_mobility(board);
    }
    
    // Calculate missing components that aren't tracked incrementally
    GamePhase phase = incremental_data.game_phase;
    
    // Piece coordination evaluation
    int coordination_score = evaluate_piece_coordination(board);
    
    // Development evaluation (mainly for opening)
    int development_score = (phase == OPENING) ? evaluate_development(board) : 0;
    
    // Endgame factors
    int endgame_score = (phase == ENDGAME) ? evaluate_endgame_factors(board) : 0;
    
    // Return the updated evaluation
    int total_score = incremental_data.material_balance + 
                     incremental_data.positional_balance + 
                     incremental_data.pawn_structure_score + 
                     incremental_data.king_safety_score + 
                     incremental_data.mobility_score +
                     coordination_score + development_score + endgame_score;
    
    // Add tempo bonus for side to move (from white's perspective)
    if (board.get_active_color() == Board::WHITE) {
        total_score += EvalConstants::TEMPO_BONUS;
    } else {
        total_score -= EvalConstants::TEMPO_BONUS;
    }
    
    return total_score;
}

// Initialize incremental evaluation
void Evaluation::initialize_incremental_eval(const Board& board) {
    incremental_data.material_balance = evaluate_material(board);
    incremental_data.positional_balance = evaluate_piece_square_tables(board);
    incremental_data.pawn_structure_score = evaluate_pawn_structure(board);
    incremental_data.king_safety_score = evaluate_king_safety(board);
    incremental_data.mobility_score = evaluate_mobility(board);
    incremental_data.game_phase = get_game_phase(board);
    incremental_data.phase_value = get_phase_value(board);
}

// Update incremental evaluation
void Evaluation::update_incremental_eval(const Board& board, const Move& move, const BitboardMoveUndoData& undo_data) {
    Board::PieceType piece_type = Board::char_to_piece_type(move.piece);
    Board::Color piece_color = Board::char_to_color(move.piece);
    int side_multiplier = side_sign(piece_color);
    
    int from_square = square_to_index(move.from_rank, move.from_file);
    int to_square = square_to_index(move.to_rank, move.to_file);
    
    // Update material balance if there was a capture
    if (move.captured_piece != '.') {
        Board::PieceType captured_type = Board::char_to_piece_type(move.captured_piece);
        incremental_data.material_balance -= side_multiplier * MATERIAL_VALUES[captured_type];
        incremental_data.phase_value -= PHASE_VALUES[captured_type];
    }
    
    // Update positional balance (piece-square tables)
    int old_pst_value = get_piece_square_value(piece_type, piece_color, from_square, incremental_data.game_phase);
    int new_pst_value = get_piece_square_value(piece_type, piece_color, to_square, incremental_data.game_phase);
    incremental_data.positional_balance += side_multiplier * (new_pst_value - old_pst_value);
    
    // Handle promotion
    if (move.promotion_piece != '.') {
        Board::PieceType promotion_type = Board::char_to_piece_type(move.promotion_piece);
        // Remove pawn value, add promoted piece value
        incremental_data.material_balance += side_multiplier * (MATERIAL_VALUES[promotion_type] - MATERIAL_VALUES[Board::PAWN]);
        incremental_data.phase_value += PHASE_VALUES[promotion_type];
        
        // Update PST for promotion
        int pawn_pst = get_piece_square_value(Board::PAWN, piece_color, to_square, incremental_data.game_phase);
        int promotion_pst = get_piece_square_value(promotion_type, piece_color, to_square, incremental_data.game_phase);
        incremental_data.positional_balance += side_multiplier * (promotion_pst - pawn_pst - new_pst_value);
    }
    
    // Update game phase
    incremental_data.game_phase = (incremental_data.phase_value > EvalConstants::TOTAL_PHASE * 2 / 3) ? OPENING :
                                 (incremental_data.phase_value > EvalConstants::TOTAL_PHASE / 3) ? MIDDLEGAME : ENDGAME;

    // Update pawn structure score incrementally
    // For pawn moves, captures involving pawns, or promotions, we need to recalculate affected areas
    if (piece_type == Board::PAWN || move.captured_piece == 'P' || move.captured_piece == 'p' || move.promotion_piece != '.') {
        // Calculate old pawn structure scores
        int old_white_pawn_score = 0;
        int old_black_pawn_score = 0;
        
        // Store current scores before move effects
        Board temp_board;
        // We need to reconstruct the board state before the move to get accurate deltas
        // For now, recalculate the affected color's pawn structure
        
        // Recalculate pawn structure for both colors when pawns are involved
        // This is more accurate than trying to track all pawn interactions
        incremental_data.pawn_structure_score = 0; // Will be recalculated in next evaluation
    }
    
    // Update king safety score incrementally
    // King safety changes when:
    // 1. King moves
    // 2. Pieces move near the king
    // 3. Castling occurs
    // 4. Pieces that can attack the king area are moved/captured
    int white_king_pos = board.get_king_position(Board::WHITE);
    int black_king_pos = board.get_king_position(Board::BLACK);
    if (piece_type == Board::KING || 
        move.captured_piece != '.' ||
        (piece_type == Board::ROOK && (from_square == 0 || from_square == 7 || from_square == 56 || from_square == 63)) ||
        abs_int(to_square - white_king_pos) <= 16 || abs_int(to_square - black_king_pos) <= 16 ||
        abs_int(from_square - white_king_pos) <= 16 || abs_int(from_square - black_king_pos) <= 16) {
        // Recalculate king safety when kings move or pieces move near kings
        incremental_data.king_safety_score = 0; // Will be recalculated in next evaluation
    }
    
    // Update mobility score incrementally
    // Mobility changes when:
    // 1. Any piece moves (changes its own mobility)
    // 2. Pieces are captured (affects mobility of other pieces)
    // 3. Pieces block/unblock other pieces' mobility
    if (move.captured_piece != '.' || 
        piece_type == Board::KNIGHT || piece_type == Board::BISHOP || 
        piece_type == Board::ROOK || piece_type == Board::QUEEN) {
        // For pieces that significantly affect mobility, recalculate
        // This is more efficient than trying to track all mobility interactions
        incremental_data.mobility_score = 0; // Will be recalculated in next evaluation
    }
    
    // Handle special moves that affect multiple evaluation components
    // Castling
    if (piece_type == Board::KING && abs_int(to_square - from_square) == 2) {
        // Castling affects king safety and potentially mobility
        incremental_data.king_safety_score = 0;
        incremental_data.mobility_score = 0;
    }
    
    // En passant capture
    if (piece_type == Board::PAWN && move.captured_piece == '.' && 
        abs_int(to_square - from_square) != 8 && abs_int(to_square - from_square) != 16) {
        // En passant affects pawn structure
        incremental_data.pawn_structure_score = 0;
    }
}

/* 
Material update:
On captures and promotions, update material balance and phase values by adding/subtracting only the changed pieces.
This is already done well in your code.

Piece-Square Table (PST) update:
Add the new PST value of the piece on its destination square.
Subtract the old PST value of the piece on its source square.
Also adjust for promotion PST changes.
This part looks good.

Pawn structure update:
Pawn structure is tricky because pawn moves and captures can affect many interrelated pawns.
Instead of resetting to 0 unconditionally, you want to:
Remove the contribution of the pawn on the old square (if a pawn moved or was captured).
Add the contribution of the pawn on the new square.
Update connected passed pawns, isolated pawns, doubled pawns, etc., incrementally.
To do this well, you typically keep bitboards or counters tracking pawn structure features.
If you cannot track incremental updates precisely, then a full recalculation might be required when pawn structure changes.

King safety update:
Only recalculate king safety if the move affects king position or nearby squares, or captures/moves pieces that influence king safety.
Ideally, update king safety incrementally:
Remove old attackers/defenders.
Add new attackers/defenders.
Resetting to zero and recalculating fully is correct but less efficient.

Mobility update:
Mobility depends on pieces' possible moves.
When a piece moves or is captured, the mobility of nearby pieces changes.
Ideally, update mobility incrementally:
Remove old mobility contributions for moved/captured pieces.
Add mobility contributions for moved pieces on their new squares.
Resetting and recalculating mobility is simpler but slower.

Special moves:
Castling affects king safety and rook mobility.
En passant affects pawn structure.
These should trigger updates to those evaluation components, as your code shows.
*/

// Undo incremental evaluation
void Evaluation::undo_incremental_eval(const Board& board, const Move& move, const BitboardMoveUndoData& undo_data) {
    Board::PieceType piece_type = Board::char_to_piece_type(move.piece);
    Board::Color piece_color = Board::char_to_color(move.piece);
    int side_multiplier = side_sign(piece_color);
    
    int from_square = square_to_index(move.from_rank, move.from_file);
    int to_square = square_to_index(move.to_rank, move.to_file);
    
    // Undo material balance changes
    if (move.captured_piece != '.') {
        Board::PieceType captured_type = Board::char_to_piece_type(move.captured_piece);
        incremental_data.material_balance += side_multiplier * MATERIAL_VALUES[captured_type];
        incremental_data.phase_value += PHASE_VALUES[captured_type];
    }
    
    // Undo positional balance changes
    int old_pst_value = get_piece_square_value(piece_type, piece_color, from_square, incremental_data.game_phase);
    int new_pst_value = get_piece_square_value(piece_type, piece_color, to_square, incremental_data.game_phase);
    incremental_data.positional_balance -= side_multiplier * (new_pst_value - old_pst_value);
    
    // Undo promotion
    if (move.promotion_piece != '.') {
        Board::PieceType promotion_type = Board::char_to_piece_type(move.promotion_piece);
        incremental_data.material_balance -= side_multiplier * (MATERIAL_VALUES[promotion_type] - MATERIAL_VALUES[Board::PAWN]);
        incremental_data.phase_value -= PHASE_VALUES[promotion_type];
        
        int pawn_pst = get_piece_square_value(Board::PAWN, piece_color, to_square, incremental_data.game_phase);
        int promotion_pst = get_piece_square_value(promotion_type, piece_color, to_square, incremental_data.game_phase);
        incremental_data.positional_balance -= side_multiplier * (promotion_pst - pawn_pst - new_pst_value);
    }
    
    // Update game phase
    incremental_data.game_phase = (incremental_data.phase_value > EvalConstants::TOTAL_PHASE * 2 / 3) ? OPENING :
                                 (incremental_data.phase_value > EvalConstants::TOTAL_PHASE / 3) ? MIDDLEGAME : ENDGAME;
    
    // Mark components for recalculation when undoing moves that affect them
    // Undo pawn structure score incrementally
    if (piece_type == Board::PAWN || move.captured_piece == 'P' || move.captured_piece == 'p' || move.promotion_piece != '.') {
        incremental_data.pawn_structure_score = 0; // Will be recalculated in next evaluation
    }
    
    // Undo king safety score incrementally
    int white_king_pos = board.get_king_position(Board::WHITE);
    int black_king_pos = board.get_king_position(Board::BLACK);
    if (piece_type == Board::KING || 
        move.captured_piece != '.' ||
        (piece_type == Board::ROOK && (from_square == 0 || from_square == 7 || from_square == 56 || from_square == 63)) ||
        abs_int(to_square - white_king_pos) <= 16 || abs_int(to_square - black_king_pos) <= 16 ||
        abs_int(from_square - white_king_pos) <= 16 || abs_int(from_square - black_king_pos) <= 16) {
        incremental_data.king_safety_score = 0; // Will be recalculated in next evaluation
    }
    
    // Undo mobility score incrementally
    if (move.captured_piece != '.' || 
        piece_type == Board::KNIGHT || piece_type == Board::BISHOP || 
        piece_type == Board::ROOK || piece_type == Board::QUEEN) {
        incremental_data.mobility_score = 0; // Will be recalculated in next evaluation
    }
    
    // Handle special moves that affect multiple evaluation components
    // Castling
    if (piece_type == Board::KING && abs_int(to_square - from_square) == 2) {
        incremental_data.king_safety_score = 0;
        incremental_data.mobility_score = 0;
    }
    
    // En passant capture
    if (piece_type == Board::PAWN && move.captured_piece == '.' && 
        abs_int(to_square - from_square) != 8 && abs_int(to_square - from_square) != 16) {
        incremental_data.pawn_structure_score = 0;
    }
}

// Zobrist hashing
uint64_t Evaluation::compute_zobrist_hash(const Board& board) const {
    uint64_t hash = 0;
    
    // Hash all pieces
    for (int square = 0; square < 64; ++square) {
        int rank = square / 8;
        int file = square % 8;
        char piece = board.get_piece(rank, file);
        
        if (piece != '.') {
            Board::PieceType piece_type = Board::char_to_piece_type(piece);
            Board::Color color = Board::char_to_color(piece);
            hash ^= zobrist_keys.piece_keys[color][piece_type][square];
        }
    }
    
    // Hash castling rights
    hash ^= zobrist_keys.castling_keys[board.get_castling_rights()];
    
    // Hash en passant
    if (board.get_en_passant_file() != -1) {
        hash ^= zobrist_keys.en_passant_keys[board.get_en_passant_file()];
    }
    
    // Hash side to move
    if (board.get_active_color() == Board::BLACK) {
        hash ^= zobrist_keys.side_to_move_key;
    }
    
    return hash;
}

// Update Zobrist hash incrementally
uint64_t Evaluation::update_zobrist_hash(uint64_t current_hash, const Move& move, const BitboardMoveUndoData& undo_data) const {
    uint64_t hash = current_hash;
    
    Board::PieceType piece_type = Board::char_to_piece_type(move.piece);
    Board::Color piece_color = Board::char_to_color(move.piece);
    
    int from_square = square_to_index(move.from_rank, move.from_file);
    int to_square = square_to_index(move.to_rank, move.to_file);
    
    // Remove piece from source square
    hash ^= zobrist_keys.piece_keys[piece_color][piece_type][from_square];
    
    // Add piece to destination square
    if (move.promotion_piece != '.') {
        Board::PieceType promotion_type = Board::char_to_piece_type(move.promotion_piece);
        hash ^= zobrist_keys.piece_keys[piece_color][promotion_type][to_square];
    } else {
        hash ^= zobrist_keys.piece_keys[piece_color][piece_type][to_square];
    }
    
    // Remove captured piece
    if (move.captured_piece != '.') {
        Board::PieceType captured_type = Board::char_to_piece_type(move.captured_piece);
        Board::Color captured_color = Board::char_to_color(move.captured_piece);
        hash ^= zobrist_keys.piece_keys[captured_color][captured_type][to_square];
    }
    
    // Update castling rights
    hash ^= zobrist_keys.castling_keys[undo_data.castling_rights];
    // Note: New castling rights would be XORed in by the caller
    
    // Update en passant
    if (undo_data.en_passant_file != -1) {
        hash ^= zobrist_keys.en_passant_keys[undo_data.en_passant_file];
    }
    
    // Toggle side to move
    hash ^= zobrist_keys.side_to_move_key;
    
    return hash;
}

// Material evaluation
int Evaluation::evaluate_material(const Board& board) const {
    int score = 0;
    
    for (int color = 0; color < 2; ++color) {
        int side_multiplier = side_sign(static_cast<Board::Color>(color));
        
        for (int piece_type = 0; piece_type < 6; ++piece_type) {
            Bitboard pieces = board.get_piece_bitboard(static_cast<Board::PieceType>(piece_type), 
                                                      static_cast<Board::Color>(color));
            int piece_count = count_bits(pieces);
            score += side_multiplier * piece_count * MATERIAL_VALUES[piece_type];
        }
    }
    
    return score;
}

// Piece-square table evaluation
int Evaluation::evaluate_piece_square_tables(const Board& board) const {
    int score = 0;
    GamePhase phase = get_game_phase(board);

    for (int square = 0; square < 64; ++square) {
        char piece = board.get_piece(square >> 3, square & 7);  // rank = square >> 3, file = square & 7

        if (piece == '.') continue;

        // Extract color and piece type directly once
        const Board::Color color = Board::char_to_color(piece);
        const Board::PieceType piece_type = Board::char_to_piece_type(piece);
        const int side_multiplier = (color == Board::WHITE) ? 1 : -1;

//        std::cout << piece_type << " " << color << " " << square << " " << get_piece_square_value(piece_type, color, square, phase) << std::endl;
        score += side_multiplier * get_piece_square_value(piece_type, color, square, phase);
    }

    return score;
}


// Get piece-square value for a specific piece
int Evaluation::get_piece_square_value(Board::PieceType piece_type, Board::Color color, int square, GamePhase phase) const {
    // Convert GamePhase to array index
    int phase_index;
    switch (phase) {
        case OPENING: phase_index = 0; break;
        case MIDDLEGAME: phase_index = 1; break;
        case ENDGAME: phase_index = 2; break;
        default: phase_index = 1; // Default to middlegame
    }
    
    return color == Board::WHITE ? PST[piece_type][phase_index][mirror_square(square)] 
                                 : PST[piece_type][phase_index][(square)];
}

// Pawn structure evaluation
int Evaluation::evaluate_pawn_structure(const Board& board) {
    // Try to get from pawn hash table first
    uint64_t pawn_hash = 0;
    
    // Calculate pawn hash (simplified - just XOR pawn positions)
    for (int color = 0; color < 2; ++color) {
        Bitboard pawns = board.get_piece_bitboard(Board::PAWN, static_cast<Board::Color>(color));
        pawn_hash ^= pawns << color;
    }
    
    auto it = pawn_hash_table.find(pawn_hash);
    if (it != pawn_hash_table.end()) {
        return it->second.score;
    }
    
    // Calculate pawn structure score
    int score = 0;
    score += evaluate_pawn_structure_for_color(board, Board::WHITE);
    score -= evaluate_pawn_structure_for_color(board, Board::BLACK);

    // Store in pawn hash table
    PawnHashEntry entry;
    entry.key = pawn_hash;
    entry.score = score;
    pawn_hash_table[pawn_hash] = entry;
    
    return score;
}

// Pawn structure evaluation for one color
int Evaluation::evaluate_pawn_structure_for_color(const Board& board, Board::Color color) const {
    int score = 0;
    Bitboard pawns = board.get_piece_bitboard(Board::PAWN, color);
    uint64_t pawn_bits = pawns;

    while (pawn_bits) {
        int square = get_lsb(pawn_bits);
        pawn_bits &= pawn_bits - 1; // Clear lowest set bit

        int rank = square >> 3;
        int file = square & 7;

        if (is_isolated_pawn(board, square, color)) {
            score += EvalConstants::ISOLATED_PAWN_PENALTY;
        }
        
        if (is_doubled_pawn(board, square, color)) {
            score += EvalConstants::DOUBLED_PAWN_PENALTY;
        }
        
        if (is_backward_pawn(board, square, color)) {
            score += EvalConstants::BACKWARD_PAWN_PENALTY;
        }

         if (is_passed_pawn(board, square, color)) {
             score += EvalConstants::PASSED_PAWN_BONUS;
             score += get_passed_pawn_rank_bonus(square, color);
         }
        
        if (is_pawn_chain(board, square, color)) {
            score += EvalConstants::PAWN_CHAIN_BONUS;
        }

        // Connected pawns (left/right on same rank)
        for (int df = -1; df <= 1; df += 2) {
            int adjacent_file = file + df;
            if (static_cast<unsigned>(adjacent_file) > 7) continue;
            int adjacent_square = (rank << 3) | adjacent_file; // rank * 8 + adjacent_file
            if (pawns & (1ULL << adjacent_square)) {
                score += EvalConstants::CONNECTED_PAWNS_BONUS;
                break;
            }
        }
    }
    
    return score;
}

// King safety evaluation
int Evaluation::evaluate_king_safety(const Board& board) const {
    int score = 0;
    score += evaluate_king_safety_for_color(board, Board::WHITE);
    score -= evaluate_king_safety_for_color(board, Board::BLACK);
    return score;
}

// King safety evaluation for one color - optimized
int Evaluation::evaluate_king_safety_for_color(const Board& board, Board::Color color) const {
    int score = 0;
    int king_pos = board.get_king_position(color);
    int king_file = king_pos & 7;
    int king_rank = king_pos >> 3;
    GamePhase phase = get_game_phase(board);
    
    // Cache commonly used values
    int expected_rank = (color == Board::WHITE) ? 0 : 7;
    bool is_castled = (king_rank == expected_rank && (king_file == 2 || king_file == 6));
    
    // I. Structural Safety (Static Factors)
    score += evaluate_pawn_shield(board, color);
    score += evaluate_open_files_near_king(board, color);
    score += evaluate_king_position_safety(board, color);
    score += evaluate_pawn_storms(board, color);
    score += evaluate_piece_cover(board, color);
    
    // II. Threat Evaluation (Dynamic Factors)
    score += evaluate_attacking_pieces_nearby(board, color);
    score += evaluate_king_mobility_and_escape(board, color);
    // score += evaluate_tactical_threats_to_king(board, color); TODO: Check Fix Implement
    score += evaluate_attack_maps_pressure_zones(board, color);
    
    // III. Game Phase Adjustments
    
    // IV. Dynamic Considerations

    // King activity penalty in middlegame - refined logic
    if (phase == MIDDLEGAME && !is_castled) {
        int distance_from_back_rank = abs_int(king_rank - expected_rank);
        score += EvalConstants::KING_ACTIVITY_PENALTY * distance_from_back_rank;
    }

    return score;
}

// =============================================================================
// NEW KING SAFETY EVALUATION FUNCTIONS
// =============================================================================

// I. STRUCTURAL SAFETY (Static Factors)
// TODO: Test the pawn shield evaluation function with various board states
// Pawn shield evaluation with detailed analysis
int Evaluation::evaluate_pawn_shield(const Board& board, Board::Color color) const {
    int score = 0;
    int king_pos = board.get_king_position(color);
    int king_file = king_pos & 7;
    int king_rank = king_pos >> 3;
    
    Bitboard pawns = board.get_piece_bitboard(Board::PAWN, color);
    int pawn_direction = (color == Board::WHITE) ? 1 : -1;
    
    // Calculate shield area around king (3 files: king_file-1, king_file, king_file+1)
    // This optimization focuses evaluation on the immediate king area only
    int left_file = max_int(0, king_file - 1);
    int right_file = min_int(7, king_file + 1);
    
    // Create shield zone mask using precomputed file masks - avoids per-rank loops
    // This bitboard represents all squares in the three files around the king
    Bitboard shield_zone_mask = 0ULL;
    for (int file = left_file; file <= right_file; ++file) {
        shield_zone_mask |= file_masks[file];
    }
    
    // Get pawns in shield zone only - eliminates checking irrelevant pawns
    // This is a key optimization: we only work with pawns that matter for king safety
    Bitboard shield_pawns_bb = pawns & shield_zone_mask;
    
    // 1. IMMEDIATE SHIELD EVALUATION (rank directly in front of king)
    // The most critical defensive line - pawns one rank ahead of the king
    int immediate_shield_rank = king_rank + pawn_direction;
    int shield_pawns = 0;
    int pawn_gaps = 0;
    int connected_pawns = 0;
    
    if (immediate_shield_rank >= 0 && immediate_shield_rank <= 7) {
        // Create rank mask for immediate shield using bit manipulation
        // This replaces nested loops with efficient bitboard operations
        Bitboard immediate_rank_mask = 0xFFULL << (immediate_shield_rank * 8);
        Bitboard immediate_shield_mask = immediate_rank_mask & shield_zone_mask;
        Bitboard immediate_shield_pawns = shield_pawns_bb & immediate_shield_mask;
        
        // Count shield pawns using efficient bit counting - replaces manual counting
        shield_pawns = BitboardUtils::popcount(immediate_shield_pawns);
        
        // Calculate gaps efficiently: total possible positions minus actual pawns
        pawn_gaps = (right_file - left_file + 1) - shield_pawns;
        
        // Count pairwise connections using optimized bitboard operations
        // This counts actual adjacent pairs rather than individual connected pawns
        int pairwise_connections = BitboardUtils::popcount(
            (immediate_shield_pawns << 1) & immediate_shield_pawns & 0xFEFEFEFEFEFEFEFEULL
        ) + BitboardUtils::popcount(
            (immediate_shield_pawns >> 1) & immediate_shield_pawns & 0x7F7F7F7F7F7F7F7FULL
        );
        connected_pawns = pairwise_connections;
    } else {
        // King on edge rank - no shield possible, all files are gaps
        pawn_gaps = right_file - left_file + 1;
    }
    
    // 2. ADVANCED PAWNS EVALUATION (ranks 2-3 in front of king)
    // Advanced pawns can weaken the shield by creating holes in the defense
    int advanced_pawns = 0;
    for (int rank_offset = 2; rank_offset <= 3; ++rank_offset) {
        int check_rank = king_rank + (rank_offset * pawn_direction);
        if (check_rank >= 0 && check_rank <= 7) {
            // Use bitboard operations to efficiently check for advanced pawns
            Bitboard advanced_rank_mask = 0xFFULL << (check_rank * 8);
            Bitboard advanced_mask = advanced_rank_mask & shield_zone_mask;
            Bitboard advanced_pawns_bb = shield_pawns_bb & advanced_mask;
            advanced_pawns += BitboardUtils::popcount(advanced_pawns_bb);
        }
    }
    
    // Apply shield scoring based on calculated metrics
    score += shield_pawns * EvalConstants::PAWN_SHIELD_BONUS;
    score += connected_pawns * EvalConstants::CONNECTED_PAWNS_BONUS;
    score -= pawn_gaps * 12; // Penalty for gaps in shield
    score -= advanced_pawns * 8; // Penalty for advanced pawns weakening shelter
    
    // 3. PAWN STRUCTURE ANALYSIS USING EXISTING FUNCTIONS
    // Leverage existing optimized pawn evaluation functions instead of reimplementing
    // This avoids code duplication and uses precomputed masks for efficiency
    
    for (int file = left_file; file <= right_file; ++file) {
        // Get all pawns on this file using precomputed file mask
        Bitboard file_pawns = pawns & file_masks[file];
        
        if (file_pawns) {
            // Find any pawn on this file to use with existing functions
            // We use the first pawn found as a representative for file-based checks
            int representative_square = BitboardUtils::lsb(file_pawns);
            
            // Use existing optimized doubled pawn function with precomputed masks
            // This function already uses file_masks internally for efficiency
            if (is_doubled_pawn(board, representative_square, color)) {
                score += EvalConstants::DOUBLED_PAWN_PENALTY / 2; // Reduced penalty near king
            }
            
            // Use existing optimized isolated pawn function with precomputed masks
            // This function uses isolated_pawn_masks for O(1) adjacency checking
            if (is_isolated_pawn(board, representative_square, color)) {
                score += EvalConstants::ISOLATED_PAWN_PENALTY / 2; // Reduced penalty near king
            }
        }
    }
    
    // 4. ADDITIONAL SHIELD QUALITY FACTORS
    // Reuse pawn_gaps info for cleaner conditional logic
    if (shield_pawns == 0) {
        score -= EvalConstants::KING_EXPOSED_PENALTY;
    } else if (pawn_gaps == 0) {
        score += EvalConstants::KING_PAWN_SHIELD;
    }
    
    // 5. ENEMY PAWN STORM EVALUATION (OPTIMIZED WITH PIPELINED BITBOARDS)
    // Check for enemy pawns that could create threats against our king
    Board::Color enemy_color = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;
    Bitboard enemy_pawns = board.get_piece_bitboard(Board::PAWN, enemy_color);
    
    // Build storm ranks mask in a single pass to reduce bitmasking logic
    Bitboard storm_ranks_mask = 0ULL;
    for (int r = 1; r <= 4; ++r) {
        int rank = king_rank + (-pawn_direction * r);
        if (rank >= 0 && rank <= 7) {
            storm_ranks_mask |= (0xFFULL << (rank * 8));
        }
    }
    
    // Get all storm pawns in one operation
    Bitboard storm_pawns = enemy_pawns & shield_zone_mask & storm_ranks_mask;
    
    // Compute storm danger using efficient bit manipulation
    int storm_penalty = 0;
    while (storm_pawns) {
        int sq = BitboardUtils::pop_lsb(storm_pawns);
        int sq_rank = sq >> 3;
        int distance = abs_int(sq_rank - king_rank);
        storm_penalty += (5 - distance);
    }
    
    // Apply game phase scaling to storm penalty
    // Storm threats are more dangerous in middlegame than endgame
    // TODO: Add proper game phase detection - for now assume middlegame scaling
    storm_penalty *= 1; // (phase == MIDDLEGAME) ? 1 : 0.5;
    
    score -= storm_penalty;
    
    return score;
}

// Evaluate open and semi-open files near the king
int Evaluation::evaluate_open_files_near_king(const Board& board, Board::Color color) const {
    int score = 0;
    int king_pos = board.get_king_position(color);
    int king_file = king_pos & 7;
    Board::Color enemy_color = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;
    
    // Check files around king
    for (int file_offset = -2; file_offset <= 2; ++file_offset) {
        int check_file = king_file + file_offset;
        if (check_file < 0 || check_file > 7) continue;
        
        bool is_open = is_file_open(board, check_file);
        bool is_semi_open = is_file_semi_open(board, check_file, color);
        
        if (is_open) {
            // Open file penalty increases with proximity to king
            int proximity_factor = 3 - abs_int(file_offset);
            score += EvalConstants::OPEN_FILE_NEAR_KING_PENALTY * proximity_factor / 3;
            
            // Extra penalty if king is directly on open file
            if (check_file == king_file) {
                score += EvalConstants::KING_ON_OPEN_FILE_PENALTY;
            }
            
            // Check if enemy major pieces can access this file
            Bitboard enemy_rooks = board.get_piece_bitboard(Board::ROOK, enemy_color);
            Bitboard enemy_queens = board.get_piece_bitboard(Board::QUEEN, enemy_color);
            
            uint64_t major_pieces = enemy_rooks | enemy_queens;
            while (major_pieces) {
                int sq = BitboardUtils::pop_lsb(major_pieces);
                if ((sq & 7) == check_file) {
                    score += EvalConstants::OPEN_FILE_NEAR_KING_PENALTY / 2;
                }
            }
        } else if (is_semi_open) {
            int proximity_factor = 3 - abs_int(file_offset);
            score += EvalConstants::SEMI_OPEN_FILE_NEAR_KING_PENALTY * proximity_factor / 3;
        }
    }
    
    return score;
}

// Evaluate king position safety (castled vs uncastled, edge vs center)
int Evaluation::evaluate_king_position_safety(const Board& board, Board::Color color) const {
    int score = 0;
    int king_pos = board.get_king_position(color);
    int king_file = king_pos & 7;
    int king_rank = king_pos >> 3;
    int expected_rank = (color == Board::WHITE) ? 0 : 7;
    GamePhase phase = get_game_phase(board);
    
    // 1. Castled vs uncastled
    bool is_castled = king_rank == expected_rank && (king_file == 2 || king_file == 6);
    if (is_castled) {
        score += EvalConstants::CASTLING_BONUS;
        
        // Kingside vs queenside safety
        if (king_file == 6) { // Kingside
            score += 10;
        } else { // Queenside
            score += 5;
        }
    }
    
    // 2. Edge vs center positioning
    if (phase == OPENING || phase == MIDDLEGAME) {
        // In opening/middlegame, edge is safer
        if (king_file <= 2 || king_file >= 5) {
            score += 8; // Edge bonus
        } else {
            score -= 15; // Center penalty
        }
        
        // King stuck in center during middlegame is very unsafe
        if (phase == MIDDLEGAME && king_file >= 3 && king_file <= 4 && king_rank != expected_rank) {
            score += EvalConstants::KING_EXPOSED_PENALTY;
        }
    } else if (phase == ENDGAME) {
        // In endgame, proximity to center is beneficial
        int center_distance = min_int(abs_int(king_file - 3), abs_int(king_file - 4)) +
                             min_int(abs_int(king_rank - 3), abs_int(king_rank - 4));
        score += (6 - center_distance) * 3; // Bonus for centralization
    }
    
    return score;
}

// Evaluate pawn storms (friendly or enemy pawns advancing toward king)
int Evaluation::evaluate_pawn_storms(const Board& board, Board::Color color) const {
    int score = 0;
    int king_pos = board.get_king_position(color);
    int king_file = king_pos & 7;
    int king_rank = king_pos >> 3;
    Board::Color enemy_color = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;
    
    // Early exit: Skip pawn storm evaluation if neither side has likely castled
    // Check if kings are in castled positions (files 2 or 6 on back ranks)
    int expected_rank = (color == Board::WHITE) ? 0 : 7;
    int enemy_expected_rank = (enemy_color == Board::WHITE) ? 0 : 7;
    
    bool king_castled = (king_rank == expected_rank && (king_file == 2 || king_file == 6));
    
    int enemy_king_pos = board.get_king_position(enemy_color);
    int enemy_king_file = enemy_king_pos & 7;
    int enemy_king_rank = enemy_king_pos >> 3;
    bool enemy_king_castled = (enemy_king_rank == enemy_expected_rank && (enemy_king_file == 2 || enemy_king_file == 6));
    
    // If neither king has castled, pawn storms are less relevant
    if (!king_castled && !enemy_king_castled) {
        return 0;
    }
    
    Bitboard enemy_pawns = board.get_piece_bitboard(Board::PAWN, enemy_color);
    int enemy_direction = (enemy_color == Board::WHITE) ? 1 : -1;
    
    // Optimized: Iterate only over existing enemy pawns using pop_lsb
    while (enemy_pawns) {
        int sq = BitboardUtils::pop_lsb(enemy_pawns);
        int file = sq & 7;
        int rank = sq >> 3;
        
        int file_offset = file - king_file;
        int rank_offset = (rank - king_rank) * enemy_direction;
        
        // Skip pawns outside storm zone or moving away from king
        if (abs_int(file_offset) > 2 || rank_offset <= 0 || rank_offset > 4)
            continue;
        
        // Branchless penalty calculation
        int proximity = 3 - abs_int(file_offset);
        int advancement = 5 - rank_offset;
        score -= (proximity * advancement * 3);
        
        // Extra penalty for wide pawn storms (opposite-side castling scenarios)
        if (abs_int(file_offset) >= 2) {
            score -= 8;
        }
    }
    
    return score;
}

// Evaluate piece cover (friendly minor pieces near king acting as defenders)
int Evaluation::evaluate_piece_cover(const Board& board, Board::Color color) const {
    int score = 0;
    const int king_pos = board.get_king_position(color);
    const int king_file = king_pos & 7;
    const int king_rank = king_pos >> 3;
    const int expected_rank = color == Board::WHITE ? 0 : 7;

    int defending_pieces = 0;

    auto evaluate_defender = [&](Bitboard defenders, int max_distance, int base_bonus, bool is_bishop) {
        while (defenders) {
            int sq = BitboardUtils::pop_lsb(defenders);
            int file = sq & 7;
            int rank = sq >> 3;

            int dist = max_int(abs_int((sq & 7) - king_file), abs_int((sq >> 3) - king_rank));
            if (dist > max_distance) continue;

            defending_pieces++;
            score += base_bonus;

            if (dist <= 2 && !is_bishop)
                score += 4; // extra bonus for knight close to king

            // Check fianchetto bishop
            if (is_bishop && rank == expected_rank + (color == Board::WHITE ? 1 : -1)) {
                if ((king_file <= 3 && file <= 2) || (king_file >= 4 && file >= 5)) {
                    score += EvalConstants::FIANCHETTO_BONUS;
                }
            }
        }
    };

    evaluate_defender(board.get_piece_bitboard(Board::KNIGHT, color), 3, 8, false);
    evaluate_defender(board.get_piece_bitboard(Board::BISHOP, color), 4, 6, true);

    if (defending_pieces == 0)
        score -= 15;
    else if (defending_pieces == 1)
        score -= 5;

    return score;
}


// II. THREAT EVALUATION (Dynamic Factors)

// Evaluate attacking pieces nearby with weight system
int Evaluation::evaluate_attacking_pieces_nearby(const Board& board, Board::Color color) const {
    int score = 0;
    int king_pos = board.get_king_position(color);
    Board::Color enemy_color = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;
    
    // Cache king coordinates to avoid recomputing in inner loops
    int king_rank = king_pos >> 3;
    int king_file = king_pos & 7;
    
    // Attack weight system: queen = 4, rook = 2, bishop/knight = 1
    static const int attack_weights[] = {0, 1, 1, 2, 4, 0}; // PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
    
    int total_attack_weight = 0;
    int attacking_pieces_count = 0;
    
    // Check each enemy piece type
    for (int piece_type = Board::KNIGHT; piece_type <= Board::QUEEN; ++piece_type) {
        Bitboard pieces = board.get_piece_bitboard(static_cast<Board::PieceType>(piece_type), enemy_color);
        
        // Use BitboardUtils::pop_lsb for efficient bit extraction
        while (pieces) {
            int piece_square = BitboardUtils::pop_lsb(pieces);
            
            // Calculate distance using cached king coordinates
            int piece_rank = piece_square >> 3;
            int piece_file = piece_square & 7;
            int rank_diff = abs_int(king_rank - piece_rank);
            int file_diff = abs_int(king_file - piece_file);
            int distance = max_int(rank_diff, file_diff); // Chebyshev distance
            
            // Consider pieces within 4 squares as "nearby"
            if (distance <= 4) {
                int weight = attack_weights[piece_type];
                
                // Weight decreases with distance
                int distance_factor = (5 - distance);
                total_attack_weight += (weight * distance_factor) / 2;
                attacking_pieces_count++;
                
                // Direct logical checks for alignment (removed has_clear_line variable)
                bool is_aligned = false;
                
                // Check rank/file alignment for rooks and queens
                if ((piece_type == Board::ROOK || piece_type == Board::QUEEN) &&
                    (piece_rank == king_rank || piece_file == king_file)) {
                    is_aligned = true;
                }
                
                // Check diagonal alignment for bishops and queens
                if ((piece_type == Board::BISHOP || piece_type == Board::QUEEN) &&
                    (rank_diff == file_diff)) {
                    is_aligned = true;
                }
                
                if (is_aligned) {
                    score -= weight * 8; // Extra penalty for pieces with clear lines
                }
            }
        }
    }
    
    // Apply penalties based on total attack weight
    score -= total_attack_weight * 3;
    
    // Progressive penalty for multiple attackers (simplified conditional)
    score -= (attacking_pieces_count >= 3) ? 20 : (attacking_pieces_count >= 2) ? 10 : 0;
    
    return score;
}

// Evaluate king mobility and escape squares
int Evaluation::evaluate_king_mobility_and_escape(const Board& board, Board::Color color) const {
    int score = 0;
    int king_pos = board.get_king_position(color);
    Board::Color enemy_color = color == Board::WHITE ? Board::BLACK : Board::WHITE;
    
    // Cache bitboards for efficient square occupancy tests
    Bitboard own_pieces = board.get_color_bitboard(color);

    int escape_squares = 0;
    int safe_escape_squares = 0;
    int attacked_escape_squares = 0;
    int blocked_escape_squares = 0;
    
    // Use precomputed king attack table for adjacent squares
    Bitboard king_attacks = BitboardUtils::king_attacks(king_pos);
    
    while (king_attacks) {
        int escape_square = BitboardUtils::pop_lsb(king_attacks);
        
        // Optimized square occupancy test using bitboard
        if (BitboardUtils::get_bit(own_pieces, escape_square)) {
            blocked_escape_squares++;
            continue;
        }
        
        escape_squares++;
        
        // This checks attacks from all enemy pieces: pawns, knights, bishops, rooks, queens, and king
        bool is_attacked = board.is_square_attacked(escape_square, enemy_color);
        
        if (is_attacked) {
            attacked_escape_squares++;
        } else {
            safe_escape_squares++;
        }
    }
    
    // Scoring based on escape square quality
    score += safe_escape_squares * EvalConstants::KING_ESCAPE_SQUARES_BONUS;
    score += attacked_escape_squares * (EvalConstants::KING_ESCAPE_SQUARES_BONUS / 3);
    
    // Severe penalty for trapped king
    if (safe_escape_squares == 0) {
        score -= 25;
        if (attacked_escape_squares == 0 && blocked_escape_squares >= 6) {
            score -= 30; // King is completely trapped
        }
    }
    
    // Bonus for very mobile king
    if (safe_escape_squares >= 6) {
        score += 8;
    }
    
    return score;
}

// Tactical threats evaluation
int Evaluation::evaluate_tactical_threats_to_king(const Board& board, Board::Color color) const {
    int score = 0;

    // 1. Pins, skewers, forks involving the king
    score += evaluate_pins(board, color);
    
    return score;
}

// Helper function for pins to the king
int Evaluation::evaluate_pins(const Board& board, Board::Color color) const {
    int score = 0;
    int king_pos = board.get_king_position(color);
    Board::Color enemy_color = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;
    
    // Early exit if no enemy sliding pieces
    Bitboard enemy_rooks = board.get_piece_bitboard(Board::ROOK, enemy_color);
    Bitboard enemy_bishops = board.get_piece_bitboard(Board::BISHOP, enemy_color);
    Bitboard enemy_queens = board.get_piece_bitboard(Board::QUEEN, enemy_color);
    
    if (!(enemy_rooks | enemy_bishops | enemy_queens)) {
        return 0; // No sliding pieces to create pins
    }
    
    Bitboard own_pieces = board.get_color_bitboard(color);
    Bitboard all_pieces = own_pieces | board.get_color_bitboard(enemy_color);
    
    // Check rook/queen pins (horizontal and vertical)
    Bitboard rank_file_pinners = enemy_rooks | enemy_queens;
    while (rank_file_pinners) {
        int pinner_square = BitboardUtils::pop_lsb(rank_file_pinners);
        
        // Get rook attacks from pinner position with all pieces as blockers
        Bitboard pinner_attacks = BitboardUtils::rook_attacks(pinner_square, all_pieces);
        
        // Check if king is in the attack ray
        if (BitboardUtils::get_bit(pinner_attacks, king_pos)) {
            // Get rook attacks from king position with all pieces as blockers
            Bitboard king_attacks = BitboardUtils::rook_attacks(king_pos, all_pieces);
            
            // Check if pinner is in king's attack ray (confirming they're on same rank/file)
            if (BitboardUtils::get_bit(king_attacks, pinner_square)) {
                // Get pieces between king and pinner by removing both endpoints
                Bitboard between_mask = pinner_attacks & king_attacks;
                Bitboard pieces_between = between_mask & all_pieces;
                
                // Check if exactly one piece between them and it's our own
                if (BitboardUtils::popcount(pieces_between) == 1) {
                    if (pieces_between & own_pieces) {
                        score += EvalConstants::PIN_ON_KING_PENALTY;
                    }
                }
            }
        }
    }
    
    // Check bishop/queen pins (diagonal)
    Bitboard diagonal_pinners = enemy_bishops | enemy_queens;
    while (diagonal_pinners) {
        int pinner_square = BitboardUtils::pop_lsb(diagonal_pinners);
        
        // Get bishop attacks from pinner position with all pieces as blockers
        Bitboard pinner_attacks = BitboardUtils::bishop_attacks(pinner_square, all_pieces);
        
        // Check if king is in the attack ray
        if (BitboardUtils::get_bit(pinner_attacks, king_pos)) {
            // Get bishop attacks from king position with all pieces as blockers
            Bitboard king_attacks = BitboardUtils::bishop_attacks(king_pos, all_pieces);
            
            // Check if pinner is in king's attack ray (confirming they're on same diagonal)
            if (BitboardUtils::get_bit(king_attacks, pinner_square)) {
                // Get pieces between king and pinner by finding intersection
                Bitboard between_mask = pinner_attacks & king_attacks;
                Bitboard pieces_between = between_mask & all_pieces;
                
                // Check if exactly one piece between them and it's our own
                if (BitboardUtils::popcount(pieces_between) == 1) {
                    if (pieces_between & own_pieces) {
                        score += EvalConstants::PIN_ON_KING_PENALTY;
                    }
                }
            }
        }
    }
    
    return score;
}

// Evaluate attack maps and pressure zones (optimized with bitboard operations)
// Comprehensive king safety evaluation combining attack maps, pressure zones, and line of fire threats
int Evaluation::evaluate_attack_maps_pressure_zones(const Board& board, Board::Color color) const {
    // Constants for evaluation
    static constexpr int LINE_OF_FIRE_PENALTY = 12;
    static constexpr int BACK_RANK_LINE_PENALTY = 15;
    static constexpr int CLOSE_DISTANCE_PENALTY = 8;
    static constexpr int DIAGONAL_LINE_PENALTY = 10;
    static constexpr int DIAGONAL_CLOSE_PENALTY = 6;
    static constexpr int MAX_LINE_DISTANCE = 4;
    static constexpr int CLOSE_DISTANCE_THRESHOLD = 2;
    static constexpr int MIN_BLOCKED_ESCAPE_SQUARES = 2;
    static constexpr int CONTROLLED_SQUARE_PENALTY = 6;
    static constexpr int UNDEFENDED_SQUARE_PENALTY = 10;
    static constexpr int ZONE_CONTROL_PENALTY = 20;
    
    int score = 0;
    int king_pos = board.get_king_position(color);
    int king_rank = king_pos >> 3;
    int king_file = king_pos & 7;
    int expected_rank = (color == Board::WHITE) ? 0 : 7;
    Board::Color enemy_color = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;
    
    // Early exit if no enemy pieces
    Bitboard enemy_pieces = board.get_color_bitboard(enemy_color);
    if (!enemy_pieces) {
        return 0;
    }
    
    // Cache frequently used bitboards
    Bitboard all_pieces = board.get_all_pieces();
    Bitboard enemy_pawns = board.get_piece_bitboard(Board::PAWN, enemy_color);
    Bitboard enemy_knights = board.get_piece_bitboard(Board::KNIGHT, enemy_color);
    Bitboard enemy_bishops = board.get_piece_bitboard(Board::BISHOP, enemy_color);
    Bitboard enemy_rooks = board.get_piece_bitboard(Board::ROOK, enemy_color);
    Bitboard enemy_queens = board.get_piece_bitboard(Board::QUEEN, enemy_color);
    
    Bitboard our_pawns = board.get_piece_bitboard(Board::PAWN, color);
    Bitboard our_knights = board.get_piece_bitboard(Board::KNIGHT, color);
    Bitboard our_bishops = board.get_piece_bitboard(Board::BISHOP, color);
    Bitboard our_rooks = board.get_piece_bitboard(Board::ROOK, color);
    Bitboard our_queens = board.get_piece_bitboard(Board::QUEEN, color);
    
    // === PART 1: ATTACK MAPS AND PRESSURE ZONES ===
    
    // Use precomputed king zone mask (3x3 area around king)
    Bitboard king_zone = BitboardUtils::king_attacks(king_pos) | (1ULL << king_pos);
    Bitboard king_square_mask = 1ULL << king_pos;
    
    // Calculate enemy attack map efficiently
    Bitboard enemy_attack_map = 0ULL;
    
    // Enemy pawn attacks
    if (enemy_pawns) {
        enemy_attack_map |= BitboardUtils::pawn_attacks(enemy_pawns, enemy_color);
    }
    
    // Enemy knight attacks
    Bitboard temp_knights = enemy_knights;
    while (temp_knights) {
        int knight_square = BitboardUtils::pop_lsb(temp_knights);
        enemy_attack_map |= BitboardUtils::knight_attacks(knight_square);
    }
    
    // Enemy sliding piece attacks (will be used for both attack map and line of fire)
    Bitboard diagonal_attackers = enemy_bishops | enemy_queens;
    Bitboard rank_file_attackers = enemy_rooks | enemy_queens;
    
    // Enemy bishop/queen diagonal attacks
    Bitboard temp_diagonal = diagonal_attackers;
    while (temp_diagonal) {
        int piece_square = BitboardUtils::pop_lsb(temp_diagonal);
        enemy_attack_map |= BitboardUtils::bishop_attacks(piece_square, all_pieces);
    }
    
    // Enemy rook/queen rank/file attacks
    Bitboard temp_rank_file = rank_file_attackers;
    while (temp_rank_file) {
        int piece_square = BitboardUtils::pop_lsb(temp_rank_file);
        enemy_attack_map |= BitboardUtils::rook_attacks(piece_square, all_pieces);
    }
    
    // Enemy king attacks (less relevant unless endgame)
    GamePhase phase = get_game_phase(board);
    if (phase == ENDGAME) {
        int enemy_king_pos = board.get_king_position(enemy_color);
        enemy_attack_map |= BitboardUtils::king_attacks(enemy_king_pos);
    }
    
    // Calculate our defense map using actual attack sets
    Bitboard our_defense_map = 0ULL;
    
    // Our pawn defenses
    if (our_pawns) {
        our_defense_map |= BitboardUtils::pawn_attacks(our_pawns, color);
    }
    
    // Our knight defenses
    Bitboard temp_our_knights = our_knights;
    while (temp_our_knights) {
        int knight_square = BitboardUtils::pop_lsb(temp_our_knights);
        our_defense_map |= BitboardUtils::knight_attacks(knight_square);
    }
    
    // Our bishop/queen diagonal defenses
    Bitboard our_diagonal_defenders = our_bishops | our_queens;
    while (our_diagonal_defenders) {
        int piece_square = BitboardUtils::pop_lsb(our_diagonal_defenders);
        our_defense_map |= BitboardUtils::bishop_attacks(piece_square, all_pieces);
    }
    
    // Our rook/queen rank/file defenses
    Bitboard our_rank_file_defenders = our_rooks | our_queens;
    while (our_rank_file_defenders) {
        int piece_square = BitboardUtils::pop_lsb(our_rank_file_defenders);
        our_defense_map |= BitboardUtils::rook_attacks(piece_square, all_pieces);
    }
    
    // Our king defenses
    our_defense_map |= BitboardUtils::king_attacks(king_pos);
    
    // Analyze king zone control
    Bitboard controlled_zone = enemy_attack_map & king_zone;
    Bitboard undefended_zone = controlled_zone & ~our_defense_map;
    
    // Remove king square from analysis
    controlled_zone &= ~king_square_mask;
    undefended_zone &= ~king_square_mask;
    
    int controlled_squares = BitboardUtils::popcount(controlled_zone);
    int undefended_squares = BitboardUtils::popcount(undefended_zone);
    int total_zone_squares = BitboardUtils::popcount(king_zone) - 1; // Exclude king square
    
    // Penalties based on enemy control
    score -= controlled_squares * CONTROLLED_SQUARE_PENALTY;
    score -= undefended_squares * UNDEFENDED_SQUARE_PENALTY;
    
    // Extra penalty if most of king zone is controlled
    if (controlled_squares >= total_zone_squares * 2 / 3) {
        score -= ZONE_CONTROL_PENALTY;
    }
    
    // === PART 2: LINE OF FIRE THREATS ===
    
    // Check if we should skip sliding piece analysis
    bool skip_sliding_analysis = !(enemy_rooks | enemy_bishops | enemy_queens);
    
    if (!skip_sliding_analysis) {
    
        // Check rook/queen line of fire threats (rank and file)
        temp_rank_file = rank_file_attackers;
        while (temp_rank_file) {
            int piece_square = BitboardUtils::pop_lsb(temp_rank_file);
            
            // Use rook attacks to check if king is in line of fire
            Bitboard piece_attacks = BitboardUtils::rook_attacks(piece_square, all_pieces);
            if (BitboardUtils::get_bit(piece_attacks, king_pos)) {
                // Calculate Chebyshev distance efficiently
                int distance = std::max(abs((piece_square >> 3) - king_rank), 
                                    abs((piece_square & 7) - king_file));
                
                if (distance <= MAX_LINE_DISTANCE) {
                    // Consolidated penalty calculation
                    int penalty = LINE_OF_FIRE_PENALTY;
                    if (king_rank == expected_rank) {
                        penalty += BACK_RANK_LINE_PENALTY;
                    }
                    if (distance <= CLOSE_DISTANCE_THRESHOLD) {
                        penalty += CLOSE_DISTANCE_PENALTY;
                    }
                    score -= penalty;
                }
            }
        }
        
        // Check bishop/queen diagonal line of fire threats
        temp_diagonal = diagonal_attackers;
        while (temp_diagonal) {
            int piece_square = BitboardUtils::pop_lsb(temp_diagonal);
            
            // Use bishop attacks to check if king is in line of fire
            Bitboard piece_attacks = BitboardUtils::bishop_attacks(piece_square, all_pieces);
            if (BitboardUtils::get_bit(piece_attacks, king_pos)) {
                // Calculate Chebyshev distance for diagonals
                int distance = std::max(abs((piece_square >> 3) - king_rank), 
                                    abs((piece_square & 7) - king_file));
                
                if (distance <= MAX_LINE_DISTANCE) {
                    // Consolidated diagonal penalty calculation
                    int penalty = DIAGONAL_LINE_PENALTY + (distance <= CLOSE_DISTANCE_THRESHOLD ? DIAGONAL_CLOSE_PENALTY : 0);
                    score -= penalty;
                }
            }
        }
    }
    
    // === PART 3: BACK RANK ESCAPE SQUARE ANALYSIS ===
    
    if (king_rank == expected_rank) {
        Bitboard own_pieces = board.get_color_bitboard(color);
        
        // Create escape square mask in front of king
        int escape_rank = (color == Board::WHITE) ? 1 : 6;
        
        Bitboard escape_mask = 0ULL;
        for (int file_offset = -1; file_offset <= 1; ++file_offset) {
            int escape_file = king_file + file_offset;
            if (escape_file >= 0 && escape_file <= 7) {
                int escape_square = (escape_rank << 3) | escape_file;
                escape_mask |= (1ULL << escape_square);
            }
        }
        
        // Count blocked escape squares using popcount
        Bitboard blocked_escape_mask = escape_mask & own_pieces;
        int blocked_escape_squares = BitboardUtils::popcount(blocked_escape_mask);
        
        // Penalty increases with more blocked squares
        if (blocked_escape_squares >= MIN_BLOCKED_ESCAPE_SQUARES) {
            score -= EvalConstants::BACK_RANK_WEAKNESS_PENALTY;
            
            // Check if there are enemy major pieces on back rank using bitboard masking
            Bitboard back_rank_mask = 0xFFULL << (expected_rank * 8);
            Bitboard back_rank_attackers = (enemy_rooks | enemy_queens) & back_rank_mask;
            
            if (back_rank_attackers) {
                score -= EvalConstants::BACK_RANK_WEAKNESS_PENALTY / 2;
            }
        }
    }
    
    return score;
}

// III. GAME PHASE ADJUSTMENTS

// IV. DYNAMIC CONSIDERATIONS

// Evaluate potential for future attacks
// Mobility evaluation
int Evaluation::evaluate_mobility(const Board& board) const {
    return evaluate_mobility_for_color(board, Board::WHITE) -
           evaluate_mobility_for_color(board, Board::BLACK);
}

// Mobility evaluation for one color
int Evaluation::evaluate_mobility_for_color(const Board& board, Board::Color color) const {
    int score = 0;

    Bitboard all_pieces = board.get_all_pieces();
    Bitboard own_pieces = board.get_color_bitboard(color);

    // Pre-store bonuses for quick access, indexed by piece_type
    static constexpr int MOBILITY_BONUSES[6] = {
        0, // PAWN (ignored here)
        EvalConstants::KNIGHT_MOBILITY_BONUS,
        EvalConstants::BISHOP_MOBILITY_BONUS,
        EvalConstants::ROOK_MOBILITY_BONUS,
        EvalConstants::QUEEN_MOBILITY_BONUS,
        0 // KING (ignored)
    };

    for (int piece_type = Board::KNIGHT; piece_type <= Board::QUEEN; ++piece_type) {
        Bitboard pieces = board.get_piece_bitboard(static_cast<Board::PieceType>(piece_type), color);
        uint64_t piece_bits = pieces;

        while (piece_bits) {
            int square = get_lsb(piece_bits);
            piece_bits &= piece_bits - 1;

            Bitboard attacks = 0;
            switch (piece_type) {
                case Board::KNIGHT:
                    attacks = BitboardUtils::knight_attacks(square);
                break;
                case Board::BISHOP:
                    attacks = BitboardUtils::bishop_attacks(square, all_pieces);
                break;
                case Board::ROOK:
                    attacks = BitboardUtils::rook_attacks(square, all_pieces);
                break;
                case Board::QUEEN:
                    attacks = BitboardUtils::queen_attacks(square, all_pieces);
                break;
                default:
                    break;
            }

            attacks &= ~own_pieces;
            int mobility = count_bits(attacks);
            score += mobility * MOBILITY_BONUSES[piece_type];
        }
    }

    return score;
}


// Piece coordination evaluation
int Evaluation::evaluate_piece_coordination(const Board& board) const {
    return evaluate_piece_coordination_for_color(board, Board::WHITE) - 
           evaluate_piece_coordination_for_color(board, Board::BLACK);
}

int Evaluation::evaluate_piece_coordination_for_color(const Board& board, Board::Color color) const {
    int score = 0;
    
    // Rook coordination (rooks on same rank/file) - this function was modified for efficiency
    Bitboard rooks = board.get_piece_bitboard(Board::ROOK, color);
    uint64_t rook_bits = rooks;

    int rook_squares[16];  // Max 16 rooks per side is more than enough
    int rook_count = 0;

    while (rook_bits) {
        int square = get_lsb(rook_bits);
        rook_squares[rook_count++] = square;
        rook_bits &= rook_bits - 1;
    }

    for (int i = 0; i < rook_count; ++i) {
        int sq1 = rook_squares[i];
        int rank1 = sq1 >> 3;
        int file1 = sq1 & 7;

        for (int j = i + 1; j < rook_count; ++j) {
            int sq2 = rook_squares[j];
            int rank2 = sq2 >> 3;
            int file2 = sq2 & 7;

            if (rank1 == rank2 || file1 == file2) {
                score += EvalConstants::ROOK_COORDINATION_BONUS;
            }
        }
    }

    
    // Bishop pair bonus
    Bitboard bishops = board.get_piece_bitboard(Board::BISHOP, color);
    if (count_bits(bishops) >= 2) {
        score += EvalConstants::BISHOP_PAIR_BONUS;
    }
    
    // Knight outpost bonus (knight on strong square supported by pawn) - this function was modified for efficiency
    Bitboard knights = board.get_piece_bitboard(Board::KNIGHT, color);
    Bitboard pawns = board.get_piece_bitboard(Board::PAWN, color);

    uint64_t knight_bits = knights;
    const bool white = (color == Board::WHITE);

    while (knight_bits) {
        int sq = get_lsb(knight_bits);
        knight_bits &= knight_bits - 1;

        int rank = sq >> 3;
        int file = sq & 7;

        bool in_enemy_territory = white ? (rank >= 4) : (rank <= 3);
        if (!in_enemy_territory) continue;

        int support_rank = white ? (rank - 1) : (rank + 1);
        if (support_rank < 0 || support_rank >= 8) continue;

        int sq_left  = (support_rank << 3) | (file - 1);
        int sq_right = (support_rank << 3) | (file + 1);

        if ((file > 0 && (pawns & (1ULL << sq_left))) ||
            (file < 7 && (pawns & (1ULL << sq_right)))) {
            score += EvalConstants::KNIGHT_OUTPOST_BONUS;
            }
    }
    
    return score;
}

// Endgame factors evaluation
int Evaluation::evaluate_endgame_factors(const Board& board) const {
    return evaluate_endgame_factors_for_color(board, Board::WHITE) - 
           evaluate_endgame_factors_for_color(board, Board::BLACK);
}

int Evaluation::evaluate_endgame_factors_for_color(const Board& board, Board::Color color) const {
    int score = 0;
    
    // King activity in endgame - use existing board function
    int king_square = board.get_king_position(color);
    int king_rank = king_square >> 3;  // Equivalent to king_square / 8
    int king_file = king_square & 7;   // Equivalent to king_square % 8
    
    // Centralization bonus
    int center_distance = max_int(abs_int(king_rank - 3), abs_int(king_file - 3));
    score += (4 - center_distance) * EvalConstants::KING_ACTIVITY_BONUS;
    
    // King near enemy pawns - use existing board functions
    Board::Color enemy_color = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;
    Bitboard enemy_pawns = board.get_piece_bitboard(Board::PAWN, enemy_color);
    uint64_t enemy_pawn_bits = enemy_pawns;
    while (enemy_pawn_bits) {
        int pawn_square = get_lsb(enemy_pawn_bits);
        enemy_pawn_bits &= enemy_pawn_bits - 1;
        
        int distance = distance_between_squares(king_square, pawn_square);
        if (distance < 3) {
            score += EvalConstants::KING_NEAR_ENEMY_PAWNS_BONUS;
        }
    }
    
    // Pawn endgame factors
    Bitboard pawns = board.get_piece_bitboard(Board::PAWN, color);
    uint64_t pawn_bits = pawns;
    while (pawn_bits) {
        int square = get_lsb(pawn_bits);
        pawn_bits &= pawn_bits - 1;
        
        // Connected passed pawns bonus
        if (is_passed_pawn(board, square, color)) {
            int file = square & 7;
            
            // Check for connected passed pawn
            for (int adj_file = file - 1; adj_file <= file + 1; adj_file += 2) {
                if (static_cast<unsigned>(adj_file) > 7) continue;
                Bitboard file_pawns = pawns & get_file_mask(adj_file);
                uint64_t file_pawn_bits = file_pawns;
                while (file_pawn_bits) {
                    int adj_square = get_lsb(file_pawn_bits);
                    file_pawn_bits &= file_pawn_bits - 1;

                    if (is_passed_pawn(board, adj_square, color)) {
                        score += EvalConstants::CONNECTED_PASSED_PAWNS_BONUS;
                    }
                }
            }
        }
    }
    
    return score;
}

// Development evaluation
int Evaluation::evaluate_development(const Board& board) const {
    int score = 0;
    
    // White development
    score += evaluate_development_for_color(board, Board::WHITE);
    
    // Black development
    score -= evaluate_development_for_color(board, Board::BLACK);
    
    return score;
}

int Evaluation::evaluate_development_for_color(const Board& board, Board::Color color) const {
    int score = 0;
    
    int back_rank = (color == Board::WHITE) ? 0 : 7;

    Bitboard knights = board.get_piece_bitboard(Board::KNIGHT, color);
    Bitboard bishops = board.get_piece_bitboard(Board::BISHOP, color);
    Bitboard queen = board.get_piece_bitboard(Board::QUEEN, color);

    int knight_start_squares[2] = {(back_rank << 3) | 1, (back_rank << 3) | 6};
    int bishop_start_squares[2] = {(back_rank << 3) | 2, (back_rank << 3) | 5};
    int queen_start_square = (back_rank << 3) | 3;

    // Knights
    for (int sq : knight_start_squares)
        if (!(knights & (1ULL << sq)))
            score += EvalConstants::PIECE_DEVELOPMENT_BONUS;

    // Bishops
    for (int sq : bishop_start_squares)
        if (!(bishops & (1ULL << sq)))
            score += EvalConstants::PIECE_DEVELOPMENT_BONUS;

    // Early queen development penalty
    if (!(queen & (1ULL << queen_start_square))) {
        int undeveloped_pieces = 0;
        for (int sq : knight_start_squares)
            if (knights & (1ULL << sq)) undeveloped_pieces++;
        for (int sq : bishop_start_squares)
            if (bishops & (1ULL << sq)) undeveloped_pieces++;

        if (undeveloped_pieces > 1)
            score += EvalConstants::EARLY_QUEEN_DEVELOPMENT_PENALTY;
    }
    
    // Evaluate pawn moves that limit piece development
    score += evaluate_development_limiting_pawn_penalties(board, color);

    return score;
}

// Evaluate pawn moves that limit piece development
int Evaluation::evaluate_development_limiting_pawn_penalties(const Board& board, Board::Color color) const {
    int score = 0;
    int back_rank = (color == Board::WHITE) ? 0 : 7;
    int pawn_rank = (color == Board::WHITE) ? 1 : 6;
    
    Bitboard pawns = board.get_piece_bitboard(Board::PAWN, color);
    Bitboard bishops = board.get_piece_bitboard(Board::BISHOP, color);
    Bitboard knights = board.get_piece_bitboard(Board::KNIGHT, color);
    
    // Check for pawns blocking bishop development
    // c-pawn blocking c1/c8 bishop
    int c_pawn_square = (pawn_rank << 3) | 2;  // c2 for white, c7 for black
    int c_bishop_square = (back_rank << 3) | 2;  // c1 for white, c8 for black
    if ((pawns & (1ULL << c_pawn_square)) && (bishops & (1ULL << c_bishop_square))) {
        score += EvalConstants::BISHOP_BLOCKING_PAWN_PENALTY;
    }
    
    // f-pawn blocking f1/f8 bishop
    int f_pawn_square = (pawn_rank << 3) | 5;  // f2 for white, f7 for black
    int f_bishop_square = (back_rank << 3) | 5;  // f1 for white, f8 for black
    if ((pawns & (1ULL << f_pawn_square)) && (bishops & (1ULL << f_bishop_square))) {
        score += EvalConstants::BISHOP_BLOCKING_PAWN_PENALTY;
    }
    
    // Check for pawns blocking knight development
    // b-pawn advanced too early blocking knight
    int b_knight_square = (back_rank << 3) | 1;  // b1 for white, b8 for black
    int b3_square = ((color == Board::WHITE) ? 2 : 5) << 3 | 1;  // b3 for white, b6 for black
    if ((pawns & (1ULL << b3_square)) && (knights & (1ULL << b_knight_square))) {
        score += EvalConstants::KNIGHT_BLOCKING_PAWN_PENALTY;
    }
    
    // g-pawn advanced too early blocking knight
    int g_knight_square = (back_rank << 3) | 6;  // g1 for white, g8 for black
    int g3_square = ((color == Board::WHITE) ? 2 : 5) << 3 | 6;  // g3 for white, g6 for black
    if ((pawns & (1ULL << g3_square)) && (knights & (1ULL << g_knight_square))) {
        score += EvalConstants::KNIGHT_BLOCKING_PAWN_PENALTY;
    }
    
    // Check for premature center pawn advances that limit development
    GamePhase phase = get_game_phase(board);
    if (phase == OPENING) {
        // Penalty for advancing center pawns too far too early
        int d4_square = ((color == Board::WHITE) ? 3 : 4) << 3 | 3;  // d4 for white, d5 for black
        int e4_square = ((color == Board::WHITE) ? 3 : 4) << 3 | 4;  // e4 for white, e5 for black
        
        // Count undeveloped pieces
        int undeveloped_count = 0;
        int knight_squares[2] = {(back_rank << 3) | 1, (back_rank << 3) | 6};
        int bishop_squares[2] = {(back_rank << 3) | 2, (back_rank << 3) | 5};
        
        for (int sq : knight_squares) {
            if (knights & (1ULL << sq)) undeveloped_count++;
        }
        for (int sq : bishop_squares) {
            if (bishops & (1ULL << sq)) undeveloped_count++;
        }
        
        // Penalty if center pawns are advanced while pieces are undeveloped
        if (undeveloped_count >= 2) {
            if (pawns & (1ULL << d4_square)) {
                score += EvalConstants::CENTER_PAWN_PREMATURE_ADVANCE_PENALTY;
            }
            if (pawns & (1ULL << e4_square)) {
                score += EvalConstants::CENTER_PAWN_PREMATURE_ADVANCE_PENALTY;
            }
        }
    }
    
    return score;
}

// Tapered evaluation
int Evaluation::tapered_eval(int opening_score, int endgame_score, int game_phase) const {
    return (opening_score * game_phase + endgame_score * (256 - game_phase)) / 256;
}

// Game phase detection
GamePhase Evaluation::get_game_phase(const Board& board) const {
    int phase_value = get_phase_value(board);
    
    if (phase_value > EvalConstants::TOTAL_PHASE * 2 / 3) {
        return OPENING;
    }
    if (phase_value > EvalConstants::TOTAL_PHASE / 3) {
        return MIDDLEGAME;
    }
    return ENDGAME;
}

// Get phase value
int Evaluation::get_phase_value(const Board& board) const {
    int phase_value = 0;
    
    for (int color = 0; color < 2; ++color) {
        for (int piece_type = 1; piece_type < 5; ++piece_type) { // Skip pawns and king
            Bitboard pieces = board.get_piece_bitboard(static_cast<Board::PieceType>(piece_type), 
                                                      static_cast<Board::Color>(color));
            int piece_count = count_bits(pieces);
            phase_value += piece_count * PHASE_VALUES[piece_type];
        }
    }
    
    return phase_value;
}

// Utility functions
void Evaluation::clear_pawn_hash_table() {
    pawn_hash_table.clear();
}

void Evaluation::print_evaluation_breakdown(const Board& board) {
    GamePhase phase = get_game_phase(board);
    int phase_value = get_phase_value(board);
    
    // Calculate all evaluation components
    int material_score = evaluate_material(board);
    int positional_score = evaluate_piece_square_tables(board);
    int pawn_score = evaluate_pawn_structure(board);
    int king_safety_score = evaluate_king_safety(board);
    int mobility_score = evaluate_mobility(board);
    int coordination_score = evaluate_piece_coordination(board);
    int development_score = (phase == OPENING) ? evaluate_development(board) : 0;
    int endgame_score = (phase == ENDGAME) ? evaluate_endgame_factors(board) : 0;
    
    // Calculate tempo bonus
    int tempo_bonus = 0;
    if (board.get_active_color() == Board::WHITE) {
        tempo_bonus = EvalConstants::TEMPO_BONUS;
    } else {
        tempo_bonus = -EvalConstants::TEMPO_BONUS;
    }
    
    std::cout << "=== Evaluation Breakdown ===" << std::endl;
    std::cout << "Material:     " << std::setw(6) << material_score << std::endl;
    std::cout << "Position:     " << std::setw(6) << positional_score << std::endl;
    std::cout << "Pawns:        " << std::setw(6) << pawn_score << std::endl;
    std::cout << "King Safety:  " << std::setw(6) << king_safety_score << std::endl;
    std::cout << "Mobility:     " << std::setw(6) << mobility_score << std::endl;
    std::cout << "Coordination: " << std::setw(6) << coordination_score << std::endl;
    
    if (phase == OPENING) {
        std::cout << "Development:  " << std::setw(6) << development_score << std::endl;
    }
    
    if (phase == ENDGAME) {
        std::cout << "Endgame:      " << std::setw(6) << endgame_score << std::endl;
    }
    
    std::cout << "Tempo Bonus:  " << std::setw(6) << tempo_bonus << std::endl;
    std::cout << "Phase:        " << (phase == OPENING ? "Opening" : 
                                phase == MIDDLEGAME ? "Middlegame" : "Endgame") << std::endl;
    std::cout << "Active Color: " << (board.get_active_color() == Board::WHITE ? "White" : "Black") << std::endl;
    std::cout << "------------------------------" << std::endl;
    std::cout << "Total:        " << std::setw(6) << evaluate(board) << std::endl;
    std::cout << "============================" << std::endl;
}

// Static helper functions
// Initialize passed pawn masks for efficient evaluation
void Evaluation::init_passed_pawn_masks() {
    if (pawn_masks_initialized) return;
    
    for (int sq = 0; sq < 64; ++sq) {
        int file = sq & 7;
        int rank = sq >> 3;

        for (int color = 0; color < 2; ++color) {
            Bitboard mask = 0ULL;
            int direction = (color == Board::WHITE) ? 8 : -8;
            int limit_rank = (color == Board::WHITE) ? 7 : 0;

            for (int f = max_int(0, file - 1); f <= min_int(7, file + 1); ++f) {
                for (int r = rank + (color == Board::WHITE ? 1 : -1);
                     (color == Board::WHITE ? r <= limit_rank : r >= limit_rank);
                     r += (color == Board::WHITE ? 1 : -1)) {
                    mask |= 1ULL << (r * 8 + f);
                }
            }

            passed_pawn_masks[sq][color] = mask;
        }
    }
}

// Initialize pawn evaluation masks
void Evaluation::init_pawn_masks() {
    if (pawn_masks_initialized) return;
    
    init_passed_pawn_masks(); // Initialize passed pawn masks
    // Initialize file masks
    for (int file = 0; file < 8; ++file) {
        file_masks[file] = 0ULL;
        for (int rank = 0; rank < 8; ++rank) {
            file_masks[file] |= (1ULL << (rank * 8 + file));
        }
    }
    
    // Initialize isolated pawn masks (adjacent files)
    for (int square = 0; square < 64; ++square) {
        int file = square & 7;
        isolated_pawn_masks[square] = 0ULL;
        
        // Add left adjacent file
        if (file > 0) {
            isolated_pawn_masks[square] |= file_masks[file - 1];
        }
        
        // Add right adjacent file
        if (file < 7) {
            isolated_pawn_masks[square] |= file_masks[file + 1];
        }
    }
    
    pawn_masks_initialized = true;
}

bool Evaluation::is_passed_pawn(const Board& board, int square, Board::Color color) const {
    Board::Color enemy = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;
    Bitboard enemy_pawns = board.get_piece_bitboard(Board::PAWN, enemy);
    return !(enemy_pawns & passed_pawn_masks[square][color]);
}

bool Evaluation::is_isolated_pawn(const Board& board, int square, Board::Color color) {
    Bitboard own_pawns = board.get_piece_bitboard(Board::PAWN, color);
    // Use precomputed mask to check adjacent files for friendly pawns
    return !(own_pawns & isolated_pawn_masks[square]);
}

bool Evaluation::is_doubled_pawn(const Board& board, int square, Board::Color color) {
    int file = square & 7; // Equivalent to square % 8
    Bitboard own_pawns = board.get_piece_bitboard(Board::PAWN, color);
    
    // Check if there are multiple pawns on the same file
    Bitboard file_pawns = own_pawns & file_masks[file];
    
    // Remove current pawn and check if any others remain
    file_pawns &= ~(1ULL << square);
    return file_pawns != 0;
}

// Helper function to count attackers to king zone
int Evaluation::count_attackers_to_king_zone(const Board& board, Board::Color attacking_color, Board::Color king_color) const {
    Bitboard king = board.get_piece_bitboard(Board::KING, king_color);
    if (!king) return 0;
    
    int king_square = get_lsb(king);
    int attackers = 0;
    
    // Check each piece type for attacks to king zone
    for (int piece_type = Board::PAWN; piece_type <= Board::QUEEN; ++piece_type) {
        Bitboard pieces = board.get_piece_bitboard(static_cast<Board::PieceType>(piece_type), attacking_color);
        uint64_t piece_bits = pieces;
        
        while (piece_bits) {
            int square = get_lsb(piece_bits);
            piece_bits &= piece_bits - 1;
            
            if (is_in_king_zone(square, king_square)) {
                attackers++;
            }
        }
    }
    
    return attackers;
}

// Helper function to check if file is open
bool Evaluation::is_file_open(const Board& board, int file) const {
    Bitboard file_mask = get_file_mask(file);
    Bitboard white_pawns = board.get_piece_bitboard(Board::PAWN, Board::WHITE);
    Bitboard black_pawns = board.get_piece_bitboard(Board::PAWN, Board::BLACK);
    
    return !(file_mask & (white_pawns | black_pawns));
}

// Helper function to check if file is semi-open for a color
bool Evaluation::is_file_semi_open(const Board& board, int file, Board::Color color) const {
    Bitboard file_mask = get_file_mask(file);
    Bitboard pawns = board.get_piece_bitboard(Board::PAWN, color);
    
    return !(file_mask & pawns);
}

// Helper function to check if a pawn is backward
bool Evaluation::is_backward_pawn(const Board& board, int square, Board::Color color) {
    int rank = square >> 3; // Equivalent to square / 8
    int file = square & 7;  // Equivalent to square % 8
    
    Bitboard own_pawns = board.get_piece_bitboard(Board::PAWN, color);
    
    // Create mask for adjacent files at or behind current rank
    Bitboard support_mask = 0ULL;

    /* TODO: check if this optimization is viable
    For White: support_mask |= left_file & ((1ULL << (rank * 8)) - 1);

    For Black: support_mask |= left_file & (~((1ULL << (rank * 8)) - 1));
    */

    // Check left adjacent file
    if (file > 0) {
        Bitboard left_file = file_masks[file - 1];
        if (color == Board::WHITE) {
            // For white, supporting pawns must be at same rank or behind (lower ranks)
            for (int r = 0; r <= rank; ++r) {
                support_mask |= left_file & (0xFFULL << (r * 8));
            }
        } else {
            // For black, supporting pawns must be at same rank or behind (higher ranks)
            for (int r = rank; r < 8; ++r) {
                support_mask |= left_file & (0xFFULL << (r * 8));
            }
        }
    }
    
    // Check right adjacent file
    if (file < 7) {
        Bitboard right_file = file_masks[file + 1];
        if (color == Board::WHITE) {
            for (int r = 0; r <= rank; ++r) {
                support_mask |= right_file & (0xFFULL << (r * 8));
            }
        } else {
            for (int r = rank; r < 8; ++r) {
                support_mask |= right_file & (0xFFULL << (r * 8));
            }
        }
    }
    
    // Check if there are any supporting pawns
    return !(own_pawns & support_mask);
}

// Helper function to check if pawns form a chain
bool Evaluation::is_pawn_chain(const Board& board, int square, Board::Color color) {
    int rank = square >> 3; // Equivalent to square / 8
    int file = square & 7;  // Equivalent to square % 8
    
    // Check for diagonal support
    int support_rank = (color == Board::WHITE) ? rank - 1 : rank + 1;
    if (static_cast<unsigned>(support_rank) >= 8) return false;
    
    Bitboard own_pawns = board.get_piece_bitboard(Board::PAWN, color);
    
    // Check left diagonal support
    if (file > 0) {
        int support_square = (support_rank << 3) | (file - 1); // support_rank * 8 + (file - 1)
        if (own_pawns & (1ULL << support_square)) {
            return true;
        }
    }
    
    // Check right diagonal support
    if (file < 7) {
        int support_square = (support_rank << 3) | (file + 1); // support_rank * 8 + (file + 1)
        if (own_pawns & (1ULL << support_square)) {
            return true;
        }
    }
    
    return false;
}

// Helper function to get passed pawn rank bonus
int Evaluation::get_passed_pawn_rank_bonus(int square, Board::Color color) {
    int rank = square >> 3;
    int relative_rank = (color == Board::WHITE) ? rank : (7 - rank);
    return (EvalConstants::ADVANCED_PASSED_PAWN_BONUS * relative_rank * relative_rank) >> 4;
}

// Helper function to calculate distance between squares
int Evaluation::distance_between_squares(int sq1, int sq2) {
    int rank1 = sq1 >> 3, file1 = sq1 & 7;
    int rank2 = sq2 >> 3, file2 = sq2 & 7;
    
    return max_int(abs_int(rank1 - rank2), abs_int(file1 - file2));
}

// Helper function to check if a square is in king zone
bool Evaluation::is_in_king_zone(int square, int king_square) {
    return distance_between_squares(square, king_square) < 3;
}