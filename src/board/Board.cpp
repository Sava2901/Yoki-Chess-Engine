#include "Board.h"
#include <iostream>
#include <sstream>
#include <cctype>

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
    
    // Initialize bitboard utilities if not already done
    BitboardUtils::init();
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
            char piece = get_piece(rank, file);
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
    
    // Clear the square first
    clear_square(square);
    
    // Place the new piece if it's not empty
    if (piece != '.') {
        PieceType piece_type = char_to_piece_type(piece);
        Color color = char_to_color(piece);
        place_piece(square, piece_type, color);
    }
}

char Board::get_piece(int rank, int file) const {
    int square = BitboardUtils::square_index(rank, file);
    
    for (int color = 0; color < NUM_COLORS; color++) {
        for (int piece = 0; piece < NUM_PIECE_TYPES; piece++) {
            if (BitboardUtils::get_bit(piece_bitboards[color][piece], square)) {
                return piece_to_char(static_cast<PieceType>(piece), static_cast<Color>(color));
            }
        }
    }
    
    return '.';
}

void Board::clear_square(int square) {
    for (int color = 0; color < NUM_COLORS; color++) {
        for (int piece = 0; piece < NUM_PIECE_TYPES; piece++) {
            BitboardUtils::clear_bit(piece_bitboards[color][piece], square);
        }
    }
    update_combined_bitboards();
}

void Board::place_piece(int square, PieceType piece_type, Color color) {
    BitboardUtils::set_bit(piece_bitboards[color][piece_type], square);
    
    if (piece_type == KING) {
        king_positions[color] = square;
    }
    
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

bool Board::make_move(const Move& move) {
    // TODO: This is a simplified version - full implementation would handle
    // all special moves like castling, en passant, promotion
    
    int from_square = BitboardUtils::square_index(move.from_rank, move.from_file);
    int to_square = BitboardUtils::square_index(move.to_rank, move.to_file);
    
    // Find the piece being moved
    PieceType moving_piece_type = char_to_piece_type(move.piece);
    Color moving_color = char_to_color(move.piece);
    
    // Remove piece from source square
    BitboardUtils::clear_bit(piece_bitboards[moving_color][moving_piece_type], from_square);
    
    // Clear destination square (capture)
    clear_square(to_square);
    
    // Place piece on destination square
    if (move.promotion_piece != '.') {
        // Handle promotion
        PieceType promotion_type = char_to_piece_type(move.promotion_piece);
        BitboardUtils::set_bit(piece_bitboards[moving_color][promotion_type], to_square);
    } else {
        BitboardUtils::set_bit(piece_bitboards[moving_color][moving_piece_type], to_square);
    }
    
    // Update king position if king moved
    if (moving_piece_type == KING) {
        king_positions[moving_color] = to_square;
    }
    
    // Handle castling
    if (move.is_castling) {
        int rook_from_file, rook_to_file;
        if (move.to_file == 6) { // Kingside
            rook_from_file = 7;
            rook_to_file = 5;
        } else { // Queenside
            rook_from_file = 0;
            rook_to_file = 3;
        }
        
        int rook_from_square = BitboardUtils::square_index(move.from_rank, rook_from_file);
        int rook_to_square = BitboardUtils::square_index(move.from_rank, rook_to_file);
        
        BitboardUtils::clear_bit(piece_bitboards[moving_color][ROOK], rook_from_square);
        BitboardUtils::set_bit(piece_bitboards[moving_color][ROOK], rook_to_square);
    }
    
    // Handle en passant
    if (move.is_en_passant) {
        int captured_pawn_rank = (moving_color == WHITE) ? move.to_rank - 1 : move.to_rank + 1;
        int captured_pawn_square = BitboardUtils::square_index(captured_pawn_rank, move.to_file);
        Color opponent_color = (moving_color == WHITE) ? BLACK : WHITE;
        BitboardUtils::clear_bit(piece_bitboards[opponent_color][PAWN], captured_pawn_square);
    }
    
    // Update game state
    active_color = (active_color == WHITE) ? BLACK : WHITE;
    
    // Update castling rights (simplified)
    if (moving_piece_type == KING) {
        if (moving_color == WHITE) {
            castling_rights &= ~0x03; // Clear white castling rights
        } else {
            castling_rights &= ~0x0C; // Clear black castling rights
        }
    }
    
    // Update en passant file
    if (moving_piece_type == PAWN && abs(move.to_rank - move.from_rank) == 2) {
        en_passant_file = move.from_file;
    } else {
        en_passant_file = -1;
    }
    
    update_combined_bitboards();
    return true;
}

void Board::undo_move(const Move& move, const BitboardMoveUndoData& undo_data) {
    // Restore game state
    castling_rights = undo_data.castling_rights;
    en_passant_file = undo_data.en_passant_file;
    halfmove_clock = undo_data.halfmove_clock;
    active_color = (active_color == WHITE) ? BLACK : WHITE;
    
    // Reverse the move (simplified implementation)
    // TODO: Full implementation would handle all special cases
    
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
            oss << get_piece(rank, file) << " ";
        }
        oss << (rank + 1) << "\n";
    }
    oss << "  a b c d e f g h\n";
    
    oss << "\nFEN: " << to_fen() << "\n";
    
    return oss.str();
}

Board::PieceType Board::char_to_piece_type(char piece) {
    switch (std::tolower(piece)) {
        case 'p': return PAWN;
        case 'n': return KNIGHT;
        case 'b': return BISHOP;
        case 'r': return ROOK;
        case 'q': return QUEEN;
        case 'k': return KING;
        default: return PAWN; // Should not happen
    }
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