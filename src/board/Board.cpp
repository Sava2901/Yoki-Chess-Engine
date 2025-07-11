#include "Board.h"
#include "MoveGenerator.h"
#include "FENUtils.h"
#include <iostream>
#include <sstream>
#include <cctype>
#include <cmath>
#include <algorithm>

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

void Board::set_from_fen(std::string_view piece_placement) {
    initialize_empty_board();
    
    std::vector<std::string> ranks;
    std::istringstream iss(static_cast<std::string>(piece_placement));
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

void Board::set_position(std::string_view fen) {
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
        // std::cout << "Invalid FEN notation: " << fen << std::endl;
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

void Board::set_castling_rights(std::string_view rights) {
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

void Board::set_en_passant_target(std::string_view target) {
    if (target == "-" || target.empty()) {
        en_passant_file = -1;
    } else {
        en_passant_file = target[0] - 'a';
    }
}

bool Board::is_legal_move(const Move& move) {
    std::vector<Move> legal_moves = MoveGenerator::generate_legal_moves(*this);
    return std::any_of(legal_moves.begin(), legal_moves.end(), [&](const Move& m) {
        return m == move;
    });
}

// Create move from algebraic notation (e.g., "e2e4", "e7e8q")
Move Board::create_move_from_algebraic(std::string_view algebraic) const {
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

// Make a move on the board
bool Board::make_move(const Move& move) {
    // Save everything needed for undo
    MoveUndoData undo_data;
    undo_data.active_color = active_color;
    undo_data.castling_rights_bits = castling_rights_bits;
    undo_data.en_passant_file = en_passant_file;
    undo_data.halfmove_clock = halfmove_clock;
    undo_data.fullmove_number = fullmove_number;
    undo_data.move = move;
    undo_data.was_castling = move.is_castling;
    undo_data.was_en_passant = move.is_en_passant;
    undo_data.promotion_piece = move.promotion_piece;
    
    // Store captured piece
    if (move.is_en_passant) {
        int captured_rank = (active_color == 'w') ? move.to_rank + 1 : move.to_rank - 1;
        undo_data.captured_piece = get_piece(captured_rank, move.to_file);
    } else {
        undo_data.captured_piece = get_piece(move.to_rank, move.to_file);
    }
    
    // Apply the move (including castling, en passant, etc.)
    apply_move(move);
    
    // Check if move is legal (king not in check)
    bool legal = !MoveGenerator::is_in_check(*this, (undo_data.active_color == 'w') ? 'b' : 'w');
    
    if (!legal) {
        // Restore board state
        undo_move(undo_data);
    }
    
    return legal;
}

// New make_move function with MoveUndoData
bool Board::make_move(const Move& move, MoveUndoData& undo_data) {
    // Save everything needed for undo
    undo_data.active_color = active_color;
    undo_data.castling_rights_bits = castling_rights_bits;
    undo_data.en_passant_file = en_passant_file;
    undo_data.halfmove_clock = halfmove_clock;
    undo_data.fullmove_number = fullmove_number;
    undo_data.move = move;
    undo_data.was_castling = move.is_castling;
    undo_data.was_en_passant = move.is_en_passant;
    undo_data.promotion_piece = move.promotion_piece;
    
    // Store captured piece
    if (move.is_en_passant) {
        int captured_rank = (active_color == 'w') ? move.to_rank + 1 : move.to_rank - 1;
        undo_data.captured_piece = get_piece(captured_rank, move.to_file);
    } else {
        undo_data.captured_piece = get_piece(move.to_rank, move.to_file);
    }
    
    // Apply the move (including castling, en passant, etc.)
    apply_move(move);

    // TODO: this does not have to check for checks in every move, filter them before checking.
    // Check if move is legal (king not in check)
    bool legal = !MoveGenerator::is_in_check(*this, (undo_data.active_color == 'w') ? 'b' : 'w');
    
    if (!legal) {
        // Restore board state
        undo_move(undo_data);
        return false;
    }
    
    return true;
}

// New undo_move function with MoveUndoData
void Board::undo_move(const MoveUndoData& undo_data) {
    const Move& move = undo_data.move;
    
    // Reverse the piece movements based on move type
    if (undo_data.was_castling) {
        // Undo castling: move king and rook back
        if (move.to_file == 6) { // Kingside castling
            // Move king back
            set_piece(move.from_rank, move.from_file, move.piece);
            set_piece(move.to_rank, move.to_file, '.');
            // Move rook back
            char rook = get_piece(move.from_rank, 5);
            set_piece(move.from_rank, 5, '.');
            set_piece(move.from_rank, 7, rook);
        } else if (move.to_file == 2) { // Queenside castling
            // Move king back
            set_piece(move.from_rank, move.from_file, move.piece);
            set_piece(move.to_rank, move.to_file, '.');
            // Move rook back
            char rook = get_piece(move.from_rank, 3);
            set_piece(move.from_rank, 3, '.');
            set_piece(move.from_rank, 0, rook);
        }
    } else if (undo_data.was_en_passant) {
        // Undo en passant: move pawn back and restore captured pawn
        set_piece(move.from_rank, move.from_file, move.piece);
        set_piece(move.to_rank, move.to_file, '.');
        // Restore captured pawn
        int captured_rank = (undo_data.active_color == 'w') ? move.to_rank + 1 : move.to_rank - 1;
        set_piece(captured_rank, move.to_file, undo_data.captured_piece);
    } else {
        // Undo normal move or promotion
        if (undo_data.promotion_piece != '.') {
            // Undo promotion: restore original pawn
            set_piece(move.from_rank, move.from_file, move.piece);
        } else {
            // Undo normal move: move piece back
            char piece_on_to_square = get_piece(move.to_rank, move.to_file);
            set_piece(move.from_rank, move.from_file, piece_on_to_square);
        }
        
        // Restore captured piece (if any)
        set_piece(move.to_rank, move.to_file, undo_data.captured_piece);
    }
    
    // Restore game state
    active_color = undo_data.active_color;
    castling_rights_bits = undo_data.castling_rights_bits;
    en_passant_file = undo_data.en_passant_file;
    halfmove_clock = undo_data.halfmove_clock;
    fullmove_number = undo_data.fullmove_number;
}

// Helper function to apply move without legality check
void Board::apply_move(const Move& move) {
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
    char captured = (move.is_en_passant) ? 
        get_piece((active_color == 'w') ? move.to_rank + 1 : move.to_rank - 1, move.to_file) :
        get_piece(move.to_rank, move.to_file);
    
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
    
    // Check if rook was captured (use the captured piece from before the move)
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
}

// IMPORTANT NOTE:
// Probably the search will function the by checking the full list of moves. (in the first phase before pruning)
// So there might be a need to create an apply_move() function that takes the move and the undo_data as parameters for undoing.
// It does not need to check legality, just apply the move to the board state.