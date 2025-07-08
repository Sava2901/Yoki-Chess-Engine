#include "board.h"
#include "utils.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace yoki {

// Zobrist hash tables (simplified)
static uint64_t piece_hash[64][12]; // 64 squares, 12 piece types (6 pieces * 2 colors)
static uint64_t side_hash;
static uint64_t castling_hash[16];
static uint64_t en_passant_hash[64];
static bool hash_initialized = false;

static void init_zobrist() {
    if (hash_initialized) return;
    
    // Simple PRNG for hash values
    uint64_t seed = 1070372;
    auto next_random = [&seed]() {
        seed = seed * 1103515245 + 12345;
        return seed;
    };
    
    for (int sq = 0; sq < 64; ++sq) {
        for (int piece = 0; piece < 12; ++piece) {
            piece_hash[sq][piece] = next_random();
        }
        en_passant_hash[sq] = next_random();
    }
    
    side_hash = next_random();
    
    for (int i = 0; i < 16; ++i) {
        castling_hash[i] = next_random();
    }
    
    hash_initialized = true;
}

std::string Move::to_string() const {
    std::string result = square_to_string(from) + square_to_string(to);
    if (promotion != EMPTY) {
        char promo_char = 'q';
        switch (promotion) {
            case QUEEN: promo_char = 'q'; break;
            case ROOK: promo_char = 'r'; break;
            case BISHOP: promo_char = 'b'; break;
            case KNIGHT: promo_char = 'n'; break;
            default: break;
        }
        result += promo_char;
    }
    return result;
}

Board::Board() {
    init_zobrist();
    reset();
}

Board::Board(const std::string& fen) {
    init_zobrist();
    load_fen(fen);
}

void Board::reset() {
    // Clear board
    for (int i = 0; i < 64; ++i) {
        pieces_[i] = EMPTY;
        colors_[i] = WHITE;
    }
    
    // Set up starting position
    load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

bool Board::load_fen(const std::string& fen) {
    std::istringstream iss(fen);
    std::string board_str, side_str, castling_str, en_passant_str, halfmove_str, fullmove_str;
    
    if (!(iss >> board_str >> side_str >> castling_str >> en_passant_str >> halfmove_str >> fullmove_str)) {
        return false;
    }
    
    // Clear board
    for (int i = 0; i < 64; ++i) {
        pieces_[i] = EMPTY;
    }
    
    // Parse board
    int rank = 7, file = 0;
    for (char c : board_str) {
        if (c == '/') {
            rank--;
            file = 0;
        } else if (std::isdigit(c)) {
            file += c - '0';
        } else {
            Square sq = make_square(file, rank);
            Color color = std::isupper(c) ? WHITE : BLACK;
            
            switch (std::tolower(c)) {
                case 'p': set_piece(sq, PAWN, color); break;
                case 'n': set_piece(sq, KNIGHT, color); break;
                case 'b': set_piece(sq, BISHOP, color); break;
                case 'r': set_piece(sq, ROOK, color); break;
                case 'q': set_piece(sq, QUEEN, color); break;
                case 'k': set_piece(sq, KING, color); break;
                default: return false;
            }
            file++;
        }
    }
    
    // Parse side to move
    side_to_move_ = (side_str == "w") ? WHITE : BLACK;
    
    // Parse castling rights
    castling_rights_ = {};
    for (char c : castling_str) {
        switch (c) {
            case 'K': castling_rights_.white_kingside = true; break;
            case 'Q': castling_rights_.white_queenside = true; break;
            case 'k': castling_rights_.black_kingside = true; break;
            case 'q': castling_rights_.black_queenside = true; break;
        }
    }
    
    // Parse en passant
    en_passant_square_ = (en_passant_str == "-") ? -1 : string_to_square(en_passant_str);
    
    // Parse move counters
    halfmove_clock_ = std::stoi(halfmove_str);
    fullmove_number_ = std::stoi(fullmove_str);
    
    update_hash();
    return true;
}

std::string Board::to_fen() const {
    std::ostringstream oss;
    
    // Board position
    for (int rank = 7; rank >= 0; --rank) {
        int empty_count = 0;
        for (int file = 0; file < 8; ++file) {
            Square sq = make_square(file, rank);
            if (is_empty(sq)) {
                empty_count++;
            } else {
                if (empty_count > 0) {
                    oss << empty_count;
                    empty_count = 0;
                }
                
                char piece_char = 'p';
                switch (get_piece_type(sq)) {
                    case PAWN: piece_char = 'p'; break;
                    case KNIGHT: piece_char = 'n'; break;
                    case BISHOP: piece_char = 'b'; break;
                    case ROOK: piece_char = 'r'; break;
                    case QUEEN: piece_char = 'q'; break;
                    case KING: piece_char = 'k'; break;
                    default: break;
                }
                
                if (get_piece_color(sq) == WHITE) {
                    piece_char = std::toupper(piece_char);
                }
                oss << piece_char;
            }
        }
        if (empty_count > 0) {
            oss << empty_count;
        }
        if (rank > 0) oss << '/';
    }
    
    // Side to move
    oss << " " << (side_to_move_ == WHITE ? "w" : "b");
    
    // Castling rights
    oss << " ";
    std::string castling;
    if (castling_rights_.white_kingside) castling += 'K';
    if (castling_rights_.white_queenside) castling += 'Q';
    if (castling_rights_.black_kingside) castling += 'k';
    if (castling_rights_.black_queenside) castling += 'q';
    oss << (castling.empty() ? "-" : castling);
    
    // En passant
    oss << " " << (en_passant_square_ == -1 ? "-" : square_to_string(en_passant_square_));
    
    // Move counters
    oss << " " << halfmove_clock_ << " " << fullmove_number_;
    
    return oss.str();
}

PieceType Board::get_piece_type(Square sq) const {
    return is_valid_square(sq) ? pieces_[sq] : EMPTY;
}

Color Board::get_piece_color(Square sq) const {
    return is_valid_square(sq) ? colors_[sq] : WHITE;
}

bool Board::is_empty(Square sq) const {
    return get_piece_type(sq) == EMPTY;
}

bool Board::make_move(const Move& move) {
    if (!is_legal_move(move)) {
        return false;
    }
    
    // Save current state
    BoardState state;
    state.captured_piece = get_piece_type(move.to);
    state.captured_color = get_piece_color(move.to);
    state.castling_rights = castling_rights_;
    state.en_passant_square = en_passant_square_;
    state.halfmove_clock = halfmove_clock_;
    state.hash = hash_;
    history_.push_back(state);
    
    // Make the move
    PieceType moving_piece = get_piece_type(move.from);
    Color moving_color = get_piece_color(move.from);
    
    clear_square(move.from);
    
    if (move.promotion != EMPTY) {
        set_piece(move.to, move.promotion, moving_color);
    } else {
        set_piece(move.to, moving_piece, moving_color);
    }
    
    // Handle special moves
    if (move.is_castling) {
        // Move rook for castling
        if (move.to == make_square(6, 0)) { // White kingside
            clear_square(make_square(7, 0));
            set_piece(make_square(5, 0), ROOK, WHITE);
        } else if (move.to == make_square(2, 0)) { // White queenside
            clear_square(make_square(0, 0));
            set_piece(make_square(3, 0), ROOK, WHITE);
        } else if (move.to == make_square(6, 7)) { // Black kingside
            clear_square(make_square(7, 7));
            set_piece(make_square(5, 7), ROOK, BLACK);
        } else if (move.to == make_square(2, 7)) { // Black queenside
            clear_square(make_square(0, 7));
            set_piece(make_square(3, 7), ROOK, BLACK);
        }
    }
    
    if (move.is_en_passant) {
        // Remove captured pawn
        int capture_rank = (moving_color == WHITE) ? get_rank(move.to) - 1 : get_rank(move.to) + 1;
        clear_square(make_square(get_file(move.to), capture_rank));
    }
    
    // Update castling rights
    if (moving_piece == KING) {
        if (moving_color == WHITE) {
            castling_rights_.white_kingside = false;
            castling_rights_.white_queenside = false;
        } else {
            castling_rights_.black_kingside = false;
            castling_rights_.black_queenside = false;
        }
    }
    
    // Update en passant square
    en_passant_square_ = -1;
    if (moving_piece == PAWN && abs(get_rank(move.to) - get_rank(move.from)) == 2) {
        en_passant_square_ = make_square(get_file(move.from), (get_rank(move.from) + get_rank(move.to)) / 2);
    }
    
    // Update move counters
    if (moving_piece == PAWN || state.captured_piece != EMPTY) {
        halfmove_clock_ = 0;
    } else {
        halfmove_clock_++;
    }
    
    if (side_to_move_ == BLACK) {
        fullmove_number_++;
    }
    
    // Switch side
    side_to_move_ = (side_to_move_ == WHITE) ? BLACK : WHITE;
    
    update_hash();
    return true;
}

void Board::unmake_move() {
    if (history_.empty()) return;
    
    BoardState state = history_.back();
    history_.pop_back();
    
    // Restore state
    castling_rights_ = state.castling_rights;
    en_passant_square_ = state.en_passant_square;
    halfmove_clock_ = state.halfmove_clock;
    hash_ = state.hash;
    
    // Switch side back
    side_to_move_ = (side_to_move_ == WHITE) ? BLACK : WHITE;
    
    if (side_to_move_ == BLACK) {
        fullmove_number_--;
    }
}

bool Board::is_legal_move(const Move& move) const {
    // Basic validation
    if (!is_valid_square(move.from) || !is_valid_square(move.to)) {
        return false;
    }
    
    if (is_empty(move.from)) {
        return false;
    }
    
    if (get_piece_color(move.from) != side_to_move_) {
        return false;
    }
    
    if (!is_empty(move.to) && get_piece_color(move.to) == side_to_move_) {
        return false;
    }
    
    // TODO: Implement full move validation logic
    // This is a simplified version
    return true;
}

bool Board::is_in_check(Color color) const {
    Square king_sq = get_king_square(color);
    if (king_sq == -1) return false;
    
    return is_attacked_by(king_sq, (color == WHITE) ? BLACK : WHITE);
}

bool Board::is_checkmate() const {
    // TODO: Implement checkmate detection
    return false;
}

bool Board::is_stalemate() const {
    // TODO: Implement stalemate detection
    return false;
}

bool Board::is_draw() const {
    return halfmove_clock_ >= 100; // 50-move rule
}

void Board::print() const {
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << (rank + 1) << " ";
        for (int file = 0; file < 8; ++file) {
            Square sq = make_square(file, rank);
            char piece_char = '.';
            
            if (!is_empty(sq)) {
                switch (get_piece_type(sq)) {
                    case PAWN: piece_char = 'p'; break;
                    case KNIGHT: piece_char = 'n'; break;
                    case BISHOP: piece_char = 'b'; break;
                    case ROOK: piece_char = 'r'; break;
                    case QUEEN: piece_char = 'q'; break;
                    case KING: piece_char = 'k'; break;
                    default: break;
                }
                
                if (get_piece_color(sq) == WHITE) {
                    piece_char = std::toupper(piece_char);
                }
            }
            
            std::cout << piece_char << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "  a b c d e f g h" << std::endl;
    std::cout << "FEN: " << to_fen() << std::endl;
}

void Board::update_hash() {
    hash_ = 0;
    
    // Hash pieces
    for (int sq = 0; sq < 64; ++sq) {
        if (!is_empty(sq)) {
            int piece_index = get_piece_type(sq) - 1 + (get_piece_color(sq) * 6);
            hash_ ^= piece_hash[sq][piece_index];
        }
    }
    
    // Hash side to move
    if (side_to_move_ == BLACK) {
        hash_ ^= side_hash;
    }
    
    // Hash castling rights
    int castling_index = 0;
    if (castling_rights_.white_kingside) castling_index |= 1;
    if (castling_rights_.white_queenside) castling_index |= 2;
    if (castling_rights_.black_kingside) castling_index |= 4;
    if (castling_rights_.black_queenside) castling_index |= 8;
    hash_ ^= castling_hash[castling_index];
    
    // Hash en passant
    if (en_passant_square_ != -1) {
        hash_ ^= en_passant_hash[en_passant_square_];
    }
}

void Board::clear_square(Square sq) {
    if (is_valid_square(sq)) {
        pieces_[sq] = EMPTY;
    }
}

void Board::set_piece(Square sq, PieceType piece, Color color) {
    if (is_valid_square(sq)) {
        pieces_[sq] = piece;
        colors_[sq] = color;
    }
}

bool Board::is_valid_square(Square sq) const {
    return sq >= 0 && sq < 64;
}

Square Board::get_king_square(Color color) const {
    for (int sq = 0; sq < 64; ++sq) {
        if (get_piece_type(sq) == KING && get_piece_color(sq) == color) {
            return sq;
        }
    }
    return -1;
}

bool Board::is_attacked_by(Square sq, Color attacker) const {
    // TODO: Implement attack detection
    // This is a simplified version
    return false;
}

// Utility functions
Square make_square(int file, int rank) {
    return rank * 8 + file;
}

int get_file(Square sq) {
    return sq % 8;
}

int get_rank(Square sq) {
    return sq / 8;
}

std::string square_to_string(Square sq) {
    if (sq < 0 || sq >= 64) return "--";
    
    char file = 'a' + get_file(sq);
    char rank = '1' + get_rank(sq);
    return std::string(1, file) + std::string(1, rank);
}

Square string_to_square(const std::string& str) {
    if (str.length() != 2) return -1;
    
    int file = str[0] - 'a';
    int rank = str[1] - '1';
    
    if (file < 0 || file > 7 || rank < 0 || rank > 7) {
        return -1;
    }
    
    return make_square(file, rank);
}

} // namespace yoki