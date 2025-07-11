#include "Board.h"
#include "MoveGenerator.h"
#include "FENUtils.h"
#include <iostream>
#include <sstream>
#include <cctype>
#include <cmath>

Board::Board() {
    initialize_starting_position();
}

Board::~Board() {
    // Nothing to clean up for now
}

void Board::clear() {
    initialize_empty_board();
    active_color = 'w';
    castling_rights_bits = NO_CASTLING;
    en_passant_file = -1;
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
    
    for (int rank_idx = 0; rank_idx < 8 && rank_idx < static_cast<int>(ranks.size()); rank_idx++) {
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
        
        // Set game state (optimized)
        active_color = components.active_color;
        set_castling_rights(components.castling_rights);
        set_en_passant_target(components.en_passant_target);
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
    components.castling_rights = get_castling_rights();
    components.en_passant_target = get_en_passant_target();
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
    // Optimized starting position setup
    initialize_empty_board();
    
    // Set pieces directly for better performance
    const char* white_pieces = "RNBQKBNR";
    const char* black_pieces = "rnbqkbnr";
    
    // Set back ranks
    for (int file = 0; file < 8; file++) {
        board[0][file] = black_pieces[file];
        board[7][file] = white_pieces[file];
    }
    
    // Set pawns
    for (int file = 0; file < 8; file++) {
        board[1][file] = 'p';
        board[6][file] = 'P';
    }
    
    // Set game state
    active_color = 'w';
    castling_rights_bits = ALL_CASTLING;
    en_passant_file = -1;
    halfmove_clock = 0;
    fullmove_number = 1;
}

// Legacy string methods for compatibility
std::string Board::get_castling_rights() const {
    if (castling_rights_bits == NO_CASTLING) return "-";
    
    std::string result;
    if (castling_rights_bits & WHITE_KINGSIDE) result += 'K';
    if (castling_rights_bits & WHITE_QUEENSIDE) result += 'Q';
    if (castling_rights_bits & BLACK_KINGSIDE) result += 'k';
    if (castling_rights_bits & BLACK_QUEENSIDE) result += 'q';
    
    return result;
}

std::string Board::get_en_passant_target() const {
    if (en_passant_file == -1) return "-";
    
    char file_char = 'a' + en_passant_file;
    char rank_char = (active_color == 'w') ? '6' : '3';
    return std::string(1, file_char) + std::string(1, rank_char);
}

void Board::set_castling_rights(const std::string& rights) {
    castling_rights_bits = NO_CASTLING;
    
    for (char c : rights) {
        switch (c) {
            case 'K': castling_rights_bits |= WHITE_KINGSIDE; break;
            case 'Q': castling_rights_bits |= WHITE_QUEENSIDE; break;
            case 'k': castling_rights_bits |= BLACK_KINGSIDE; break;
            case 'q': castling_rights_bits |= BLACK_QUEENSIDE; break;
        }
    }
}

void Board::set_en_passant_target(const std::string& target) {
    if (target == "-" || target.empty()) {
        en_passant_file = -1;
    } else {
        en_passant_file = target[0] - 'a';
    }
}

// Move validation
bool Board::is_valid_move(const Move& move) const {
    // Basic bounds checking
    if (!move.is_valid()) {
        return false;
    }
    
    // Use MoveGenerator to check if this move is in the list of legal moves
    MoveList legal_moves = MoveGenerator::generate_legal_moves(*this);
    for (const Move& legal_move : legal_moves) {
        if (move == legal_move) {
            return true;
        }
    }
    
    return false;
}

// Make a move on the board
bool Board::make_move(const Move& move) {
    if (!is_valid_move(move)) {
        return false;
    }
    
    // Store captured piece for undo (will be updated in BoardState when needed)
    char captured = get_piece(move.to_rank, move.to_file);
    
    // Handle special moves first
    if (move.is_castling) {
        // Castling: move king and rook
        if (move.to_file == 6) { // Kingside castling
            set_piece(move.from_rank, move.from_file, '.');
            set_piece(move.to_rank, move.to_file, move.piece);
            // Move rook
            char rook = get_piece(move.from_rank, 7);
            set_piece(move.from_rank, 7, '.');
            set_piece(move.from_rank, 5, rook);
        } else if (move.to_file == 2) { // Queenside castling
            set_piece(move.from_rank, move.from_file, '.');
            set_piece(move.to_rank, move.to_file, move.piece);
            // Move rook
            char rook = get_piece(move.from_rank, 0);
            set_piece(move.from_rank, 0, '.');
            set_piece(move.from_rank, 3, rook);
        }
    } else if (move.is_en_passant) {
        // En passant: move pawn and remove captured pawn
        set_piece(move.from_rank, move.from_file, '.');
        set_piece(move.to_rank, move.to_file, move.piece);
        // Remove captured pawn
        int captured_rank = (active_color == 'w') ? move.to_rank + 1 : move.to_rank - 1;
        set_piece(captured_rank, move.to_file, '.');
    } else {
        // Normal move
        set_piece(move.from_rank, move.from_file, '.');
        if (move.promotion_piece != '.') {
            // Promotion
            set_piece(move.to_rank, move.to_file, move.promotion_piece);
        } else {
            set_piece(move.to_rank, move.to_file, move.piece);
        }
    }
    
    // Update game state
    
    // Update castling rights (optimized with bitwise operations)
    if (move.piece == 'K') {
        // White king moved - remove all white castling rights
        remove_castling_rights(WHITE_CASTLING);
    } else if (move.piece == 'k') {
        // Black king moved - remove all black castling rights
        remove_castling_rights(BLACK_CASTLING);
    } else if (move.piece == 'R') {
        // White rook moved
        if (move.from_rank == 7 && move.from_file == 0) {
            remove_castling_rights(WHITE_QUEENSIDE);
        } else if (move.from_rank == 7 && move.from_file == 7) {
            remove_castling_rights(WHITE_KINGSIDE);
        }
    } else if (move.piece == 'r') {
        // Black rook moved
        if (move.from_rank == 0 && move.from_file == 0) {
            remove_castling_rights(BLACK_QUEENSIDE);
        } else if (move.from_rank == 0 && move.from_file == 7) {
            remove_castling_rights(BLACK_KINGSIDE);
        }
    }
    
    // Check if rook was captured
    if (captured == 'R') {
        if (move.to_rank == 7 && move.to_file == 0) {
            remove_castling_rights(WHITE_QUEENSIDE);
        } else if (move.to_rank == 7 && move.to_file == 7) {
            remove_castling_rights(WHITE_KINGSIDE);
        }
    } else if (captured == 'r') {
        if (move.to_rank == 0 && move.to_file == 0) {
            remove_castling_rights(BLACK_QUEENSIDE);
        } else if (move.to_rank == 0 && move.to_file == 7) {
            remove_castling_rights(BLACK_KINGSIDE);
        }
    }
    
    // Update en passant target (optimized)
    if (std::tolower(move.piece) == 'p' && abs(move.to_rank - move.from_rank) == 2) {
        // Pawn moved two squares, set en passant file
        en_passant_file = move.from_file;
    } else {
        en_passant_file = -1;
    }
    
    // Update halfmove clock
    if (std::tolower(move.piece) == 'p' || move.captured_piece != '.') {
        halfmove_clock = 0; // Reset on pawn move or capture
    } else {
        halfmove_clock++;
    }
    
    // Update fullmove number
    if (active_color == 'b') {
        fullmove_number++;
    }
    
    // Switch active color
    active_color = (active_color == 'w') ? 'b' : 'w';
    
    return true;
}

// Make a move on the board with state capture for undo
bool Board::make_move(const Move& move, BoardState& state) {
    if (!is_valid_move(move)) {
        return false;
    }
    
    // Store current state for undo
    state = get_board_state();
    
    // Store captured piece
    if (move.is_en_passant) {
        // For en passant, the captured piece is on a different square
        int captured_rank = (active_color == 'w') ? move.to_rank + 1 : move.to_rank - 1;
        state.captured_piece = get_piece(captured_rank, move.to_file);
    } else {
        state.captured_piece = get_piece(move.to_rank, move.to_file);
    }
    
    // Handle special moves first
    if (move.is_castling) {
        // Castling: move king and rook
        if (move.to_file == 6) { // Kingside castling
            set_piece(move.from_rank, move.from_file, '.');
            set_piece(move.to_rank, move.to_file, move.piece);
            // Move rook
            char rook = get_piece(move.from_rank, 7);
            set_piece(move.from_rank, 7, '.');
            set_piece(move.from_rank, 5, rook);
        } else if (move.to_file == 2) { // Queenside castling
            set_piece(move.from_rank, move.from_file, '.');
            set_piece(move.to_rank, move.to_file, move.piece);
            // Move rook
            char rook = get_piece(move.from_rank, 0);
            set_piece(move.from_rank, 0, '.');
            set_piece(move.from_rank, 3, rook);
        }
    } else if (move.is_en_passant) {
        // En passant: move pawn and remove captured pawn
        set_piece(move.from_rank, move.from_file, '.');
        set_piece(move.to_rank, move.to_file, move.piece);
        // Remove captured pawn
        int captured_rank = (active_color == 'w') ? move.to_rank + 1 : move.to_rank - 1;
        set_piece(captured_rank, move.to_file, '.');
    } else {
        // Normal move
        set_piece(move.from_rank, move.from_file, '.');
        if (move.promotion_piece != '.') {
            // Promotion
            set_piece(move.to_rank, move.to_file, move.promotion_piece);
        } else {
            set_piece(move.to_rank, move.to_file, move.piece);
        }
    }
    
    // Update game state
    
    // Update castling rights (optimized with bitwise operations)
    if (move.piece == 'K') {
        // White king moved - remove all white castling rights
        remove_castling_rights(WHITE_CASTLING);
    } else if (move.piece == 'k') {
        // Black king moved - remove all black castling rights
        remove_castling_rights(BLACK_CASTLING);
    } else if (move.piece == 'R') {
        // White rook moved
        if (move.from_rank == 7 && move.from_file == 0) {
            remove_castling_rights(WHITE_QUEENSIDE);
        } else if (move.from_rank == 7 && move.from_file == 7) {
            remove_castling_rights(WHITE_KINGSIDE);
        }
    } else if (move.piece == 'r') {
        // Black rook moved
        if (move.from_rank == 0 && move.from_file == 0) {
            remove_castling_rights(BLACK_QUEENSIDE);
        } else if (move.from_rank == 0 && move.from_file == 7) {
            remove_castling_rights(BLACK_KINGSIDE);
        }
    }
    
    // Check if rook was captured
    if (state.captured_piece == 'R') {
        if (move.to_rank == 7 && move.to_file == 0) {
            remove_castling_rights(WHITE_QUEENSIDE);
        } else if (move.to_rank == 7 && move.to_file == 7) {
            remove_castling_rights(WHITE_KINGSIDE);
        }
    } else if (state.captured_piece == 'r') {
        if (move.to_rank == 0 && move.to_file == 0) {
            remove_castling_rights(BLACK_QUEENSIDE);
        } else if (move.to_rank == 0 && move.to_file == 7) {
            remove_castling_rights(BLACK_KINGSIDE);
        }
    }
    
    // Update en passant target (optimized)
    if (std::tolower(move.piece) == 'p' && abs(move.to_rank - move.from_rank) == 2) {
        // Pawn moved two squares, set en passant file
        en_passant_file = move.from_file;
    } else {
        en_passant_file = -1;
    }
    
    // Update halfmove clock
    if (std::tolower(move.piece) == 'p' || state.captured_piece != '.') {
        halfmove_clock = 0; // Reset on pawn move or capture
    } else {
        halfmove_clock++;
    }
    
    // Update fullmove number
    if (active_color == 'b') {
        fullmove_number++;
    }
    
    // Switch active color
    active_color = (active_color == 'w') ? 'b' : 'w';
    
    return true;
}

// Undo a move (optimized)
void Board::undo_move(const Move& move, const BoardState& state) {
    // Restore game state (optimized)
    active_color = state.active_color;
    castling_rights_bits = state.castling_rights_bits;
    en_passant_file = state.en_passant_file;
    halfmove_clock = state.halfmove_clock;
    fullmove_number = state.fullmove_number;
    
    // Undo the move on the board
    if (move.is_castling) {
        // Undo castling: move king and rook back
        if (move.to_file == 6) { // Kingside castling
            set_piece(move.to_rank, move.to_file, '.');
            set_piece(move.from_rank, move.from_file, move.piece);
            // Move rook back
            char rook = get_piece(move.from_rank, 5);
            set_piece(move.from_rank, 5, '.');
            set_piece(move.from_rank, 7, rook);
        } else if (move.to_file == 2) { // Queenside castling
            set_piece(move.to_rank, move.to_file, '.');
            set_piece(move.from_rank, move.from_file, move.piece);
            // Move rook back
            char rook = get_piece(move.from_rank, 3);
            set_piece(move.from_rank, 3, '.');
            set_piece(move.from_rank, 0, rook);
        }
    } else if (move.is_en_passant) {
        // Undo en passant: move pawn back and restore captured pawn
        set_piece(move.to_rank, move.to_file, '.');
        set_piece(move.from_rank, move.from_file, move.piece);
        // Restore captured pawn
        int captured_rank = (state.active_color == 'w') ? move.to_rank + 1 : move.to_rank - 1;
        set_piece(captured_rank, move.to_file, state.captured_piece);
    } else {
        // Undo normal move
        set_piece(move.to_rank, move.to_file, state.captured_piece);
        set_piece(move.from_rank, move.from_file, move.piece);
    }
}

// Get current board state for undo (optimized)
Board::BoardState Board::get_board_state() const {
    BoardState state;
    state.active_color = active_color;
    state.castling_rights_bits = castling_rights_bits;
    state.en_passant_file = en_passant_file;
    state.halfmove_clock = halfmove_clock;
    state.fullmove_number = fullmove_number;
    state.captured_piece = '.';
    return state;
}

// Create move from algebraic notation (e.g., "e2e4", "e7e8q")
Move Board::create_move_from_algebraic(const std::string& algebraic) const {
    if (algebraic.length() < 4) {
        return Move(); // Invalid move
    }
    
    // Parse from and to squares
    int from_file = char_to_file(algebraic[0]);
    int from_rank = char_to_rank(algebraic[1]);
    int to_file = char_to_file(algebraic[2]);
    int to_rank = char_to_rank(algebraic[3]);
    
    // Validate coordinates
    if (!is_valid_square(from_rank, from_file) || !is_valid_square(to_rank, to_file)) {
        return Move(); // Invalid coordinates
    }
    
    // Get the piece being moved
    char piece = get_piece(from_rank, from_file);
    if (piece == '.') {
        return Move(); // No piece on from square
    }
    
    // Get captured piece (if any)
    char captured_piece = get_piece(to_rank, to_file);
    
    // Check for promotion
    char promotion_piece = '.';
    if (algebraic.length() >= 5) {
        promotion_piece = std::toupper(algebraic[4]);
    }
    
    // Check for castling
    bool is_castling = false;
    if (std::tolower(piece) == 'k' && abs(to_file - from_file) == 2) {
        is_castling = true;
    }
    
    // Check for en passant (optimized)
    bool is_en_passant = false;
    if (std::tolower(piece) == 'p' && captured_piece == '.' && from_file != to_file) {
        // Pawn moving diagonally to empty square - must be en passant
        if (get_en_passant_file() != -1 && to_file == get_en_passant_file()) {
            // Calculate expected en passant target rank
            int expected_rank = (active_color == 'w') ? 2 : 5;
            if (to_rank == expected_rank) {
                is_en_passant = true;
                // The captured piece is actually on a different square
                int captured_rank = (active_color == 'w') ? to_rank + 1 : to_rank - 1;
                captured_piece = get_piece(captured_rank, to_file);
            }
        }
    }
    
    return Move(from_rank, from_file, to_rank, to_file, piece, captured_piece, 
                promotion_piece, is_castling, is_en_passant);
}