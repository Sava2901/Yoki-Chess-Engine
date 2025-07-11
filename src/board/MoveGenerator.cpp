#include "MoveGenerator.h"
#include <algorithm>
#include <cctype>

// Promotion pieces constant
constexpr std::array<char, 4> PROMOTION_PIECES = {'Q', 'R', 'B', 'N'};

// Template specialization for sliding moves - defined here to avoid linker issues
template<size_t N>
void MoveGenerator::generate_sliding_moves(const Board& board, int rank, int file, 
                                         const std::array<std::pair<int, int>, N>& directions, 
                                         MoveList& moves) {
    char piece = board.get_piece(rank, file);
    char active_color = board.get_active_color();
    
    for (const auto& dir : directions) {
        for (int i = 1; i < 8; i++) {
            int new_rank = rank + dir.first * i;
            int new_file = file + dir.second * i;
            
            if (new_rank < 0 || new_rank >= 8 || new_file < 0 || new_file >= 8) {
                break;
            }
            
            char target = board.get_piece(new_rank, new_file);
            
            if (target == '.') {
                moves.emplace_back(rank, file, new_rank, new_file, piece);
            } else if (is_opponent_piece(target, active_color)) {
                moves.emplace_back(rank, file, new_rank, new_file, piece, target);
                break; // Can't move further after capture
            } else {
                break; // Blocked by own piece
            }
        }
    }
}

MoveList MoveGenerator::generate_pseudo_legal_moves(const Board& board) {
    MoveList moves;
    char active_color = board.get_active_color();
    
    // Iterate through all squares on the board
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            char piece = board.get_piece(rank, file);
            
            // Skip empty squares and opponent pieces
            if (piece == '.' || !is_own_piece(piece, active_color)) {
                continue;
            }
            
            // Generate moves based on piece type
            char piece_type = std::tolower(piece);
            switch (piece_type) {
                case 'p':
                    generate_pawn_moves(board, rank, file, moves);
                    break;
                case 'r':
                    generate_rook_moves(board, rank, file, moves);
                    break;
                case 'n':
                    generate_knight_moves(board, rank, file, moves);
                    break;
                case 'b':
                    generate_bishop_moves(board, rank, file, moves);
                    break;
                case 'q':
                    generate_queen_moves(board, rank, file, moves);
                    break;
                case 'k':
                    generate_king_moves(board, rank, file, moves);
                    break;
            }
        }
    }
    
    // Add special moves
    generate_castling_moves(board, moves);

    return moves;
}

MoveList MoveGenerator::generate_legal_moves(Board& board) {
    MoveList pseudo_legal = generate_pseudo_legal_moves(board);
    MoveList legal_moves;
    
    // Filter out moves that leave the king in check
    for (const Move& move : pseudo_legal) {
        if (is_legal_move(board, move)) {
            legal_moves.push_back(move);
        }
    }
    
    return legal_moves;
}

bool MoveGenerator::is_legal_move(Board& board, const Move& move) {
    // Store original state for undo
    char original_piece_from = board.get_piece(move.from_rank, move.from_file);
    char original_piece_to = board.get_piece(move.to_rank, move.to_file);
    char original_en_passant_captured = 0;
    int original_en_passant_rank = 0;
    char original_rook_from = '.';
    char original_rook_to = '.';
    int rook_from_file = -1, rook_to_file = -1;
    uint8_t original_castling_rights = board.get_castling_rights_bits();
    int8_t original_en_passant_file = board.get_en_passant_file();
    
    char our_color = board.get_active_color();
    
    // Make the move
    board.set_piece(move.from_rank, move.from_file, '.');
    board.set_piece(move.to_rank, move.to_file, move.piece);
    
    // Handle special moves
    if (move.is_castling) {
        // Move the rook for castling using helper logic
        perform_castling_rook_move(board, move, rook_from_file, rook_to_file, original_rook_from, original_rook_to);
    }
    
    if (move.is_en_passant) {
        // Remove the captured pawn
        original_en_passant_rank = (our_color == 'w') ? move.to_rank + 1 : move.to_rank - 1;
        original_en_passant_captured = board.get_piece(original_en_passant_rank, move.to_file);
        board.set_piece(original_en_passant_rank, move.to_file, '.');
    }
    
    if (move.promotion_piece != '.') {
        // Handle pawn promotion
        board.set_piece(move.to_rank, move.to_file, move.promotion_piece);
    }

    // Check if our king is in check after the move
    bool is_legal = !is_in_check(board, our_color);
    
    // Undo the move
    board.set_piece(move.from_rank, move.from_file, original_piece_from);
    board.set_piece(move.to_rank, move.to_file, original_piece_to);
    
    // Undo special moves
    if (move.is_castling) {
        board.set_piece(move.from_rank, rook_from_file, original_rook_from);
        board.set_piece(move.from_rank, rook_to_file, original_rook_to);
    }
    
    if (move.is_en_passant) {
        board.set_piece(original_en_passant_rank, move.to_file, original_en_passant_captured);
    }
    
    // Restore board state (castling rights, en passant)
    board.set_castling_rights_bits(original_castling_rights);
    board.set_en_passant_file(original_en_passant_file);

    return is_legal;
}

bool MoveGenerator::is_in_check(const Board& board, char color) {
    // Find the king position
    auto king_pos = find_king_position(board, color);
    if (king_pos.first == -1) {
        return false; // King not found (shouldn't happen in valid position)
    }
    
    // Check if the king's square is under attack
    char opponent_color = (color == 'w') ? 'b' : 'w';
    return is_square_attacked(board, king_pos.first, king_pos.second, opponent_color);
}

bool MoveGenerator::is_in_check(const Board& board, char color, const std::pair<int, int>& king_pos) {
    // Check if the king's square is under attack
    char opponent_color = (color == 'w') ? 'b' : 'w';
    return is_square_attacked(board, king_pos.first, king_pos.second, opponent_color);
}

bool MoveGenerator::is_square_attacked(const Board& board, int rank, int file, char attacking_color) {
    // Check for pawn attacks
    int pawn_direction = (attacking_color == 'w') ? -1 : 1;
    char attacking_pawn = (attacking_color == 'w') ? 'P' : 'p';
    
    // Check diagonal pawn attacks
    for (int file_offset : {-1, 1}) {
        int attack_rank = rank - pawn_direction;
        int attack_file = file + file_offset;
        
        if (is_valid_square(attack_rank, attack_file)) {
            if (board.get_piece(attack_rank, attack_file) == attacking_pawn) {
                return true;
            }
        }
    }
    
    // Check for knight attacks
    char attacking_knight = (attacking_color == 'w') ? 'N' : 'n';
    
    for (const auto& move : KNIGHT_MOVES) {
        int attack_rank = rank + move.first;
        int attack_file = file + move.second;
        
        if (is_valid_square(attack_rank, attack_file)) {
            if (board.get_piece(attack_rank, attack_file) == attacking_knight) {
                return true;
            }
        }
    }
    
    // Check for sliding piece attacks (rook, bishop, queen)
    char attacking_rook = (attacking_color == 'w') ? 'R' : 'r';
    char attacking_bishop = (attacking_color == 'w') ? 'B' : 'b';
    char attacking_queen = (attacking_color == 'w') ? 'Q' : 'q';
    
    // Rook/Queen horizontal and vertical
    for (const auto& dir : ROOK_DIRECTIONS) {
        for (int i = 1; i < 8; i++) {
            int attack_rank = rank + dir.first * i;
            int attack_file = file + dir.second * i;
            
            if (!is_valid_square(attack_rank, attack_file)) {
                break;
            }
            
            char piece = board.get_piece(attack_rank, attack_file);
            if (piece == attacking_rook || piece == attacking_queen) {
                return true;
            }
            if (piece != '.') {
                break; // Blocked by another piece
            }
        }
    }
    
    // Bishop/Queen diagonal
    for (const auto& dir : BISHOP_DIRECTIONS) {
        for (int i = 1; i < 8; i++) {
            int attack_rank = rank + dir.first * i;
            int attack_file = file + dir.second * i;
            
            if (!is_valid_square(attack_rank, attack_file)) {
                break;
            }
            
            char piece = board.get_piece(attack_rank, attack_file);
            if (piece == attacking_bishop || piece == attacking_queen) {
                return true;
            }
            if (piece != '.') {
                break; // Blocked by another piece
            }
        }
    }
    
    // Check for king attacks
    char attacking_king = (attacking_color == 'w') ? 'K' : 'k';
    
    for (const auto& move : KING_MOVES) {
        int attack_rank = rank + move.first;
        int attack_file = file + move.second;
        
        if (is_valid_square(attack_rank, attack_file)) {
            if (board.get_piece(attack_rank, attack_file) == attacking_king) {
                return true;
            }
        }
    }
    
    return false;
}

void MoveGenerator::generate_pawn_moves(const Board& board, int rank, int file, MoveList& moves) {
    char piece = board.get_piece(rank, file);
    char active_color = board.get_active_color();
    
    int direction = (active_color == 'w') ? -1 : 1;
    int start_rank = (active_color == 'w') ? 6 : 1;
    int promotion_rank = (active_color == 'w') ? 0 : 7;
    
    // Forward move
    int new_rank = rank + direction;
    if (is_valid_square(new_rank, file) && board.get_piece(new_rank, file) == '.') {
        if (new_rank == promotion_rank) {
            // Promotion moves
            for (char promo : PROMOTION_PIECES) {
                moves.emplace_back(rank, file, new_rank, file, piece, '.', promo);
            }
        } else {
            moves.emplace_back(rank, file, new_rank, file, piece);
        }
        
        // Double forward move from starting position
        if (rank == start_rank) {
            int double_rank = rank + 2 * direction;
            if (is_valid_square(double_rank, file) && board.get_piece(double_rank, file) == '.') {
                moves.emplace_back(rank, file, double_rank, file, piece);
            }
        }
    }
    
    // Capture moves
    for (int file_offset : {-1, 1}) {
        int new_file = file + file_offset;
        if (is_valid_square(rank + direction, new_file)) {
            int capture_rank = rank + direction;
            char target = board.get_piece(capture_rank, new_file);
            if (target != '.' && is_opponent_piece(target, active_color)) {
                if (capture_rank == promotion_rank) {
                    // Promotion captures
                    for (char promo : PROMOTION_PIECES) {
                        moves.emplace_back(rank, file, capture_rank, new_file, piece, target, promo);
                    }
                } else {
                    moves.emplace_back(rank, file, capture_rank, new_file, piece, target);
                }
            }
        }
    }
    
    // En passant
    int8_t en_passant_file = board.get_en_passant_file();
    if (en_passant_file != -1) {
        int ep_rank = (active_color == 'w') ? 2 : 5;
        
        if (abs(file - en_passant_file) == 1 && rank + direction == ep_rank) {
            Move en_passant_move(rank, file, ep_rank, en_passant_file, piece, '.');
            en_passant_move.is_en_passant = true;
            moves.push_back(en_passant_move);
        }
    }
}

void MoveGenerator::generate_rook_moves(const Board& board, int rank, int file, MoveList& moves) {
    generate_sliding_moves(board, rank, file, ROOK_DIRECTIONS, moves);
}

void MoveGenerator::generate_knight_moves(const Board& board, int rank, int file, MoveList& moves) {
    char piece = board.get_piece(rank, file);
    char active_color = board.get_active_color();
    
    for (const auto& move : KNIGHT_MOVES) {
        int new_rank = rank + move.first;
        int new_file = file + move.second;
        
        if (is_valid_square(new_rank, new_file)) {
            char target = board.get_piece(new_rank, new_file);
            if (target == '.' || is_opponent_piece(target, active_color)) {
                moves.emplace_back(rank, file, new_rank, new_file, piece, target);
            }
        }
    }
}

void MoveGenerator::generate_bishop_moves(const Board& board, int rank, int file, MoveList& moves) {
    generate_sliding_moves(board, rank, file, BISHOP_DIRECTIONS, moves);
}

void MoveGenerator::generate_queen_moves(const Board& board, int rank, int file, MoveList& moves) {
    generate_sliding_moves(board, rank, file, QUEEN_DIRECTIONS, moves);
}

void MoveGenerator::generate_king_moves(const Board& board, int rank, int file, MoveList& moves) {
    char piece = board.get_piece(rank, file);
    char active_color = board.get_active_color();
    
    for (const auto& move : KING_MOVES) {
        int new_rank = rank + move.first;
        int new_file = file + move.second;
        
        if (is_valid_square(new_rank, new_file)) {
            char target = board.get_piece(new_rank, new_file);
            if (target == '.' || is_opponent_piece(target, active_color)) {
                moves.emplace_back(rank, file, new_rank, new_file, piece, target);
            }
        }
    }
}

void MoveGenerator::generate_castling_moves(const Board& board, MoveList& moves) {
    char active_color = board.get_active_color();
    uint8_t castling_rights = board.get_castling_rights_bits();
    
    if (active_color == 'w') {
        // White kingside castling
        if (castling_rights & WHITE_KINGSIDE) {
            if (board.get_piece(7, 5) == '.' && board.get_piece(7, 6) == '.' &&
                !is_square_attacked(board, 7, 4, 'b') &&
                !is_square_attacked(board, 7, 5, 'b') &&
                !is_square_attacked(board, 7, 6, 'b')) {
                Move castle_move(7, 4, 7, 6, 'K');
                castle_move.is_castling = true;
                moves.push_back(castle_move);
            }
        }
        
        // White queenside castling
        if (castling_rights & WHITE_QUEENSIDE) {
            if (board.get_piece(7, 1) == '.' && board.get_piece(7, 2) == '.' && board.get_piece(7, 3) == '.' &&
                !is_square_attacked(board, 7, 4, 'b') &&
                !is_square_attacked(board, 7, 3, 'b') &&
                !is_square_attacked(board, 7, 2, 'b')) {
                Move castle_move(7, 4, 7, 2, 'K');
                castle_move.is_castling = true;
                moves.push_back(castle_move);
            }
        }
    } else {
        // Black kingside castling
        if (castling_rights & BLACK_KINGSIDE) {
            if (board.get_piece(0, 5) == '.' && board.get_piece(0, 6) == '.' &&
                !is_square_attacked(board, 0, 4, 'w') &&
                !is_square_attacked(board, 0, 5, 'w') &&
                !is_square_attacked(board, 0, 6, 'w')) {
                Move castle_move(0, 4, 0, 6, 'k');
                castle_move.is_castling = true;
                moves.push_back(castle_move);
            }
        }
        
        // Black queenside castling
        if (castling_rights & BLACK_QUEENSIDE) {
            if (board.get_piece(0, 1) == '.' && board.get_piece(0, 2) == '.' && board.get_piece(0, 3) == '.' &&
                !is_square_attacked(board, 0, 4, 'w') &&
                !is_square_attacked(board, 0, 3, 'w') &&
                !is_square_attacked(board, 0, 2, 'w')) {
                Move castle_move(0, 4, 0, 2, 'k');
                castle_move.is_castling = true;
                moves.push_back(castle_move);
            }
        }
    }
}

void MoveGenerator::perform_castling_rook_move(Board& board, const Move& move, 
                                             int& rook_from_file, int& rook_to_file,
                                             char& original_rook_from, char& original_rook_to) {
    // Determine rook movement based on castling type
    rook_from_file = (move.to_file == 6) ? 7 : 0; // Kingside or queenside
    rook_to_file = (move.to_file == 6) ? 5 : 3;
    
    // Store original rook positions
    original_rook_from = board.get_piece(move.from_rank, rook_from_file);
    original_rook_to = board.get_piece(move.from_rank, rook_to_file);
    
    // Move the rook
    board.set_piece(move.from_rank, rook_from_file, '.');
    board.set_piece(move.from_rank, rook_to_file, original_rook_from);
}

std::pair<int, int> MoveGenerator::find_king_position(const Board& board, char color) {
    char king = (color == 'w') ? 'K' : 'k';
    
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            if (board.get_piece(rank, file) == king) {
                return {rank, file};
            }
        }
    }
    
    return {-1, -1}; // King not found (should not happen in valid position)
}