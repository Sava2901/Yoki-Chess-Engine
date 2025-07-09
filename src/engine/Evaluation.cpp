#include "Evaluation.h"
#include "../board/MoveGenerator.h"
#include <cctype>
#include <algorithm>

// Piece-square tables (from white's perspective)
const std::array<std::array<int, 8>, 8> Evaluation::PAWN_TABLE = {{
    {{  0,  0,  0,  0,  0,  0,  0,  0 }},
    {{ 50, 50, 50, 50, 50, 50, 50, 50 }},
    {{ 10, 10, 20, 30, 30, 20, 10, 10 }},
    {{  5,  5, 10, 25, 25, 10,  5,  5 }},
    {{  0,  0,  0, 20, 20,  0,  0,  0 }},
    {{  5, -5,-10,  0,  0,-10, -5,  5 }},
    {{  5, 10, 10,-20,-20, 10, 10,  5 }},
    {{  0,  0,  0,  0,  0,  0,  0,  0 }}
}};

const std::array<std::array<int, 8>, 8> Evaluation::KNIGHT_TABLE = {{
    {{ -50,-40,-30,-30,-30,-30,-40,-50 }},
    {{ -40,-20,  0,  0,  0,  0,-20,-40 }},
    {{ -30,  0, 10, 15, 15, 10,  0,-30 }},
    {{ -30,  5, 15, 20, 20, 15,  5,-30 }},
    {{ -30,  0, 15, 20, 20, 15,  0,-30 }},
    {{ -30,  5, 10, 15, 15, 10,  5,-30 }},
    {{ -40,-20,  0,  5,  5,  0,-20,-40 }},
    {{ -50,-40,-30,-30,-30,-30,-40,-50 }}
}};

const std::array<std::array<int, 8>, 8> Evaluation::BISHOP_TABLE = {{
    {{ -20,-10,-10,-10,-10,-10,-10,-20 }},
    {{ -10,  0,  0,  0,  0,  0,  0,-10 }},
    {{ -10,  0,  5, 10, 10,  5,  0,-10 }},
    {{ -10,  5,  5, 10, 10,  5,  5,-10 }},
    {{ -10,  0, 10, 10, 10, 10,  0,-10 }},
    {{ -10, 10, 10, 10, 10, 10, 10,-10 }},
    {{ -10,  5,  0,  0,  0,  0,  5,-10 }},
    {{ -20,-10,-10,-10,-10,-10,-10,-20 }}
}};

const std::array<std::array<int, 8>, 8> Evaluation::ROOK_TABLE = {{
    {{  0,  0,  0,  0,  0,  0,  0,  0 }},
    {{  5, 10, 10, 10, 10, 10, 10,  5 }},
    {{ -5,  0,  0,  0,  0,  0,  0, -5 }},
    {{ -5,  0,  0,  0,  0,  0,  0, -5 }},
    {{ -5,  0,  0,  0,  0,  0,  0, -5 }},
    {{ -5,  0,  0,  0,  0,  0,  0, -5 }},
    {{ -5,  0,  0,  0,  0,  0,  0, -5 }},
    {{  0,  0,  0,  5,  5,  0,  0,  0 }}
}};

const std::array<std::array<int, 8>, 8> Evaluation::QUEEN_TABLE = {{
    {{ -20,-10,-10, -5, -5,-10,-10,-20 }},
    {{ -10,  0,  0,  0,  0,  0,  0,-10 }},
    {{ -10,  0,  5,  5,  5,  5,  0,-10 }},
    {{  -5,  0,  5,  5,  5,  5,  0, -5 }},
    {{   0,  0,  5,  5,  5,  5,  0, -5 }},
    {{ -10,  5,  5,  5,  5,  5,  0,-10 }},
    {{ -10,  0,  5,  0,  0,  0,  0,-10 }},
    {{ -20,-10,-10, -5, -5,-10,-10,-20 }}
}};

const std::array<std::array<int, 8>, 8> Evaluation::KING_MG_TABLE = {{
    {{ -30,-40,-40,-50,-50,-40,-40,-30 }},
    {{ -30,-40,-40,-50,-50,-40,-40,-30 }},
    {{ -30,-40,-40,-50,-50,-40,-40,-30 }},
    {{ -30,-40,-40,-50,-50,-40,-40,-30 }},
    {{ -20,-30,-30,-40,-40,-30,-30,-20 }},
    {{ -10,-20,-20,-20,-20,-20,-20,-10 }},
    {{  20, 20,  0,  0,  0,  0, 20, 20 }},
    {{  20, 30, 10,  0,  0, 10, 30, 20 }}
}};

const std::array<std::array<int, 8>, 8> Evaluation::KING_EG_TABLE = {{
    {{ -50,-40,-30,-20,-20,-30,-40,-50 }},
    {{ -30,-20,-10,  0,  0,-10,-20,-30 }},
    {{ -30,-10, 20, 30, 30, 20,-10,-30 }},
    {{ -30,-10, 30, 40, 40, 30,-10,-30 }},
    {{ -30,-10, 30, 40, 40, 30,-10,-30 }},
    {{ -30,-10, 20, 30, 30, 20,-10,-30 }},
    {{ -30,-30,  0,  0,  0,  0,-30,-30 }},
    {{ -50,-30,-30,-30,-30,-30,-30,-50 }}
}};

int Evaluation::evaluate_position(const Board& board) {
    int score = 0;
    
    // Material evaluation
    score += evaluate_material(board);
    
    // Positional evaluation
    score += evaluate_position_tables(board);
    
    // Mobility evaluation
    score += evaluate_mobility(board);
    
    // King safety evaluation
    score += evaluate_king_safety(board);
    
    // Pawn structure evaluation
    score += evaluate_pawn_structure(board);
    
    // Return score from the perspective of the side to move
    return board.get_active_color() == 'w' ? score : -score;
}

int Evaluation::evaluate_material(const Board& board) {
    int white_material = 0;
    int black_material = 0;
    
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            char piece = board.get_piece(rank, file);
            if (piece != '.') {
                int value = get_piece_value(piece);
                if (is_white_piece(piece)) {
                    white_material += value;
                } else {
                    black_material += value;
                }
            }
        }
    }
    
    return white_material - black_material;
}

int Evaluation::evaluate_position_tables(const Board& board) {
    int white_positional = 0;
    int black_positional = 0;
    bool endgame = is_endgame(board);
    
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            char piece = board.get_piece(rank, file);
            if (piece != '.') {
                int value = get_piece_square_value(piece, rank, file, endgame);
                if (is_white_piece(piece)) {
                    white_positional += value;
                } else {
                    black_positional += value;
                }
            }
        }
    }
    
    return white_positional - black_positional;
}

int Evaluation::evaluate_mobility(const Board& board) {
    // Simple mobility evaluation - count legal moves
    Board temp_board = board;
    
    // Count white mobility
    temp_board.set_active_color('w');
    MoveList white_moves = MoveGenerator::generate_legal_moves(temp_board);
    int white_mobility = white_moves.size();
    
    // Count black mobility
    temp_board.set_active_color('b');
    MoveList black_moves = MoveGenerator::generate_legal_moves(temp_board);
    int black_mobility = black_moves.size();
    
    return (white_mobility - black_mobility) * 2; // 2 centipawns per move
}

int Evaluation::evaluate_king_safety(const Board& board) {
    int safety_score = 0;
    
    // Find kings
    int white_king_rank = -1, white_king_file = -1;
    int black_king_rank = -1, black_king_file = -1;
    
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            char piece = board.get_piece(rank, file);
            if (piece == 'K') {
                white_king_rank = rank;
                white_king_file = file;
            } else if (piece == 'k') {
                black_king_rank = rank;
                black_king_file = file;
            }
        }
    }
    
    // Evaluate pawn shield for white king
    if (white_king_rank != -1 && white_king_file != -1) {
        int shield_bonus = 0;
        for (int file_offset = -1; file_offset <= 1; file_offset++) {
            int shield_file = white_king_file + file_offset;
            if (shield_file >= 0 && shield_file < 8) {
                // Check for pawn in front of king
                if (white_king_rank > 0 && board.get_piece(white_king_rank - 1, shield_file) == 'P') {
                    shield_bonus += 10;
                }
                // Check for pawn two squares in front
                if (white_king_rank > 1 && board.get_piece(white_king_rank - 2, shield_file) == 'P') {
                    shield_bonus += 5;
                }
            }
        }
        safety_score += shield_bonus;
    }
    
    // Evaluate pawn shield for black king
    if (black_king_rank != -1 && black_king_file != -1) {
        int shield_bonus = 0;
        for (int file_offset = -1; file_offset <= 1; file_offset++) {
            int shield_file = black_king_file + file_offset;
            if (shield_file >= 0 && shield_file < 8) {
                // Check for pawn in front of king
                if (black_king_rank < 7 && board.get_piece(black_king_rank + 1, shield_file) == 'p') {
                    shield_bonus += 10;
                }
                // Check for pawn two squares in front
                if (black_king_rank < 6 && board.get_piece(black_king_rank + 2, shield_file) == 'p') {
                    shield_bonus += 5;
                }
            }
        }
        safety_score -= shield_bonus;
    }
    
    return safety_score;
}

int Evaluation::evaluate_pawn_structure(const Board& board) {
    int structure_score = 0;
    
    // Count doubled pawns, isolated pawns, etc.
    for (int file = 0; file < 8; file++) {
        int white_pawns = 0;
        int black_pawns = 0;
        
        for (int rank = 0; rank < 8; rank++) {
            char piece = board.get_piece(rank, file);
            if (piece == 'P') white_pawns++;
            if (piece == 'p') black_pawns++;
        }
        
        // Penalty for doubled pawns
        if (white_pawns > 1) {
            structure_score -= (white_pawns - 1) * 10;
        }
        if (black_pawns > 1) {
            structure_score += (black_pawns - 1) * 10;
        }
        
        // Check for isolated pawns
        bool white_has_neighbor = false;
        bool black_has_neighbor = false;
        
        for (int neighbor_file = std::max(0, file - 1); neighbor_file <= std::min(7, file + 1); neighbor_file++) {
            if (neighbor_file == file) continue;
            
            for (int rank = 0; rank < 8; rank++) {
                char piece = board.get_piece(rank, neighbor_file);
                if (piece == 'P') white_has_neighbor = true;
                if (piece == 'p') black_has_neighbor = true;
            }
        }
        
        if (white_pawns > 0 && !white_has_neighbor) {
            structure_score -= 15; // Isolated pawn penalty
        }
        if (black_pawns > 0 && !black_has_neighbor) {
            structure_score += 15; // Isolated pawn penalty
        }
    }
    
    return structure_score;
}

int Evaluation::get_piece_value(char piece) {
    switch (to_lower(piece)) {
        case 'p': return PAWN_VALUE;
        case 'n': return KNIGHT_VALUE;
        case 'b': return BISHOP_VALUE;
        case 'r': return ROOK_VALUE;
        case 'q': return QUEEN_VALUE;
        case 'k': return KING_VALUE;
        default: return 0;
    }
}

bool Evaluation::is_endgame(const Board& board) {
    int total_material = 0;
    
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            char piece = board.get_piece(rank, file);
            if (piece != '.' && to_lower(piece) != 'k') {
                total_material += get_piece_value(piece);
            }
        }
    }
    
    // Consider it endgame if total material is less than 2 rooks + 2 knights
    return total_material < (2 * ROOK_VALUE + 2 * KNIGHT_VALUE);
}

int Evaluation::get_piece_square_value(char piece, int rank, int file, bool is_endgame) {
    char piece_type = to_lower(piece);
    bool is_white = is_white_piece(piece);
    
    // Flip rank for black pieces
    int table_rank = is_white ? rank : flip_rank(rank);
    
    int value = 0;
    
    switch (piece_type) {
        case 'p':
            value = PAWN_TABLE[table_rank][file];
            break;
        case 'n':
            value = KNIGHT_TABLE[table_rank][file];
            break;
        case 'b':
            value = BISHOP_TABLE[table_rank][file];
            break;
        case 'r':
            value = ROOK_TABLE[table_rank][file];
            break;
        case 'q':
            value = QUEEN_TABLE[table_rank][file];
            break;
        case 'k':
            if (is_endgame) {
                value = KING_EG_TABLE[table_rank][file];
            } else {
                value = KING_MG_TABLE[table_rank][file];
            }
            break;
        default:
            value = 0;
            break;
    }
    
    return is_white ? value : -value;
}

int Evaluation::flip_rank(int rank) {
    return 7 - rank;
}

bool Evaluation::is_white_piece(char piece) {
    return piece >= 'A' && piece <= 'Z';
}

bool Evaluation::is_black_piece(char piece) {
    return piece >= 'a' && piece <= 'z';
}

char Evaluation::to_lower(char piece) {
    return std::tolower(piece);
}