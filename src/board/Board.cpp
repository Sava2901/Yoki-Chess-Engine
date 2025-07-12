#include "Board.h"
#include <iostream>
#include <sstream>
#include <cctype>
#include <algorithm>
#include "MoveGenerator.h"

// Castling lookup tables
static const int CASTLING_ROOK_FROM[2] = {7, 0}; // [kingside, queenside]
static const int CASTLING_ROOK_TO[2] = {5, 3};

// Lookup table for char to piece type conversion
static Board::PieceType CHAR_TO_PIECE_LOOKUP[256];
static bool char_lookup_initialized = false;

// Castling rights masks for square-based updates
static uint8_t CASTLING_RIGHTS_MASK[64];
static bool castling_mask_initialized = false;

void init_char_lookup() {
    if (char_lookup_initialized) return;
    
    // Initialize all to PAWN as default
    for (int i = 0; i < 256; i++) {
        CHAR_TO_PIECE_LOOKUP[i] = Board::PAWN;
    }
    
    // Set specific mappings
    CHAR_TO_PIECE_LOOKUP['p'] = Board::PAWN;
    CHAR_TO_PIECE_LOOKUP['P'] = Board::PAWN;
    CHAR_TO_PIECE_LOOKUP['n'] = Board::KNIGHT;
    CHAR_TO_PIECE_LOOKUP['N'] = Board::KNIGHT;
    CHAR_TO_PIECE_LOOKUP['b'] = Board::BISHOP;
    CHAR_TO_PIECE_LOOKUP['B'] = Board::BISHOP;
    CHAR_TO_PIECE_LOOKUP['r'] = Board::ROOK;
    CHAR_TO_PIECE_LOOKUP['R'] = Board::ROOK;
    CHAR_TO_PIECE_LOOKUP['q'] = Board::QUEEN;
    CHAR_TO_PIECE_LOOKUP['Q'] = Board::QUEEN;
    CHAR_TO_PIECE_LOOKUP['k'] = Board::KING;
    CHAR_TO_PIECE_LOOKUP['K'] = Board::KING;
    
    char_lookup_initialized = true;
}

void init_castling_mask() {
    if (castling_mask_initialized) return;
    
    // Initialize all squares to preserve all rights
    for (int i = 0; i < 64; i++) {
        CASTLING_RIGHTS_MASK[i] = 0xFF;
    }
    
    // Set masks for specific squares
    // Castling rights bits: 0x01=White kingside, 0x02=White queenside, 0x04=Black kingside, 0x08=Black queenside
    CASTLING_RIGHTS_MASK[0] = ~0x02;  // a1 - White queenside rook
    CASTLING_RIGHTS_MASK[7] = ~0x01;  // h1 - White kingside rook
    CASTLING_RIGHTS_MASK[56] = ~0x08; // a8 - Black queenside rook
    CASTLING_RIGHTS_MASK[63] = ~0x04; // h8 - Black kingside rook
    CASTLING_RIGHTS_MASK[4] = ~0x03;  // e1 - White king (both White castling rights)
    CASTLING_RIGHTS_MASK[60] = ~0x0C; // e8 - Black king (both Black castling rights)
    
    castling_mask_initialized = true;
}

Board::Board() {
    // Initialize bitboards to empty
    for (int color = 0; color < NUM_COLORS; color++) {
        for (int piece = 0; piece < NUM_PIECE_TYPES; piece++) {
            piece_bitboards[color][piece] = 0;
        }
        color_bitboards[color] = 0;
        king_positions[color] = -1;
    }
    
    all_pieces = 0;
    active_color = WHITE;
    castling_rights = 0;
    en_passant_file = -1;
    halfmove_clock = 0;
    fullmove_number = 1;
    
    // Initialize piece mailbox
    for (int i = 0; i < 64; i++) {
        piece_mailbox[i] = '.';
    }
    
    // Initialize bitboard utilities if not already done
    BitboardUtils::init();
    
    // Initialize castling mask
    init_castling_mask();
    
    // Initialize character lookup
    init_char_lookup();
}

void Board::set_starting_position() {
    set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void Board::set_from_fen(const std::string& fen) {
    // Clear all bitboards
    for (int color = 0; color < NUM_COLORS; color++) {
        for (int piece = 0; piece < NUM_PIECE_TYPES; piece++) {
            piece_bitboards[color][piece] = 0;
        }
        color_bitboards[color] = 0;
        king_positions[color] = -1;
    }
    all_pieces = 0;
    
    // Clear piece mailbox
    for (int i = 0; i < 64; i++) {
        piece_mailbox[i] = '.';
    }
    
    std::istringstream iss(fen);
    std::string board_part, active_color_part, castling_part, en_passant_part;
    int halfmove_part, fullmove_part;
    
    iss >> board_part >> active_color_part >> castling_part >> en_passant_part >> halfmove_part >> fullmove_part;
    
    // Parse board position
    int rank = 7, file = 0;
    for (char c : board_part) {
        if (c == '/') {
            rank--;
            file = 0;
        } else if (std::isdigit(c)) {
            file += (c - '0');
        } else {
            set_piece(rank, file, c);
            file++;
        }
    }
    
    // Parse active color
    active_color = (active_color_part == "w") ? WHITE : BLACK;
    
    // Parse castling rights
    castling_rights = 0;
    for (char c : castling_part) {
        switch (c) {
            case 'K': castling_rights |= 0x01; break; // White kingside
            case 'Q': castling_rights |= 0x02; break; // White queenside
            case 'k': castling_rights |= 0x04; break; // Black kingside
            case 'q': castling_rights |= 0x08; break; // Black queenside
            default: ;
        }
    }
    
    // Parse en passant
    if (en_passant_part != "-") {
        en_passant_file = en_passant_part[0] - 'a';
    } else {
        en_passant_file = -1;
    }
    
    // Parse move counters
    halfmove_clock = halfmove_part;
    fullmove_number = fullmove_part;
    
    update_combined_bitboards();
}

std::string Board::to_fen() const {
    std::ostringstream oss;
    
    // Board position
    for (int rank = 7; rank >= 0; rank--) {
        int empty_count = 0;
        for (int file = 0; file < 8; file++) {
            int square = BitboardUtils::square_index(rank, file);
            char piece = piece_mailbox[square];  // Direct access instead of get_piece()
            if (piece == '.') {
                empty_count++;
            } else {
                if (empty_count > 0) {
                    oss << empty_count;
                    empty_count = 0;
                }
                oss << piece;
            }
        }
        if (empty_count > 0) {
            oss << empty_count;
        }
        if (rank > 0) oss << '/';
    }
    
    // Active color
    oss << " " << (active_color == WHITE ? "w" : "b");
    
    // Castling rights
    oss << " ";
    if (castling_rights == 0) {
        oss << "-";
    } else {
        if (castling_rights & 0x01) oss << "K";
        if (castling_rights & 0x02) oss << "Q";
        if (castling_rights & 0x04) oss << "k";
        if (castling_rights & 0x08) oss << "q";
    }
    
    // En passant
    oss << " ";
    if (en_passant_file == -1) {
        oss << "-";
    } else {
        oss << static_cast<char>('a' + en_passant_file);
        oss << (active_color == WHITE ? "6" : "3");
    }
    
    // Move counters
    oss << " " << halfmove_clock << " " << fullmove_number;
    
    return oss.str();
}

void Board::set_piece(int rank, int file, char piece) {
    int square = BitboardUtils::square_index(rank, file);
    
    // Only clear if square is not already empty (branchless check)
    if (piece_mailbox[square] != '.') {
        clear_square(square);
    }
    
    // Place the new piece if it's not empty
    if (piece != '.') {
        PieceType piece_type = char_to_piece_type(piece);
        Color color = char_to_color(piece);
        place_piece(square, piece_type, color);
    }
}

char Board::get_piece(int rank, int file) const {
    int square = BitboardUtils::square_index(rank, file);
    return piece_mailbox[square];
}

void Board::clear_square(int square) {
    char piece = piece_mailbox[square];
    if (piece != '.') {
        PieceType piece_type = char_to_piece_type(piece);
        Color color = char_to_color(piece);
        BitboardUtils::clear_bit(piece_bitboards[color][piece_type], square);
        piece_mailbox[square] = '.';
        update_combined_bitboards();
    }
}

void Board::place_piece(int square, PieceType piece_type, Color color) {
    BitboardUtils::set_bit(piece_bitboards[color][piece_type], square);
    piece_mailbox[square] = piece_to_char(piece_type, color);
    
    // Update king position (branchless)
    king_positions[color] = (piece_type == KING) ? square : king_positions[color];
    
    update_combined_bitboards();
}

Bitboard Board::get_piece_bitboard(PieceType piece_type, Color color) const {
    return piece_bitboards[color][piece_type];
}

Bitboard Board::get_color_bitboard(Color color) const {
    return color_bitboards[color];
}

Bitboard Board::get_all_pieces() const {
    return all_pieces;
}

bool Board::is_move_valid(const Move& move) const {
    // Basic move validation
    if (!move.is_valid()) {
        return false;
    }
    
    // Check if piece exists on source square
    char piece_on_source = get_piece(move.from_rank, move.from_file);
    if (piece_on_source == '.' || piece_on_source != move.piece) {
        return false;
    }
    
    // Check if it's the correct player's turn
    Color piece_color = char_to_color(move.piece);
    if (piece_color != active_color) {
        return false;
    }
    
    // Check for friendly fire (capturing own pieces)
    if (!move.is_en_passant) {
        char target_piece = get_piece(move.to_rank, move.to_file);
        if (target_piece != '.' && char_to_color(target_piece) == piece_color) {
            return false; // Cannot capture own pieces
        }
    }
    
    // For en passant, check if it's valid
    if (move.is_en_passant) {
        if (en_passant_file == -1 || move.to_file != en_passant_file) {
            return false;
        }
        PieceType piece_type = char_to_piece_type(move.piece);
        if (piece_type != PAWN) {
            return false;
        }
    }
    
    // For castling, perform basic validation
    if (move.is_castling) {
        PieceType piece_type = char_to_piece_type(move.piece);
        if (piece_type != KING) {
            return false;
        }
        // Additional castling validation would go here
    }
    
    return true;
}

bool Board::is_move_legal(const Move& move) {
    MoveGenerator generator;
    std::vector<Move> legal_moves = generator.generate_legal_moves(*this);
    return std::any_of(legal_moves.begin(), legal_moves.end(), [&](const Move& m) {
        return m == move;
    });
}

BitboardMoveUndoData Board::make_move(const Move& move) {
    if (!is_move_legal(move)) return {};
    return apply_move(move);
}

BitboardMoveUndoData Board::apply_move(const Move& move) {
    // Store undo data
    BitboardMoveUndoData undo_data;
    undo_data.move = move;
    undo_data.captured_piece = move.captured_piece;
    undo_data.castling_rights = castling_rights;
    undo_data.en_passant_file = en_passant_file;
    undo_data.halfmove_clock = halfmove_clock;
    
    int from_square = BitboardUtils::square_index(move.from_rank, move.from_file);
    int to_square = BitboardUtils::square_index(move.to_rank, move.to_file);
    
    // Cache piece type and color lookups
    PieceType moving_piece_type = char_to_piece_type(move.piece);
    Color moving_color = char_to_color(move.piece);
    Color opponent_color = (moving_color == WHITE) ? BLACK : WHITE;
    
    // Handle captures (store captured piece if not already set)
    if (undo_data.captured_piece == '.' && !move.is_en_passant) {
        undo_data.captured_piece = get_piece(move.to_rank, move.to_file);
    }
    
    // Clear source square from mailbox
    piece_mailbox[from_square] = '.';
    
    // Remove piece from source square bitboard
    BitboardUtils::clear_bit(piece_bitboards[moving_color][moving_piece_type], from_square);
    
    // Handle captures
    if (move.is_en_passant) {
        // En passant capture - remove the captured pawn
        int captured_pawn_rank = (moving_color == WHITE) ? move.to_rank - 1 : move.to_rank + 1;
        int captured_pawn_square = BitboardUtils::square_index(captured_pawn_rank, move.to_file);
        BitboardUtils::clear_bit(piece_bitboards[opponent_color][PAWN], captured_pawn_square);
        piece_mailbox[captured_pawn_square] = '.';
    } else if (undo_data.captured_piece != '.') {
        // Normal capture - remove captured piece from destination square
        PieceType captured_piece_type = char_to_piece_type(undo_data.captured_piece);
        Color captured_color = char_to_color(undo_data.captured_piece);
        BitboardUtils::clear_bit(piece_bitboards[captured_color][captured_piece_type], to_square);
    }
    
    // Place piece on destination square
    if (move.promotion_piece != '.') {
        // Handle promotion
        PieceType promotion_type = char_to_piece_type(move.promotion_piece);
        BitboardUtils::set_bit(piece_bitboards[moving_color][promotion_type], to_square);
        piece_mailbox[to_square] = move.promotion_piece;
    } else {
        BitboardUtils::set_bit(piece_bitboards[moving_color][moving_piece_type], to_square);
        piece_mailbox[to_square] = move.piece;
    }
    
    // Update king position if king moved (branchless)
    king_positions[moving_color] = (moving_piece_type == KING) ? to_square : king_positions[moving_color];
    
    // Handle castling
    if (move.is_castling) {
        int castling_side = (move.to_file == 6) ? 0 : 1; // 0=kingside, 1=queenside
        int rook_from_square = BitboardUtils::square_index(move.from_rank, CASTLING_ROOK_FROM[castling_side]);
        int rook_to_square = BitboardUtils::square_index(move.from_rank, CASTLING_ROOK_TO[castling_side]);
        
        BitboardUtils::clear_bit(piece_bitboards[moving_color][ROOK], rook_from_square);
        BitboardUtils::set_bit(piece_bitboards[moving_color][ROOK], rook_to_square);
        piece_mailbox[rook_from_square] = '.';
        piece_mailbox[rook_to_square] = piece_to_char(ROOK, moving_color);
    }
    
    // Update castling rights using lookup table
    castling_rights &= CASTLING_RIGHTS_MASK[from_square];
    castling_rights &= CASTLING_RIGHTS_MASK[to_square];
    
    // Update en passant file (branchless)
    bool is_double_pawn_move = (moving_piece_type == PAWN) && 
        ((moving_color == WHITE && move.to_rank - move.from_rank == 2) ||
         (moving_color == BLACK && move.from_rank - move.to_rank == 2));
    en_passant_file = is_double_pawn_move ? move.from_file : -1;
    
    // Update halfmove clock (branchless)
    bool reset_halfmove = (moving_piece_type == PAWN) || (undo_data.captured_piece != '.');
    halfmove_clock = reset_halfmove ? 0 : halfmove_clock + 1;
    
    // Update fullmove number (branchless)
    fullmove_number += (active_color == BLACK);
    
    // Switch active color
    active_color = opponent_color;
    
    update_combined_bitboards();
    return undo_data;
}

void Board::undo_move(const BitboardMoveUndoData& undo_data) {
    const Move& move = undo_data.move;
    
    // Restore game state
    castling_rights = undo_data.castling_rights;
    en_passant_file = undo_data.en_passant_file;
    halfmove_clock = undo_data.halfmove_clock;
    
    // Switch back active color
    active_color = (active_color == WHITE) ? BLACK : WHITE;
    
    // Update fullmove number (branchless)
    fullmove_number -= (active_color == BLACK);
    
    int from_square = BitboardUtils::square_index(move.from_rank, move.from_file);
    int to_square = BitboardUtils::square_index(move.to_rank, move.to_file);
    
    // Cache piece type and color lookups
    PieceType moving_piece_type = char_to_piece_type(move.piece);
    Color moving_color = char_to_color(move.piece);
    Color opponent_color = (moving_color == WHITE) ? BLACK : WHITE;
    
    // Handle castling undo
    if (move.is_castling) {
        int castling_side = (move.to_file == 6) ? 0 : 1; // 0=kingside, 1=queenside
        int rook_from_square = BitboardUtils::square_index(move.from_rank, CASTLING_ROOK_FROM[castling_side]);
        int rook_to_square = BitboardUtils::square_index(move.from_rank, CASTLING_ROOK_TO[castling_side]);
        
        // Move rook back
        BitboardUtils::clear_bit(piece_bitboards[moving_color][ROOK], rook_to_square);
        BitboardUtils::set_bit(piece_bitboards[moving_color][ROOK], rook_from_square);
        piece_mailbox[rook_to_square] = '.';
        piece_mailbox[rook_from_square] = piece_to_char(ROOK, moving_color);
    }
    
    // Remove piece from destination square
    if (move.promotion_piece != '.') {
        // Undo promotion
        PieceType promotion_type = char_to_piece_type(move.promotion_piece);
        BitboardUtils::clear_bit(piece_bitboards[moving_color][promotion_type], to_square);
    } else {
        BitboardUtils::clear_bit(piece_bitboards[moving_color][moving_piece_type], to_square);
    }
    
    // Place piece back on source square
    BitboardUtils::set_bit(piece_bitboards[moving_color][moving_piece_type], from_square);
    piece_mailbox[from_square] = move.piece;
    piece_mailbox[to_square] = '.';
    
    // Update king position if king moved (branchless)
    king_positions[moving_color] = (moving_piece_type == KING) ? from_square : king_positions[moving_color];
    
    // Restore captured piece
    if (move.is_en_passant) {
        // Restore en passant captured pawn
        int captured_pawn_rank = (moving_color == WHITE) ? move.to_rank - 1 : move.to_rank + 1;
        int captured_pawn_square = BitboardUtils::square_index(captured_pawn_rank, move.to_file);
        BitboardUtils::set_bit(piece_bitboards[opponent_color][PAWN], captured_pawn_square);
        piece_mailbox[captured_pawn_square] = piece_to_char(PAWN, opponent_color);
    } else if (undo_data.captured_piece != '.') {
        // Restore normally captured piece
        PieceType captured_type = char_to_piece_type(undo_data.captured_piece);
        Color captured_color = char_to_color(undo_data.captured_piece);
        BitboardUtils::set_bit(piece_bitboards[captured_color][captured_type], to_square);
        piece_mailbox[to_square] = undo_data.captured_piece;
        
        // Update king position if captured piece was a king (branchless)
        king_positions[captured_color] = (captured_type == KING) ? to_square : king_positions[captured_color];
    }
    
    update_combined_bitboards();
}

bool Board::is_square_attacked(int square, Color attacking_color) const {
    return get_attackers_to_square(square, attacking_color) != 0;
}

bool Board::is_in_check(Color color) const {
    int king_square = king_positions[color];
    if (king_square == -1) return false;
    
    Color opponent_color = (color == WHITE) ? BLACK : WHITE;
    return is_square_attacked(king_square, opponent_color);
}

void Board::print() const {
    std::cout << to_string() << std::endl;
}

std::string Board::to_string() const {
    std::ostringstream oss;
    
    oss << "  a b c d e f g h\n";
    for (int rank = 7; rank >= 0; rank--) {
        oss << (rank + 1) << " ";
        for (int file = 0; file < 8; file++) {
            int square = BitboardUtils::square_index(rank, file);
            char piece = piece_mailbox[square];  // Direct access instead of get_piece()
            oss << piece << " ";
        }
        oss << (rank + 1) << "\n";
    }
    oss << "  a b c d e f g h\n";
    
    oss << "\nFEN: " << to_fen() << "\n";
    
    return oss.str();
}

Board::PieceType Board::char_to_piece_type(char piece) {
    init_char_lookup(); // Ensure lookup table is initialized
    return CHAR_TO_PIECE_LOOKUP[static_cast<unsigned char>(piece)];
}

Board::Color Board::char_to_color(char piece) {
    return std::isupper(piece) ? WHITE : BLACK;
}

char Board::piece_to_char(PieceType piece_type, Color color) {
    char pieces[] = {'p', 'n', 'b', 'r', 'q', 'k'};
    char piece_char = pieces[piece_type];
    return (color == WHITE) ? std::toupper(piece_char) : piece_char;
}

void Board::update_combined_bitboards() {
    // Update color bitboards
    for (int color = 0; color < NUM_COLORS; color++) {
        color_bitboards[color] = 0;
        for (int piece = 0; piece < NUM_PIECE_TYPES; piece++) {
            color_bitboards[color] |= piece_bitboards[color][piece];
        }
    }
    
    // Update all pieces bitboard
    all_pieces = color_bitboards[WHITE] | color_bitboards[BLACK];
}

void Board::update_king_position(Color color) {
    Bitboard king_bb = piece_bitboards[color][KING];
    if (king_bb != 0) {
        king_positions[color] = BitboardUtils::get_lsb_index(king_bb);
    } else {
        king_positions[color] = -1;
    }
}

Bitboard Board::get_attackers_to_square(int square, Color attacking_color) const {
    Bitboard attackers = 0;
    
    // Pawn attacks
    Bitboard pawn_attacks = BitboardUtils::pawn_attacks(square, attacking_color == BLACK);
    attackers |= pawn_attacks & piece_bitboards[attacking_color][PAWN];
    
    // Knight attacks
    Bitboard knight_attacks = BitboardUtils::knight_attacks(square);
    attackers |= knight_attacks & piece_bitboards[attacking_color][KNIGHT];
    
    // Bishop/Queen diagonal attacks
    Bitboard bishop_attacks = BitboardUtils::bishop_attacks(square, all_pieces);
    attackers |= bishop_attacks & (piece_bitboards[attacking_color][BISHOP] | piece_bitboards[attacking_color][QUEEN]);
    
    // Rook/Queen straight attacks
    Bitboard rook_attacks = BitboardUtils::rook_attacks(square, all_pieces);
    attackers |= rook_attacks & (piece_bitboards[attacking_color][ROOK] | piece_bitboards[attacking_color][QUEEN]);
    
    // King attacks
    Bitboard king_attacks = BitboardUtils::king_attacks(square);
    attackers |= king_attacks & piece_bitboards[attacking_color][KING];
    
    return attackers;
}