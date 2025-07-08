#include "MoveGenerator.h"
#include <algorithm>
#include <cctype>

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
    generate_en_passant_moves(board, moves);
    
    return moves;
}

MoveList MoveGenerator::generate_legal_moves(const Board& board) {
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

bool MoveGenerator::is_legal_move(const Board& board, const Move& move) {
    // Make the move on a copy of the board
    Board temp_board = make_move_on_copy(board, move);
    
    // Check if our king is in check after the move
    char our_color = board.get_active_color();
    return !is_in_check(temp_board, our_color);
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

bool MoveGenerator::is_square_attacked(const Board& board, int rank, int file, char attacking_color) {
    // Check for pawn attacks
    int pawn_direction = (attacking_color == 'w') ? -1 : 1;
    int pawn_rank = rank - pawn_direction;
    
    if (pawn_rank >= 0 && pawn_rank < 8) {
        // Check diagonal pawn attacks
        if (file > 0) {
            char piece = board.get_piece(pawn_rank, file - 1);
            if (std::tolower(piece) == 'p' && get_piece_color(piece) == attacking_color) {
                return true;
            }
        }
        if (file < 7) {
            char piece = board.get_piece(pawn_rank, file + 1);
            if (std::tolower(piece) == 'p' && get_piece_color(piece) == attacking_color) {
                return true;
            }
        }
    }
    
    // Check for knight attacks
    std::vector<std::pair<int, int>> knight_moves = {
        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
        {1, -2}, {1, 2}, {2, -1}, {2, 1}
    };
    
    for (auto& move : knight_moves) {
        int new_rank = rank + move.first;
        int new_file = file + move.second;
        
        if (new_rank >= 0 && new_rank < 8 && new_file >= 0 && new_file < 8) {
            char piece = board.get_piece(new_rank, new_file);
            if (std::tolower(piece) == 'n' && get_piece_color(piece) == attacking_color) {
                return true;
            }
        }
    }
    
    // Check for sliding piece attacks (rook, bishop, queen)
    std::vector<std::pair<int, int>> rook_directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    std::vector<std::pair<int, int>> bishop_directions = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    
    // Check rook and queen attacks (horizontal/vertical)
    for (auto& dir : rook_directions) {
        for (int i = 1; i < 8; i++) {
            int new_rank = rank + dir.first * i;
            int new_file = file + dir.second * i;
            
            if (new_rank < 0 || new_rank >= 8 || new_file < 0 || new_file >= 8) {
                break;
            }
            
            char piece = board.get_piece(new_rank, new_file);
            if (piece != '.') {
                if (get_piece_color(piece) == attacking_color) {
                    char piece_type = std::tolower(piece);
                    if (piece_type == 'r' || piece_type == 'q') {
                        return true;
                    }
                }
                break; // Piece blocks further movement
            }
        }
    }
    
    // Check bishop and queen attacks (diagonal)
    for (auto& dir : bishop_directions) {
        for (int i = 1; i < 8; i++) {
            int new_rank = rank + dir.first * i;
            int new_file = file + dir.second * i;
            
            if (new_rank < 0 || new_rank >= 8 || new_file < 0 || new_file >= 8) {
                break;
            }
            
            char piece = board.get_piece(new_rank, new_file);
            if (piece != '.') {
                if (get_piece_color(piece) == attacking_color) {
                    char piece_type = std::tolower(piece);
                    if (piece_type == 'b' || piece_type == 'q') {
                        return true;
                    }
                }
                break; // Piece blocks further movement
            }
        }
    }
    
    // Check for king attacks
    std::vector<std::pair<int, int>> king_moves = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},           {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    
    for (auto& move : king_moves) {
        int new_rank = rank + move.first;
        int new_file = file + move.second;
        
        if (new_rank >= 0 && new_rank < 8 && new_file >= 0 && new_file < 8) {
            char piece = board.get_piece(new_rank, new_file);
            if (std::tolower(piece) == 'k' && get_piece_color(piece) == attacking_color) {
                return true;
            }
        }
    }
    
    return false;
}

void MoveGenerator::generate_pawn_moves(const Board& board, int rank, int file, MoveList& moves) {
    char piece = board.get_piece(rank, file);
    char active_color = board.get_active_color();
    bool is_white = (active_color == 'w');
    int direction = is_white ? -1 : 1; // White moves up (decreasing rank), black moves down
    int start_rank = is_white ? 6 : 1;
    int promotion_rank = is_white ? 0 : 7;
    
    // Forward move
    int new_rank = rank + direction;
    if (new_rank >= 0 && new_rank < 8 && board.get_piece(new_rank, file) == '.') {
        if (new_rank == promotion_rank) {
            // Promotion moves
            char promotions[] = {'q', 'r', 'b', 'n'};
            for (char prom : promotions) {
                char prom_piece = is_white ? std::toupper(prom) : prom;
                moves.emplace_back(rank, file, new_rank, file, piece, '.', prom_piece);
            }
        } else {
            moves.emplace_back(rank, file, new_rank, file, piece);
        }
        
        // Double forward move from starting position
        if (rank == start_rank) {
            int double_rank = rank + 2 * direction;
            if (double_rank >= 0 && double_rank < 8 && board.get_piece(double_rank, file) == '.') {
                moves.emplace_back(rank, file, double_rank, file, piece);
            }
        }
    }
    
    // Diagonal captures
    for (int file_offset : {-1, 1}) {
        int new_file = file + file_offset;
        if (new_file >= 0 && new_file < 8) {
            char target = board.get_piece(new_rank, new_file);
            if (target != '.' && is_opponent_piece(target, active_color)) {
                if (new_rank == promotion_rank) {
                    // Promotion captures
                    char promotions[] = {'q', 'r', 'b', 'n'};
                    for (char prom : promotions) {
                        char prom_piece = is_white ? std::toupper(prom) : prom;
                        moves.emplace_back(rank, file, new_rank, new_file, piece, target, prom_piece);
                    }
                } else {
                    moves.emplace_back(rank, file, new_rank, new_file, piece, target);
                }
            }
        }
    }
}

void MoveGenerator::generate_rook_moves(const Board& board, int rank, int file, MoveList& moves) {
    std::vector<std::pair<int, int>> directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    generate_sliding_moves(board, rank, file, directions, moves);
}

void MoveGenerator::generate_knight_moves(const Board& board, int rank, int file, MoveList& moves) {
    char piece = board.get_piece(rank, file);
    char active_color = board.get_active_color();
    
    std::vector<std::pair<int, int>> knight_moves = {
        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
        {1, -2}, {1, 2}, {2, -1}, {2, 1}
    };
    
    for (auto& move : knight_moves) {
        int new_rank = rank + move.first;
        int new_file = file + move.second;
        
        if (new_rank >= 0 && new_rank < 8 && new_file >= 0 && new_file < 8) {
            char target = board.get_piece(new_rank, new_file);
            if (target == '.' || is_opponent_piece(target, active_color)) {
                moves.emplace_back(rank, file, new_rank, new_file, piece, target);
            }
        }
    }
}

void MoveGenerator::generate_bishop_moves(const Board& board, int rank, int file, MoveList& moves) {
    std::vector<std::pair<int, int>> directions = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    generate_sliding_moves(board, rank, file, directions, moves);
}

void MoveGenerator::generate_queen_moves(const Board& board, int rank, int file, MoveList& moves) {
    std::vector<std::pair<int, int>> directions = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},           {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    generate_sliding_moves(board, rank, file, directions, moves);
}

//TODO: Check if the kings are not able to move next to each other
void MoveGenerator::generate_king_moves(const Board& board, int rank, int file, MoveList& moves) {
    char piece = board.get_piece(rank, file);
    char active_color = board.get_active_color();
    
    std::vector<std::pair<int, int>> king_moves = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},           {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    
    for (auto& move : king_moves) {
        int new_rank = rank + move.first;
        int new_file = file + move.second;
        
        if (new_rank >= 0 && new_rank < 8 && new_file >= 0 && new_file < 8) {
            char target = board.get_piece(new_rank, new_file);
            if (target == '.' || is_opponent_piece(target, active_color)) {
                moves.emplace_back(rank, file, new_rank, new_file, piece, target);
            }
        }
    }
}

void MoveGenerator::generate_castling_moves(const Board& board, MoveList& moves) {
    char active_color = board.get_active_color();
    const std::string& castling_rights = board.get_castling_rights();
    
    if (castling_rights == "-") {
        return;
    }
    
    // Check if king is in check (can't castle out of check)
    if (is_in_check(board, active_color)) {
        return;
    }
    
    int king_rank = (active_color == 'w') ? 7 : 0;
    int king_file = 4;
    
    // Kingside castling
    if ((active_color == 'w' && castling_rights.find('K') != std::string::npos) ||
        (active_color == 'b' && castling_rights.find('k') != std::string::npos)) {
        
        // Check if squares between king and rook are empty
        if (board.get_piece(king_rank, 5) == '.' && board.get_piece(king_rank, 6) == '.') {
            // Check if king doesn't pass through check
            if (!is_square_attacked(board, king_rank, 5, (active_color == 'w') ? 'b' : 'w') &&
                !is_square_attacked(board, king_rank, 6, (active_color == 'w') ? 'b' : 'w')) {
                
                char king = board.get_piece(king_rank, king_file);
                moves.emplace_back(king_rank, king_file, king_rank, 6, king, '.', '.', true);
            }
        }
    }
    
    // Queenside castling
    if ((active_color == 'w' && castling_rights.find('Q') != std::string::npos) ||
        (active_color == 'b' && castling_rights.find('q') != std::string::npos)) {
        
        // Check if squares between king and rook are empty
        if (board.get_piece(king_rank, 1) == '.' && board.get_piece(king_rank, 2) == '.' && 
            board.get_piece(king_rank, 3) == '.') {
            // Check if king doesn't pass through check
            if (!is_square_attacked(board, king_rank, 2, (active_color == 'w') ? 'b' : 'w') &&
                !is_square_attacked(board, king_rank, 3, (active_color == 'w') ? 'b' : 'w')) {
                
                char king = board.get_piece(king_rank, king_file);
                moves.emplace_back(king_rank, king_file, king_rank, 2, king, '.', '.', true);
            }
        }
    }
}

void MoveGenerator::generate_en_passant_moves(const Board& board, MoveList& moves) {
    const std::string& en_passant_target = board.get_en_passant_target();
    
    if (en_passant_target == "-") {
        return;
    }
    
    char active_color = board.get_active_color();
    int target_file = en_passant_target[0] - 'a';
    int target_rank = 8 - (en_passant_target[1] - '0');
    
    int pawn_rank = (active_color == 'w') ? target_rank + 1 : target_rank - 1;
    
    // Check for pawns that can capture en passant
    for (int file_offset : {-1, 1}) {
        int pawn_file = target_file + file_offset;
        if (pawn_file >= 0 && pawn_file < 8) {
            char piece = board.get_piece(pawn_rank, pawn_file);
            if (std::tolower(piece) == 'p' && is_own_piece(piece, active_color)) {
                char captured_pawn = (active_color == 'w') ? 'p' : 'P';
                moves.emplace_back(pawn_rank, pawn_file, target_rank, target_file, 
                                 piece, captured_pawn, '.', false, true);
            }
        }
    }
}

void MoveGenerator::generate_sliding_moves(const Board& board, int rank, int file, 
                                         const std::vector<std::pair<int, int>>& directions, 
                                         MoveList& moves) {
    char piece = board.get_piece(rank, file);
    char active_color = board.get_active_color();
    
    for (auto& dir : directions) {
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

bool MoveGenerator::is_own_piece(char piece, char active_color) {
    if (piece == '.') return false;
    return get_piece_color(piece) == active_color;
}

bool MoveGenerator::is_opponent_piece(char piece, char active_color) {
    if (piece == '.') return false;
    return get_piece_color(piece) != active_color;
}

char MoveGenerator::get_piece_color(char piece) {
    return std::isupper(piece) ? 'w' : 'b';
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
    
    return {-1, -1}; // King not found
}

Board MoveGenerator::make_move_on_copy(const Board& board, const Move& move) {
    Board temp_board = board;
    
    // Make the move on the temporary board
    temp_board.set_piece(move.from_rank, move.from_file, '.');
    temp_board.set_piece(move.to_rank, move.to_file, move.piece);
    
    // Handle special moves
    if (move.is_castling) {
        // Move the rook for castling
        int rook_from_file = (move.to_file == 6) ? 7 : 0; // Kingside or queenside
        int rook_to_file = (move.to_file == 6) ? 5 : 3;
        char rook = temp_board.get_piece(move.from_rank, rook_from_file);
        temp_board.set_piece(move.from_rank, rook_from_file, '.');
        temp_board.set_piece(move.from_rank, rook_to_file, rook);
    }
    
    if (move.is_en_passant) {
        // Remove the captured pawn
        int captured_pawn_rank = (temp_board.get_active_color() == 'w') ? move.to_rank + 1 : move.to_rank - 1;
        temp_board.set_piece(captured_pawn_rank, move.to_file, '.');
    }
    
    if (move.promotion_piece != '.') {
        // Handle pawn promotion
        temp_board.set_piece(move.to_rank, move.to_file, move.promotion_piece);
    }
    
    return temp_board;
}