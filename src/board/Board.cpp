#include "Board.h"
#include <iostream>
#include <sstream>
#include <cctype>

Board::Board() {
    initialize_starting_position();
}

Board::~Board() {
    // Nothing to clean up for now
}

void Board::clear() {
    initialize_empty_board();
    active_color = 'w';
    castling_rights = "-";
    en_passant_target = "-";
    halfmove_clock = 0;
    fullmove_number = 1;
}

void Board::set_from_fen(const std::string& piece_placement) {
    initialize_empty_board();
    
    std::vector<std::string> ranks;
    std::istringstream iss(piece_placement);
    std::string rank;
    
    // Split by '/'
    while (std::getline(iss, rank, '/')) {
        ranks.push_back(rank);
    }
    
    for (int rank_idx = 0; rank_idx < 8 && rank_idx < (int)ranks.size(); rank_idx++) {
        int file_idx = 0;
        for (char c : ranks[rank_idx]) {
            if (std::isdigit(c)) {
                int empty_squares = c - '0';
                for (int i = 0; i < empty_squares && file_idx < 8; i++) {
                    board[rank_idx][file_idx++] = '.';
                }
            } else if (file_idx < 8) {
                board[rank_idx][file_idx++] = c;
            }
        }
    }
}

void Board::set_position(const std::string& fen) {
    if (FENUtils::is_valid_fen(fen)) {
        FENComponents components = FENUtils::parse_fen(fen);
        
        // Set board position
        set_from_fen(components.piece_placement);
        
        // Set game state
        active_color = components.active_color;
        castling_rights = components.castling_rights;
        en_passant_target = components.en_passant_target;
        halfmove_clock = components.halfmove_clock;
        fullmove_number = components.fullmove_number;
        
        // Print the board
        print();
    } else {
        std::cout << "Invalid FEN notation: " << fen << std::endl;
    }
}

void Board::print() const {
    std::cout << "\nCurrent board position:" << std::endl;
    for (int rank = 0; rank < 8; rank++) {
        std::cout << (8 - rank) << "   ";
        for (int file = 0; file < 8; file++) {
            std::cout << board[rank][file] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "\n    a b c d e f g h" << std::endl;
}

char Board::get_piece(int rank, int file) const {
    if (is_valid_square(rank, file)) {
        return board[rank][file];
    }
    return '?'; // Invalid square
}

void Board::set_piece(int rank, int file, char piece) {
    if (is_valid_square(rank, file)) {
        board[rank][file] = piece;
    }
}

bool Board::is_empty(int rank, int file) const {
    return is_valid_square(rank, file) && board[rank][file] == '.';
}

bool Board::is_valid_square(int rank, int file) const {
    return rank >= 0 && rank < 8 && file >= 0 && file < 8;
}

std::string Board::to_fen_piece_placement() const {
    std::string fen = "";
    
    for (int rank = 0; rank < 8; rank++) {
        int empty_count = 0;
        
        for (int file = 0; file < 8; file++) {
            if (board[rank][file] == '.') {
                empty_count++;
            } else {
                if (empty_count > 0) {
                    fen += std::to_string(empty_count);
                    empty_count = 0;
                }
                fen += board[rank][file];
            }
        }
        
        // Add remaining empty squares at end of rank
        if (empty_count > 0) {
            fen += std::to_string(empty_count);
        }
        
        // Add rank separator (except for last rank)
        if (rank < 7) {
            fen += "/";
        }
    }
    
    return fen;
}

std::string Board::to_fen() const {
    FENComponents components;
    components.piece_placement = to_fen_piece_placement();
    components.active_color = active_color;
    components.castling_rights = castling_rights;
    components.en_passant_target = en_passant_target;
    components.halfmove_clock = halfmove_clock;
    components.fullmove_number = fullmove_number;
    
    return FENUtils::create_fen(components);
}

void Board::initialize_empty_board() {
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            board[rank][file] = '.';
        }
    }
}

void Board::initialize_starting_position() {
    // Set starting position
    set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

char Board::file_to_char(int file) const {
    return 'a' + file;
}

int Board::char_to_file(char file) const {
    return file - 'a';
}

char Board::rank_to_char(int rank) const {
    return '8' - rank;
}

int Board::char_to_rank(char rank) const {
    return '8' - rank;
}