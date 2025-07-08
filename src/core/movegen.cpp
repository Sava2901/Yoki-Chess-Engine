#include "movegen.h"
#include <algorithm>
#include <cctype>

namespace yoki {

// Direction vectors
const int MoveGenerator::knight_directions[8][2] = {
    {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
    {1, -2}, {1, 2}, {2, -1}, {2, 1}
};

const int MoveGenerator::king_directions[8][2] = {
    {-1, -1}, {-1, 0}, {-1, 1}, {0, -1},
    {0, 1}, {1, -1}, {1, 0}, {1, 1}
};

const int MoveGenerator::bishop_directions[4][2] = {
    {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
};

const int MoveGenerator::rook_directions[4][2] = {
    {-1, 0}, {1, 0}, {0, -1}, {0, 1}
};

MoveGenerator::MoveGenerator(const Board& board) : board_(board) {}

std::vector<Move> MoveGenerator::generate_legal_moves() const {
    std::vector<Move> pseudo_legal = generate_pseudo_legal_moves();
    std::vector<Move> legal_moves;
    
    for (const Move& move : pseudo_legal) {
        if (is_move_legal(move)) {
            legal_moves.push_back(move);
        }
    }
    
    return legal_moves;
}

std::vector<Move> MoveGenerator::generate_pseudo_legal_moves() const {
    std::vector<Move> moves;
    Color side_to_move = board_.get_side_to_move();
    
    for (int sq = 0; sq < 64; ++sq) {
        if (board_.is_empty(sq) || board_.get_piece_color(sq) != side_to_move) {
            continue;
        }
        
        PieceType piece = board_.get_piece_type(sq);
        std::vector<Move> piece_moves;
        
        switch (piece) {
            case PAWN:
                piece_moves = generate_pawn_moves(sq);
                break;
            case KNIGHT:
                piece_moves = generate_knight_moves(sq);
                break;
            case BISHOP:
                piece_moves = generate_bishop_moves(sq);
                break;
            case ROOK:
                piece_moves = generate_rook_moves(sq);
                break;
            case QUEEN:
                piece_moves = generate_queen_moves(sq);
                break;
            case KING:
                piece_moves = generate_king_moves(sq);
                break;
            default:
                break;
        }
        
        moves.insert(moves.end(), piece_moves.begin(), piece_moves.end());
    }
    
    // Add castling moves
    std::vector<Move> castling_moves = generate_castling_moves();
    moves.insert(moves.end(), castling_moves.begin(), castling_moves.end());
    
    return moves;
}

bool MoveGenerator::is_move_legal(const Move& move) const {
    // Create a copy of the board and make the move
    Board temp_board = board_;
    if (!temp_board.make_move(move)) {
        return false;
    }
    
    // Check if the king is in check after the move
    Color moving_color = board_.get_side_to_move();
    return !temp_board.is_in_check(moving_color);
}

std::vector<Move> MoveGenerator::generate_pawn_moves(Square from) const {
    std::vector<Move> moves;
    Color color = board_.get_piece_color(from);
    int direction = (color == WHITE) ? 1 : -1;
    int start_rank = (color == WHITE) ? 1 : 6;
    int promotion_rank = (color == WHITE) ? 7 : 0;
    
    int file = get_file(from);
    int rank = get_rank(from);
    
    // Forward move
    Square to = make_square(file, rank + direction);
    if (to >= 0 && to < 64 && board_.is_empty(to)) {
        if (get_rank(to) == promotion_rank) {
            add_promotion_moves(moves, from, to);
        } else {
            moves.emplace_back(from, to);
        }
        
        // Double forward move from starting position
        if (rank == start_rank) {
            Square double_to = make_square(file, rank + 2 * direction);
            if (double_to >= 0 && double_to < 64 && board_.is_empty(double_to)) {
                moves.emplace_back(from, double_to);
            }
        }
    }
    
    // Captures
    for (int df = -1; df <= 1; df += 2) {
        if (file + df >= 0 && file + df < 8) {
            Square capture_to = make_square(file + df, rank + direction);
            if (capture_to >= 0 && capture_to < 64) {
                if (!board_.is_empty(capture_to) && board_.get_piece_color(capture_to) != color) {
                    if (get_rank(capture_to) == promotion_rank) {
                        add_promotion_moves(moves, from, capture_to);
                    } else {
                        moves.emplace_back(from, capture_to);
                    }
                }
                
                // En passant capture
                // TODO: Implement en passant logic
            }
        }
    }
    
    return moves;
}

std::vector<Move> MoveGenerator::generate_knight_moves(Square from) const {
    std::vector<Move> moves;
    Color color = board_.get_piece_color(from);
    
    int file = get_file(from);
    int rank = get_rank(from);
    
    for (int i = 0; i < 8; ++i) {
        int new_file = file + knight_directions[i][0];
        int new_rank = rank + knight_directions[i][1];
        
        if (new_file >= 0 && new_file < 8 && new_rank >= 0 && new_rank < 8) {
            Square to = make_square(new_file, new_rank);
            if (is_valid_destination(to, color)) {
                moves.emplace_back(from, to);
            }
        }
    }
    
    return moves;
}

std::vector<Move> MoveGenerator::generate_bishop_moves(Square from) const {
    std::vector<Move> moves;
    Color color = board_.get_piece_color(from);
    
    int file = get_file(from);
    int rank = get_rank(from);
    
    for (int i = 0; i < 4; ++i) {
        int df = bishop_directions[i][0];
        int dr = bishop_directions[i][1];
        
        for (int dist = 1; dist < 8; ++dist) {
            int new_file = file + df * dist;
            int new_rank = rank + dr * dist;
            
            if (new_file < 0 || new_file >= 8 || new_rank < 0 || new_rank >= 8) {
                break;
            }
            
            Square to = make_square(new_file, new_rank);
            
            if (board_.is_empty(to)) {
                moves.emplace_back(from, to);
            } else {
                if (board_.get_piece_color(to) != color) {
                    moves.emplace_back(from, to);
                }
                break; // Blocked by piece
            }
        }
    }
    
    return moves;
}

std::vector<Move> MoveGenerator::generate_rook_moves(Square from) const {
    std::vector<Move> moves;
    Color color = board_.get_piece_color(from);
    
    int file = get_file(from);
    int rank = get_rank(from);
    
    for (int i = 0; i < 4; ++i) {
        int df = rook_directions[i][0];
        int dr = rook_directions[i][1];
        
        for (int dist = 1; dist < 8; ++dist) {
            int new_file = file + df * dist;
            int new_rank = rank + dr * dist;
            
            if (new_file < 0 || new_file >= 8 || new_rank < 0 || new_rank >= 8) {
                break;
            }
            
            Square to = make_square(new_file, new_rank);
            
            if (board_.is_empty(to)) {
                moves.emplace_back(from, to);
            } else {
                if (board_.get_piece_color(to) != color) {
                    moves.emplace_back(from, to);
                }
                break; // Blocked by piece
            }
        }
    }
    
    return moves;
}

std::vector<Move> MoveGenerator::generate_queen_moves(Square from) const {
    std::vector<Move> moves;
    
    // Queen moves like both rook and bishop
    std::vector<Move> rook_moves = generate_rook_moves(from);
    std::vector<Move> bishop_moves = generate_bishop_moves(from);
    
    moves.insert(moves.end(), rook_moves.begin(), rook_moves.end());
    moves.insert(moves.end(), bishop_moves.begin(), bishop_moves.end());
    
    return moves;
}

std::vector<Move> MoveGenerator::generate_king_moves(Square from) const {
    std::vector<Move> moves;
    Color color = board_.get_piece_color(from);
    
    int file = get_file(from);
    int rank = get_rank(from);
    
    for (int i = 0; i < 8; ++i) {
        int new_file = file + king_directions[i][0];
        int new_rank = rank + king_directions[i][1];
        
        if (new_file >= 0 && new_file < 8 && new_rank >= 0 && new_rank < 8) {
            Square to = make_square(new_file, new_rank);
            if (is_valid_destination(to, color)) {
                moves.emplace_back(from, to);
            }
        }
    }
    
    return moves;
}

std::vector<Move> MoveGenerator::generate_castling_moves() const {
    std::vector<Move> moves;
    // TODO: Implement castling move generation
    return moves;
}

bool MoveGenerator::is_square_attacked(Square sq, Color attacker) const {
    // TODO: Implement attack detection
    return false;
}

void MoveGenerator::add_move_if_valid(std::vector<Move>& moves, Square from, Square to) const {
    if (to >= 0 && to < 64 && is_valid_destination(to, board_.get_piece_color(from))) {
        moves.emplace_back(from, to);
    }
}

void MoveGenerator::add_promotion_moves(std::vector<Move>& moves, Square from, Square to) const {
    moves.emplace_back(from, to, QUEEN);
    moves.emplace_back(from, to, ROOK);
    moves.emplace_back(from, to, BISHOP);
    moves.emplace_back(from, to, KNIGHT);
}

bool MoveGenerator::is_path_clear(Square from, Square to) const {
    int file_diff = get_file(to) - get_file(from);
    int rank_diff = get_rank(to) - get_rank(from);
    
    int file_step = (file_diff == 0) ? 0 : (file_diff > 0 ? 1 : -1);
    int rank_step = (rank_diff == 0) ? 0 : (rank_diff > 0 ? 1 : -1);
    
    int current_file = get_file(from) + file_step;
    int current_rank = get_rank(from) + rank_step;
    
    while (current_file != get_file(to) || current_rank != get_rank(to)) {
        Square current_sq = make_square(current_file, current_rank);
        if (!board_.is_empty(current_sq)) {
            return false;
        }
        current_file += file_step;
        current_rank += rank_step;
    }
    
    return true;
}

bool MoveGenerator::is_valid_destination(Square to, Color moving_color) const {
    return board_.is_empty(to) || board_.get_piece_color(to) != moving_color;
}

// Utility functions
bool is_valid_move_format(const std::string& move_str) {
    if (move_str.length() < 4 || move_str.length() > 5) {
        return false;
    }
    
    // Check format: e2e4 or e7e8q
    if (move_str[0] < 'a' || move_str[0] > 'h' ||
        move_str[1] < '1' || move_str[1] > '8' ||
        move_str[2] < 'a' || move_str[2] > 'h' ||
        move_str[3] < '1' || move_str[3] > '8') {
        return false;
    }
    
    if (move_str.length() == 5) {
        char promo = std::tolower(move_str[4]);
        if (promo != 'q' && promo != 'r' && promo != 'b' && promo != 'n') {
            return false;
        }
    }
    
    return true;
}

Move parse_move_string(const std::string& move_str) {
    if (!is_valid_move_format(move_str)) {
        return Move(-1, -1); // Invalid move
    }
    
    Square from = string_to_square(move_str.substr(0, 2));
    Square to = string_to_square(move_str.substr(2, 2));
    
    Move move(from, to);
    
    if (move_str.length() == 5) {
        char promo = std::tolower(move_str[4]);
        switch (promo) {
            case 'q': move.promotion = QUEEN; break;
            case 'r': move.promotion = ROOK; break;
            case 'b': move.promotion = BISHOP; break;
            case 'n': move.promotion = KNIGHT; break;
        }
    }
    
    return move;
}

std::string move_to_algebraic(const Move& move, const Board& board) {
    // TODO: Implement algebraic notation conversion
    return move.to_string();
}

} // namespace yoki