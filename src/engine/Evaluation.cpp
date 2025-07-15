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

//        std::cout << color << std::endl;
        // Check for isolated pawns - reduced penalty in opening
        if (is_isolated_pawn(board, square, color)) {
            score += EvalConstants::ISOLATED_PAWN_PENALTY;
//            std::cout << "Isolated pawn " << score << " at: " << rank << " " << file << std::endl;
        }
        
        // Check for doubled pawns
        if (is_doubled_pawn(board, square, color)) {
            score += EvalConstants::DOUBLED_PAWN_PENALTY;
//            std::cout << "Doubled pawn " << score << " at: " << rank << " " << file << std::endl;
        }
        
        // Check for backward pawns
        if (is_backward_pawn(board, square, color)) {
            score += EvalConstants::BACKWARD_PAWN_PENALTY;
//            std::cout << "Backward pawn " << score << " at: " << rank << " " << file << std::endl;
        }

        // Check for passed pawns
         if (is_passed_pawn(board, square, color)) {
             score += EvalConstants::PASSED_PAWN_BONUS;
             score += get_passed_pawn_rank_bonus(square, color);
//             std::cout << "Passed pawn " << score << " at: " << rank << " " << file << std::endl;
         }
        
        // Check for pawn chains
        if (is_pawn_chain(board, square, color)) {
            score += EvalConstants::PAWN_CHAIN_BONUS;
//            std::cout << "Pawn chain " << score << " at: " << rank << " " << file << std::endl;
        }

        // Connected pawns (left/right on same rank)
        for (int df = -1; df <= 1; df += 2) {
            int adjacent_file = file + df;
            if (static_cast<unsigned>(adjacent_file) > 7) continue;
            int adjacent_square = (rank << 3) | adjacent_file; // rank * 8 + adjacent_file
            if (pawns & (1ULL << adjacent_square)) {
                score += EvalConstants::CONNECTED_PAWNS_BONUS;
//                std::cout << "Connected pawn " << score << " at: " << rank << " " << file << std::endl;
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
    
    // Basic file safety around king - optimized loop
    int start_file = max_int(0, king_file - 1);
    int end_file = min_int(7, king_file + 1);
    
    for (int check_file = start_file; check_file <= end_file; ++check_file) {
        if (is_file_open(board, check_file)) {
            score += EvalConstants::OPEN_FILE_NEAR_KING_PENALTY;
            // Extra penalty if king is directly on open file
            if (check_file == king_file) {
                score += EvalConstants::KING_ON_OPEN_FILE_PENALTY;
            }
        } else if (is_file_semi_open(board, check_file, color)) {
            score += EvalConstants::SEMI_OPEN_FILE_NEAR_KING_PENALTY;
        }
    }

    // Comprehensive king safety evaluation - call order optimized for early returns
    score += evaluate_pawn_shield(board, color);
    score += evaluate_king_exposure(board, color);
    score += evaluate_castling_safety(board, color);
    score += evaluate_king_attackers(board, color);
    score += evaluate_king_tropism(board, color);
    score += evaluate_back_rank_safety(board, color);
    score += evaluate_king_escape_squares(board, color);
    score += evaluate_tactical_threats_to_king(board, color);
    score += evaluate_king_safety_pawn_penalties(board, color);

    // King activity penalty in middlegame - refined logic
    if (phase == MIDDLEGAME && !is_castled) {
        int distance_from_back_rank = abs_int(king_rank - expected_rank);
        score += EvalConstants::KING_ACTIVITY_PENALTY * distance_from_back_rank;
    }

    return score;
}

// Evaluate pawn moves that damage king safety
int Evaluation::evaluate_king_safety_pawn_penalties(const Board& board, Board::Color color) const {
    int score = 0;
    int king_pos = board.get_king_position(color);
    int king_file = king_pos & 7;
    int king_rank = king_pos >> 3;
    
    Bitboard pawns = board.get_piece_bitboard(Board::PAWN, color);
    
    // Check for weakened king shelter (missing or advanced pawns in front of king)
    for (int file_offset = -1; file_offset <= 1; ++file_offset) {
        int check_file = king_file + file_offset;
        if (static_cast<unsigned>(check_file) > 7) continue;
        
        bool has_shelter_pawn = false;
        int shelter_rank = (color == Board::WHITE) ? king_rank + 1 : king_rank - 1;
        
        // Check if there's a pawn providing shelter
        if (shelter_rank >= 0 && shelter_rank < 8) {
            int shelter_square = (shelter_rank << 3) | check_file;
            if (pawns & (1ULL << shelter_square)) {
                has_shelter_pawn = true;
            }
        }
        
        // Penalty for missing king shelter
        if (!has_shelter_pawn) {
            score += EvalConstants::WEAKENED_KING_SHELTER_PENALTY;
        }
        
        // Check for advanced pawns that weaken king safety
        int advanced_rank = (color == Board::WHITE) ? king_rank + 2 : king_rank - 2;
        if (advanced_rank >= 0 && advanced_rank < 8) {
            int advanced_square = (advanced_rank << 3) | check_file;
            if (pawns & (1ULL << advanced_square)) {
                score += EvalConstants::PAWN_STORM_AGAINST_KING_PENALTY;
            }
        }
    }
    
    return score;
}

// Evaluate king exposure (weak squares, missing pieces) - optimized
int Evaluation::evaluate_king_exposure(const Board& board, Board::Color color) const {
    int score = 0;
    int king_pos = board.get_king_position(color);
    int king_file = king_pos & 7;
    int king_rank = king_pos >> 3;
    int expected_rank = (color == Board::WHITE) ? 0 : 7;
    GamePhase phase = get_game_phase(board);
    
    // Check for fianchetto bishop protection - optimized with bitboard masks
    Bitboard bishops = board.get_piece_bitboard(Board::BISHOP, color);
    bool has_fianchetto_protection = false;
    
    if (color == Board::WHITE) {
        // White fianchetto squares: b1(1), g1(6)
        Bitboard white_fianchetto_mask = (1ULL << 1) | (1ULL << 6);
        if (bishops & white_fianchetto_mask) {
            // Check if fianchetto bishop is protecting king area
            if ((king_file >= 0 && king_file <= 3 && (bishops & (1ULL << 1))) ||
                (king_file >= 4 && king_file <= 7 && (bishops & (1ULL << 6)))) {
                has_fianchetto_protection = true;
                score += EvalConstants::FIANCHETTO_BONUS;
            }
        }
    } else {
        // Black fianchetto squares: b8(57), g8(62)
        Bitboard black_fianchetto_mask = (1ULL << 57) | (1ULL << 62);
        if (bishops & black_fianchetto_mask) {
            // Check if fianchetto bishop is protecting king area
            if ((king_file >= 0 && king_file <= 3 && (bishops & (1ULL << 57))) ||
                (king_file >= 4 && king_file <= 7 && (bishops & (1ULL << 62)))) {
                has_fianchetto_protection = true;
                score += EvalConstants::FIANCHETTO_BONUS;
            }
        }
    }
    
    // Penalty for missing fianchetto bishop when king is castled on fianchetto side
    bool is_fianchetto_castled = (king_rank == expected_rank && (king_file == 1 || king_file == 6));
    if (is_fianchetto_castled && !has_fianchetto_protection) {
        score += EvalConstants::MISSING_FIANCHETTO_BISHOP_PENALTY;
    }
    
    // King corner safety bonus - refined conditions
    bool is_in_corner = (king_rank == expected_rank && (king_file <= 2 || king_file >= 5));
    if (is_in_corner) {
        score += EvalConstants::KING_CORNER_SAFETY_BONUS;
        // Additional bonus if king is well-castled
        if (king_file == 2 || king_file == 6) {
            score += EvalConstants::KING_CORNER_SAFETY_BONUS / 2;
        }
    }
    
    // Check for exposed king - more nuanced evaluation
    if (phase == MIDDLEGAME || phase == OPENING) {
        if (king_rank != expected_rank) {
            // Penalty increases with distance from back rank and game phase
            int exposure_penalty = EvalConstants::KING_EXPOSED_PENALTY;
            int distance_penalty = abs_int(king_rank - expected_rank);
            score += exposure_penalty + (distance_penalty * 5);
        }
        
        // Additional penalty for king in center files during opening/middlegame
        if ((phase == OPENING || phase == MIDDLEGAME) && king_file >= 3 && king_file <= 4) {
            score += EvalConstants::KING_EXPOSED_PENALTY / 2;
        }
    }
    
    return score;
}

// Evaluate attackers targeting the king - optimized
int Evaluation::evaluate_king_attackers(const Board& board, Board::Color color) const {
    int score = 0;
    Board::Color enemy_color = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;
    int king_pos = board.get_king_position(color);
    int king_file = king_pos & 7;
    int king_rank = king_pos >> 3;
    
    // Count attackers in king zone - optimized
    int attackers = count_attackers_to_king_zone(board, enemy_color, color);
    
    // Progressive penalty for multiple attackers
    if (attackers > 0) {
        score -= attackers * EvalConstants::KING_ZONE_ATTACK_BONUS;
        
        // Exponential penalty for multiple attackers
        if (attackers >= 2) {
            score += EvalConstants::MULTIPLE_ATTACKERS_PENALTY;
            if (attackers >= 4) {
                score += EvalConstants::MULTIPLE_ATTACKERS_PENALTY; // Double penalty for 4+ attackers
            }
        }
    }
    
    // Enemy queen proximity - more detailed evaluation
    Bitboard enemy_queen = board.get_piece_bitboard(Board::QUEEN, enemy_color);
    if (enemy_queen) {
        int queen_square = get_lsb(enemy_queen);
        int distance = distance_between_squares(king_pos, queen_square);
        
        if (distance <= 4) {
            // Penalty increases as queen gets closer
            int proximity_penalty = EvalConstants::ENEMY_QUEEN_NEAR_KING_PENALTY * (5 - distance) / 4;
            score += proximity_penalty;
            
            // Extra penalty if queen is on same rank/file as king
            int queen_file = queen_square & 7;
            int queen_rank = queen_square >> 3;
            if (queen_file == king_file || queen_rank == king_rank) {
                score += EvalConstants::ENEMY_QUEEN_NEAR_KING_PENALTY / 2;
            }
        }
    }
    
    // Check for enemy rooks on same file/rank as king
    Bitboard enemy_rooks = board.get_piece_bitboard(Board::ROOK, enemy_color);
    uint64_t rook_bits = enemy_rooks;
    while (rook_bits) {
        int rook_square = get_lsb(rook_bits);
        rook_bits &= rook_bits - 1;
        
        int rook_file = rook_square & 7;
        int rook_rank = rook_square >> 3;
        
        // Penalty for rook on same file/rank as king
        if (rook_file == king_file || rook_rank == king_rank) {
            score += EvalConstants::KING_ZONE_ATTACK_BONUS;
        }
    }
    
    return score;
}

// Evaluate king tropism (enemy pieces close to king) - optimized
int Evaluation::evaluate_king_tropism(const Board& board, Board::Color color) const {
    int score = 0;
    Board::Color enemy_color = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;
    int king_pos = board.get_king_position(color);
    
    // Piece-specific tropism weights (more dangerous pieces get higher weights)
    static const int tropism_weights[] = {0, 0, 3, 2, 4, 6}; // PAWN, KNIGHT, BISHOP, ROOK, QUEEN
    static const int max_tropism_distance = 4;
    
    // Check distance of enemy pieces to king - optimized with piece-specific weights
    for (int piece_type = Board::KNIGHT; piece_type <= Board::QUEEN; ++piece_type) {
        Bitboard pieces = board.get_piece_bitboard(static_cast<Board::PieceType>(piece_type), enemy_color);
        
        if (!pieces) continue; // Skip if no pieces of this type
        
        uint64_t piece_bits = pieces;
        int piece_weight = tropism_weights[piece_type];
        
        while (piece_bits) {
            int square = get_lsb(piece_bits);
            piece_bits &= piece_bits - 1;
            
            int distance = distance_between_squares(king_pos, square);
            if (distance <= max_tropism_distance) {
                // More sophisticated penalty calculation
                int base_penalty = EvalConstants::KING_TROPISM_PENALTY;
                int distance_multiplier = (max_tropism_distance + 1 - distance);
                int piece_penalty = (base_penalty * piece_weight * distance_multiplier) / 4;
                score += piece_penalty;
            }
        }
    }
    
    // Special case: enemy pawns near king (different evaluation)
    Bitboard enemy_pawns = board.get_piece_bitboard(Board::PAWN, enemy_color);
    uint64_t pawn_bits = enemy_pawns;
    while (pawn_bits) {
        int pawn_square = get_lsb(pawn_bits);
        pawn_bits &= pawn_bits - 1;
        
        int distance = distance_between_squares(king_pos, pawn_square);
        if (distance <= 2) {
            // Pawns are less dangerous but still contribute to king pressure
            score += EvalConstants::KING_TROPISM_PENALTY * (3 - distance) / 2;
        }
    }
    
    return score;
}

// Evaluate pawn shield around king - optimized
int Evaluation::evaluate_pawn_shield(const Board& board, Board::Color color) const {
    int score = 0;
    int king_pos = board.get_king_position(color);
    int king_file = king_pos & 7;
    int king_rank = king_pos >> 3;
    
    Bitboard pawns = board.get_piece_bitboard(Board::PAWN, color);
    
    // Optimized file range calculation
    int start_file = std::max(0, king_file - 1);
    int end_file = std::min(7, king_file + 1);
    
    // Direction for pawn advancement
    int pawn_direction = (color == Board::WHITE) ? 1 : -1;
    
    // Check pawn shield in front of king - optimized with bitboard masks
    for (int check_file = start_file; check_file <= end_file; ++check_file) {
        bool found_shield = false;
        
        // Check for pawn shield at different distances (closer is better)
        for (int rank_offset = 1; rank_offset <= 3; ++rank_offset) {
            int shield_rank = king_rank + (rank_offset * pawn_direction);
            
            // Bounds check
            if (shield_rank < 0 || shield_rank > 7) break;
            
            int shield_square = (shield_rank << 3) | check_file;
            if (pawns & (1ULL << shield_square)) {
                // Bonus decreases with distance, but file position matters too
                int base_bonus = EvalConstants::PAWN_SHIELD_BONUS;
                int distance_factor = 4 - rank_offset; // Closer pawns are more valuable
                int file_factor = (check_file == king_file) ? 2 : 1; // Direct file protection is more valuable
                
                score += (base_bonus * distance_factor * file_factor) / 3;
                found_shield = true;
                break; // Found shield pawn, no need to check further on this file
            }
        }
        
        // Penalty for missing pawn shield on important files
        if (!found_shield) {
            // More penalty for missing shield directly in front of king
            int missing_penalty = (check_file == king_file) ? -8 : -4;
            score += missing_penalty;
        }
    }
    
    // Additional evaluation: check for advanced enemy pawns near king
    Board::Color enemy_color = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;
    Bitboard enemy_pawns = board.get_piece_bitboard(Board::PAWN, enemy_color);
    int enemy_direction = -pawn_direction;
    
    // Check for enemy pawn storm
    for (int check_file = start_file; check_file <= end_file; ++check_file) {
        for (int rank_offset = 1; rank_offset <= 3; ++rank_offset) {
            int storm_rank = king_rank + (rank_offset * enemy_direction);
            
            if (storm_rank < 0 || storm_rank > 7) break;
            
            int storm_square = (storm_rank << 3) | check_file;
            if (enemy_pawns & (1ULL << storm_square)) {
                // Penalty for enemy pawns approaching king
                int storm_penalty = -EvalConstants::PAWN_SHIELD_BONUS / (rank_offset + 1);
                score += storm_penalty;
                break;
            }
        }
    }
    
    return score;
}

// Evaluate castling safety - optimized
int Evaluation::evaluate_castling_safety(const Board& board, Board::Color color) const {
    int score = 0;
    int king_pos = board.get_king_position(color);
    int king_file = king_pos & 7;
    int king_rank = king_pos >> 3;
    int expected_rank = (color == Board::WHITE) ? 0 : 7;
    
    // Check if king has castled
    bool has_castled_kingside = (king_rank == expected_rank && king_file == 6);
    bool has_castled_queenside = (king_rank == expected_rank && king_file == 2);
    bool has_castled = has_castled_kingside || has_castled_queenside;
    
    if (has_castled) {
        // Base castling bonus
        score += EvalConstants::CASTLING_BONUS;
        
        // Kingside castling is generally safer
        if (has_castled_kingside) {
            score += EvalConstants::CASTLING_BONUS / 3;
            
            // Check if rook is still protecting after kingside castling
            int rook_square = (expected_rank << 3) | 5; // f1/f8
            Bitboard rooks = board.get_piece_bitboard(Board::ROOK, color);
            if (rooks & (1ULL << rook_square)) {
                score += 5; // Bonus for rook protection
            }
        } else {
            // Queenside castling - slightly less safe but still good
            score += EvalConstants::CASTLING_BONUS / 4;
            
            // Check rook protection after queenside castling
            int rook_square = (expected_rank << 3) | 3; // d1/d8
            Bitboard rooks = board.get_piece_bitboard(Board::ROOK, color);
            if (rooks & (1ULL << rook_square)) {
                score += 3;
            }
        }
        
        // Evaluate pawn shield quality after castling
        Bitboard pawns = board.get_piece_bitboard(Board::PAWN, color);
        int shield_files[] = {king_file - 1, king_file, king_file + 1};
        int shield_count = 0;
        
        for (int i = 0; i < 3; ++i) {
            int file = shield_files[i];
            if (file >= 0 && file <= 7) {
                int pawn_rank = expected_rank + ((color == Board::WHITE) ? 1 : -1);
                if (pawn_rank >= 0 && pawn_rank <= 7) {
                    int pawn_square = (pawn_rank << 3) | file;
                    if (pawns & (1ULL << pawn_square)) {
                        shield_count++;
                    }
                }
            }
        }
        score += shield_count * 4; // Bonus for pawn shield
        
    } else if (king_rank == expected_rank && king_file == 4) {
        // King on starting square - has castling rights
        score += EvalConstants::CASTLING_RIGHTS_BONUS;
        
        // Check if castling is still viable (rooks in place)
        Bitboard rooks = board.get_piece_bitboard(Board::ROOK, color);
        bool kingside_rook = rooks & (1ULL << ((expected_rank << 3) | 7));
        bool queenside_rook = rooks & (1ULL << ((expected_rank << 3) | 0));
        
        if (kingside_rook) score += 3;
        if (queenside_rook) score += 2;
        
        // Penalty for delaying castling in middlegame
        int total_pieces = 0;
        for (int c = 0; c < 2; ++c) {
            for (int pt = 0; pt < 6; ++pt) {
                Bitboard pieces = board.get_piece_bitboard(static_cast<Board::PieceType>(pt), 
                                                          static_cast<Board::Color>(c));
                total_pieces += count_bits(pieces);
            }
        }
        if (total_pieces < 20) { // Middlegame
            score -= 8; // Encourage castling
        }
        
    } else {
        // King has moved but not castled
        score -= EvalConstants::CASTLING_BONUS / 2;
        
        // Additional penalty for king in center
        if (king_file >= 3 && king_file <= 5) {
            score -= 12;
        }
        
        // Penalty for king advancement in opening/middlegame
        int total_pieces = 0;
        for (int c = 0; c < 2; ++c) {
            for (int pt = 0; pt < 6; ++pt) {
                Bitboard pieces = board.get_piece_bitboard(static_cast<Board::PieceType>(pt), 
                                                          static_cast<Board::Color>(c));
                total_pieces += count_bits(pieces);
            }
        }
        if (total_pieces > 16) { // Still early in game
            int rank_penalty = (color == Board::WHITE) ? 
                std::max(0, king_rank - expected_rank) : 
                std::max(0, expected_rank - king_rank);
            score -= rank_penalty * 6;
        }
    }
    
    return score;
}

// Evaluate back rank safety - optimized
int Evaluation::evaluate_back_rank_safety(const Board& board, Board::Color color) const {
    int score = 0;
    int king_pos = board.get_king_position(color);
    int king_file = king_pos & 7;
    int king_rank = king_pos >> 3;
    int expected_rank = (color == Board::WHITE) ? 0 : 7;
    Board::Color enemy_color = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;
    
    // Only evaluate if king is on back rank
    if (king_rank != expected_rank) {
        return score; // King not on back rank, no back rank weakness
    }
    
    // Get enemy attacking pieces
    Bitboard enemy_rooks = board.get_piece_bitboard(Board::ROOK, enemy_color);
    Bitboard enemy_queens = board.get_piece_bitboard(Board::QUEEN, enemy_color);
    Bitboard back_rank_attackers = enemy_rooks | enemy_queens;
    
    // Early return if no potential attackers
    if (!back_rank_attackers) {
        return score;
    }
    
    // Check for back rank mate threats
    bool has_back_rank_threat = false;
    
    // Check if any enemy rook/queen can attack the back rank
    uint64_t attacker_bits = back_rank_attackers;
    while (attacker_bits) {
        int square = get_lsb(attacker_bits);
        attacker_bits &= attacker_bits - 1;
        
        int piece_file = square & 7;
        int piece_rank = square >> 3;
        
        // Check if piece can attack king's rank (same rank or file)
        if (piece_rank == expected_rank || piece_file == king_file) {
            has_back_rank_threat = true;
            break;
        }
    }
    
    if (!has_back_rank_threat) {
        return score;
    }
    
    // Evaluate escape squares more efficiently
    Bitboard own_pieces = board.get_color_bitboard(color);
    Bitboard enemy_pieces = board.get_color_bitboard(enemy_color);
    Bitboard all_pieces = own_pieces | enemy_pieces;
    
    int escape_rank = (color == Board::WHITE) ? 1 : 6;
    int escape_squares = 0;
    int blocked_squares = 0;
    
    // Check escape squares in front of king
    for (int file_offset = -1; file_offset <= 1; ++file_offset) {
        int escape_file = king_file + file_offset;
        if (escape_file < 0 || escape_file > 7) continue;
        
        int escape_square = (escape_rank << 3) | escape_file;
        
        if (!(own_pieces & (1ULL << escape_square))) {
            escape_squares++;
        } else {
            blocked_squares++;
        }
    }
    
    // Calculate penalty based on escape squares availability
    if (escape_squares == 0) {
        // No escape squares - severe back rank weakness
        score += EvalConstants::BACK_RANK_WEAKNESS_PENALTY;
        
        // Additional penalty if all squares are blocked by own pieces
        if (blocked_squares == 3) {
            score += EvalConstants::BACK_RANK_WEAKNESS_PENALTY / 2;
        }
    } else if (escape_squares == 1) {
        // Only one escape square - moderate weakness
        score += EvalConstants::BACK_RANK_WEAKNESS_PENALTY / 2;
    } else if (escape_squares == 2) {
        // Two escape squares - minor weakness
        score += EvalConstants::BACK_RANK_WEAKNESS_PENALTY / 4;
    }
    // Three escape squares = no penalty
    
    // Additional evaluation: check for defending pieces
    Bitboard own_rooks = board.get_piece_bitboard(Board::ROOK, color);
    Bitboard own_queens = board.get_piece_bitboard(Board::QUEEN, color);
    Bitboard defenders = own_rooks | own_queens;
    
    // Check if we have pieces defending the back rank
    uint64_t defender_bits = defenders;
    bool has_back_rank_defender = false;
    
    while (defender_bits) {
        int square = get_lsb(defender_bits);
        defender_bits &= defender_bits - 1;
        
        int piece_rank = square >> 3;
        if (piece_rank == expected_rank) {
            has_back_rank_defender = true;
            break;
        }
    }
    
    // Bonus for having defenders on back rank
    if (has_back_rank_defender) {
        score -= EvalConstants::BACK_RANK_WEAKNESS_PENALTY / 3;
    }
    
    return score;
}

// Evaluate king escape squares - optimized
int Evaluation::evaluate_king_escape_squares(const Board& board, Board::Color color) const {
    int score = 0;
    int king_pos = board.get_king_position(color);
    int king_file = king_pos & 7;
    int king_rank = king_pos >> 3;
    
    Bitboard own_pieces = board.get_color_bitboard(color);
    Bitboard enemy_pieces = board.get_color_bitboard((color == Board::WHITE) ? Board::BLACK : Board::WHITE);
    Bitboard all_pieces = own_pieces | enemy_pieces;
    
    int escape_squares = 0;
    int safe_escape_squares = 0;
    int attacked_escape_squares = 0;
    
    // Pre-calculate king movement offsets for efficiency
    static const int king_offsets[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},           {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    
    // Check all adjacent squares more efficiently
    for (int i = 0; i < 8; ++i) {
        int new_rank = king_rank + king_offsets[i][0];
        int new_file = king_file + king_offsets[i][1];
        
        // Bounds check
        if (new_rank < 0 || new_rank > 7 || new_file < 0 || new_file > 7) {
            continue;
        }
        
        int escape_square = (new_rank << 3) | new_file;
        
        // Check if square is not occupied by own piece
        if (!(own_pieces & (1ULL << escape_square))) {
            escape_squares++;
            
            // Additional evaluation: check if escape square is safe
            bool is_safe = true;
            
            // Quick check for enemy piece occupation
            if (enemy_pieces & (1ULL << escape_square)) {
                is_safe = false; // Enemy piece on square
            } else {
                // Check if square is attacked by enemy pieces (simplified)
                // This is a basic check - a full implementation would use attack tables
                
                // Check for enemy pawn attacks
                Board::Color enemy_color = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;
                int pawn_attack_rank = new_rank + ((enemy_color == Board::WHITE) ? 1 : -1);
                
                if (pawn_attack_rank >= 0 && pawn_attack_rank <= 7) {
                    Bitboard enemy_pawns = board.get_piece_bitboard(Board::PAWN, enemy_color);
                    
                    // Check diagonal pawn attacks
                    for (int pawn_file_offset = -1; pawn_file_offset <= 1; pawn_file_offset += 2) {
                        int pawn_file = new_file + pawn_file_offset;
                        if (pawn_file >= 0 && pawn_file <= 7) {
                            int pawn_square = (pawn_attack_rank << 3) | pawn_file;
                            if (enemy_pawns & (1ULL << pawn_square)) {
                                is_safe = false;
                                break;
                            }
                        }
                    }
                }
                
                // Check for enemy king attacks (kings can't be adjacent)
                int enemy_king_pos = board.get_king_position(enemy_color);
                int enemy_king_file = enemy_king_pos & 7;
                int enemy_king_rank = enemy_king_pos >> 3;
                
                int file_diff = abs(new_file - enemy_king_file);
                int rank_diff = abs(new_rank - enemy_king_rank);
                
                if (file_diff <= 1 && rank_diff <= 1) {
                    is_safe = false;
                }
            }
            
            if (is_safe) {
                safe_escape_squares++;
            } else {
                attacked_escape_squares++;
            }
        }
    }
    
    // Calculate score based on escape square quality
    score += safe_escape_squares * EvalConstants::KING_ESCAPE_SQUARES_BONUS;
    score += attacked_escape_squares * (EvalConstants::KING_ESCAPE_SQUARES_BONUS / 3); // Partial credit for attacked squares
    
    // Penalty for having very few escape squares (trapped king)
    if (escape_squares <= 2) {
        score -= (3 - escape_squares) * 8; // Increasing penalty for fewer squares
    }
    
    // Bonus for having many safe escape squares
    if (safe_escape_squares >= 6) {
        score += 5; // Bonus for very mobile king
    }
    
    return score;
}

// Evaluate tactical threats to king - optimized
int Evaluation::evaluate_tactical_threats_to_king(const Board& board, Board::Color color) const {
    int score = 0;
    int king_pos = board.get_king_position(color);
    int king_file = king_pos & 7;
    int king_rank = king_pos >> 3;
    Board::Color enemy_color = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;
    
    // Get enemy pieces
    Bitboard enemy_bishops = board.get_piece_bitboard(Board::BISHOP, enemy_color);
    Bitboard enemy_rooks = board.get_piece_bitboard(Board::ROOK, enemy_color);
    Bitboard enemy_queens = board.get_piece_bitboard(Board::QUEEN, enemy_color);
    Bitboard enemy_knights = board.get_piece_bitboard(Board::KNIGHT, enemy_color);
    
    // Check for direct attacks on king
    int direct_attackers = 0;
    
    // Check rook/queen attacks on same rank/file
    Bitboard rank_file_attackers = enemy_rooks | enemy_queens;
    uint64_t rf_bits = rank_file_attackers;
    
    while (rf_bits) {
        int attacker_square = get_lsb(rf_bits);
        rf_bits &= rf_bits - 1;
        
        int attacker_rank = attacker_square >> 3;
        int attacker_file = attacker_square & 7;
        
        // Check if on same rank or file
        if (attacker_rank == king_rank || attacker_file == king_file) {
            direct_attackers++;
            
            // Distance-based penalty (closer is more dangerous)
            int distance = abs(attacker_rank - king_rank) + abs(attacker_file - king_file);
            score += EvalConstants::PIN_ON_KING_PENALTY / std::max(1, distance - 1);
        }
    }
    
    // Check bishop/queen attacks on diagonals
    Bitboard diagonal_attackers = enemy_bishops | enemy_queens;
    uint64_t diag_bits = diagonal_attackers;
    
    while (diag_bits) {
        int attacker_square = get_lsb(diag_bits);
        diag_bits &= diag_bits - 1;
        
        int attacker_rank = attacker_square >> 3;
        int attacker_file = attacker_square & 7;
        
        // Check if on same diagonal
        if (abs(attacker_rank - king_rank) == abs(attacker_file - king_file)) {
            direct_attackers++;
            
            // Distance-based penalty
            int distance = abs(attacker_rank - king_rank);
            score += EvalConstants::PIN_ON_KING_PENALTY / std::max(1, distance - 1);
        }
    }
    
    // Check knight attacks (knights are particularly dangerous in close quarters)
    uint64_t knight_bits = enemy_knights;
    while (knight_bits) {
        int knight_square = get_lsb(knight_bits);
        knight_bits &= knight_bits - 1;
        
        int knight_rank = knight_square >> 3;
        int knight_file = knight_square & 7;
        
        // Check if knight can attack king (L-shaped moves)
        int rank_diff = abs(knight_rank - king_rank);
        int file_diff = abs(knight_file - king_file);
        
        if ((rank_diff == 2 && file_diff == 1) || (rank_diff == 1 && file_diff == 2)) {
            direct_attackers++;
            score += EvalConstants::PIN_ON_KING_PENALTY;
        } else if (rank_diff <= 2 && file_diff <= 2 && (rank_diff + file_diff) <= 3) {
            // Knight is close to king - potential threat
            score += EvalConstants::PIN_ON_KING_PENALTY / 3;
        }
    }
    
    // Penalty multiplier for multiple attackers
    if (direct_attackers >= 2) {
        score += direct_attackers * EvalConstants::PIN_ON_KING_PENALTY / 2;
    }
    
    // Check for pinned pieces (simplified but more accurate)
    Bitboard own_pieces = board.get_color_bitboard(color);
    Bitboard all_pieces = own_pieces | board.get_color_bitboard(enemy_color);
    
    // Check for pins along ranks and files
    Bitboard rank_file_pinners = enemy_rooks | enemy_queens;
    uint64_t pinner_bits = rank_file_pinners;
    
    while (pinner_bits) {
        int pinner_square = get_lsb(pinner_bits);
        pinner_bits &= pinner_bits - 1;
        
        int pinner_rank = pinner_square >> 3;
        int pinner_file = pinner_square & 7;
        
        // Check if pinner is on same rank or file as king
        if (pinner_rank == king_rank || pinner_file == king_file) {
            // Count pieces between pinner and king
            int pieces_between = 0;
            int start_pos, end_pos, step;
            
            if (pinner_rank == king_rank) {
                // Same rank
                start_pos = std::min(pinner_file, king_file) + 1;
                end_pos = std::max(pinner_file, king_file);
                step = 1;
                
                for (int f = start_pos; f < end_pos; ++f) {
                    int check_square = (king_rank << 3) | f;
                    if (all_pieces & (1ULL << check_square)) {
                        pieces_between++;
                        if (own_pieces & (1ULL << check_square)) {
                            // Our piece is pinned
                            if (pieces_between == 1) {
                                score += EvalConstants::DISCOVERED_CHECK_THREAT_PENALTY;
                            }
                        }
                    }
                }
            } else {
                // Same file
                start_pos = std::min(pinner_rank, king_rank) + 1;
                end_pos = std::max(pinner_rank, king_rank);
                
                for (int r = start_pos; r < end_pos; ++r) {
                    int check_square = (r << 3) | king_file;
                    if (all_pieces & (1ULL << check_square)) {
                        pieces_between++;
                        if (own_pieces & (1ULL << check_square)) {
                            // Our piece is pinned
                            if (pieces_between == 1) {
                                score += EvalConstants::DISCOVERED_CHECK_THREAT_PENALTY;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Check for fork threats (simplified)
    uint64_t enemy_knight_bits = enemy_knights;
    while (enemy_knight_bits) {
        int knight_square = get_lsb(enemy_knight_bits);
        enemy_knight_bits &= enemy_knight_bits - 1;
        
        // Check if knight can fork king and another valuable piece
        // This is a simplified check for demonstration
        int knight_rank = knight_square >> 3;
        int knight_file = knight_square & 7;
        
        // Check all knight moves for potential forks
        static const int knight_moves[8][2] = {
            {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
            {1, -2}, {1, 2}, {2, -1}, {2, 1}
        };
        
        for (int i = 0; i < 8; ++i) {
            int target_rank = knight_rank + knight_moves[i][0];
            int target_file = knight_file + knight_moves[i][1];
            
            if (target_rank >= 0 && target_rank <= 7 && target_file >= 0 && target_file <= 7) {
                int target_square = (target_rank << 3) | target_file;
                
                // If knight can attack king from this square
                if (target_square == king_pos) {
                    // Check if knight can also attack other valuable pieces
                    Bitboard valuable_pieces = board.get_piece_bitboard(Board::QUEEN, color) |
                                             board.get_piece_bitboard(Board::ROOK, color);
                    
                    for (int j = 0; j < 8; ++j) {
                        int fork_rank = target_rank + knight_moves[j][0];
                        int fork_file = target_file + knight_moves[j][1];
                        
                        if (fork_rank >= 0 && fork_rank <= 7 && fork_file >= 0 && fork_file <= 7) {
                            int fork_square = (fork_rank << 3) | fork_file;
                            if (valuable_pieces & (1ULL << fork_square)) {
                                score += EvalConstants::PIN_ON_KING_PENALTY / 2; // Fork threat
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    
    return score;
}

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