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

        score += side_multiplier * get_piece_square_value(piece_type, color, square, phase);
    }

    return score;
}


// Get piece-square value for a specific piece
int Evaluation::get_piece_square_value(Board::PieceType piece_type, Board::Color color, int square, GamePhase phase) const {
    // For kings in endgame, use special endgame table
    if (piece_type == Board::KING && phase == ENDGAME) {
        return color == Board::WHITE ? KING_ENDGAME_PST[square] : KING_ENDGAME_PST[mirror_square(square)];
    }
    
    return PST[color][piece_type][square];
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
        
        // Check for isolated pawns
        if (is_isolated_pawn(board, square, color)) {
            score += EvalConstants::ISOLATED_PAWN_PENALTY;
        }
        
        // Check for doubled pawns
        if (is_doubled_pawn(board, square, color)) {
            score += EvalConstants::DOUBLED_PAWN_PENALTY;
        }
        
        // Check for backward pawns
        if (is_backward_pawn(board, square, color)) {
            score += EvalConstants::BACKWARD_PAWN_PENALTY;
        }

        // TODO: (BUGFIX) Returns true in false cases
        // Check for passed pawns
        // if (is_passed_pawn(board, square, color)) {
        //     score += EvalConstants::PASSED_PAWN_BONUS;
        //     score += get_passed_pawn_rank_bonus(square, color);
        //     // std::cout << "Passed pawn " << score << std::endl;
        // }
        
        // Check for pawn chains
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

// King safety evaluation for one color
int Evaluation::evaluate_king_safety_for_color(const Board& board, Board::Color color) const {
    int score = 0;
    int king_pos = board.get_king_position(color);
    int king_file = king_pos & 7;
    Board::Color enemy_color = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;

    // Evaluate files around the king
    for (int file_offset = -1; file_offset < 2; ++file_offset) {
        int check_file = king_file + file_offset;
        if (static_cast<unsigned>(check_file) > 7) continue;
        if (is_file_open(board, check_file)) {
            score += EvalConstants::OPEN_FILE_NEAR_KING_PENALTY;
        } else if (is_file_semi_open(board, check_file, color)) {
            score += EvalConstants::SEMI_OPEN_FILE_NEAR_KING_PENALTY;
        }
    }
    
    // Count attackers in king zone
    int attackers = count_attackers_to_king_zone(board, enemy_color, color);
    score -= attackers * EvalConstants::KING_ZONE_ATTACK_BONUS;
    
    // Bonus for castling (if king is on castled square)
    int king_rank = king_pos >> 3;
    int expected_rank = (color == Board::WHITE) ? 0 : 7;
    if (king_rank == expected_rank && (king_file == 2 || king_file == 6)) {
        score += EvalConstants::CASTLING_BONUS;
    }
    
    return score;
}

// Mobility evaluation
int Evaluation::evaluate_mobility(const Board& board) const {
    int score = 0;
    score += evaluate_mobility_for_color(board, Board::WHITE);
    score -= evaluate_mobility_for_color(board, Board::BLACK);
    return score;
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