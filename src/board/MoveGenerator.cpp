#include "MoveGenerator.h"
#include <algorithm>

MoveGenerator::MoveGenerator() : nodes_searched(0), moves_generated(0) {
    // Initialize bitboard utilities if not already done
    BitboardUtils::init();
}

std::vector<Move> MoveGenerator::generate_all_moves(const Board& board) {
    std::vector<Move> moves;
    moves.reserve(256); // Reserve space for typical move count

    generate_pawn_moves(board, moves);
    generate_knight_moves(board, moves);
    generate_bishop_moves(board, moves);
    generate_rook_moves(board, moves);
    generate_queen_moves(board, moves);
    generate_king_moves(board, moves);
    generate_castling_moves(board, moves);
    generate_en_passant_moves(board, moves);

    moves_generated += moves.size();
    return moves;
}

std::vector<Move> MoveGenerator::generate_legal_moves(Board& board) {
    std::vector<Move> pseudo_legal = generate_all_moves(board);
    std::vector<Move> legal_moves;
    legal_moves.reserve(pseudo_legal.size());

    for (const Move& move : pseudo_legal) {
        if (is_legal_move(board, move)) {
            legal_moves.push_back(move);
        }
    }

    return legal_moves;
}

std::vector<Move> MoveGenerator::generate_captures(const Board& board) {
    std::vector<Move> moves;
    moves.reserve(64);

    generate_pawn_moves(board, moves, true);
    generate_knight_moves(board, moves, true);
    generate_bishop_moves(board, moves, true);
    generate_rook_moves(board, moves, true);
    generate_queen_moves(board, moves, true);
    generate_king_moves(board, moves, true);
    generate_en_passant_moves(board, moves);

    return moves;
}

std::vector<Move> MoveGenerator::generate_quiet_moves(const Board& board) {
    std::vector<Move> moves;
    moves.reserve(192);

    generate_pawn_moves(board, moves, false);
    generate_knight_moves(board, moves, false);
    generate_bishop_moves(board, moves, false);
    generate_rook_moves(board, moves, false);
    generate_queen_moves(board, moves, false);
    generate_king_moves(board, moves, false);
    generate_castling_moves(board, moves);

    // Filter out captures
    Board::Color opponent = (board.get_active_color() == Board::WHITE) ?
                                   Board::BLACK : Board::WHITE;
    Bitboard opponent_pieces = board.get_color_bitboard(opponent);

    moves.erase(std::remove_if(moves.begin(), moves.end(), [&](const Move& move) {
        int to_square = BitboardUtils::square_index(move.to_rank, move.to_file);
        return BitboardUtils::get_bit(opponent_pieces, to_square);
    }), moves.end());

    return moves;
}

void MoveGenerator::generate_pawn_moves(const Board& board, std::vector<Move>& moves, bool captures_only) {
    Board::Color color = board.get_active_color();
    Board::Color opponent = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;

    Bitboard pawns = board.get_piece_bitboard(Board::PAWN, color);
    Bitboard opponent_pieces = board.get_color_bitboard(opponent);
    Bitboard all_pieces = board.get_all_pieces();

    int direction = (color == Board::WHITE) ? 1 : -1;
    int start_rank = (color == Board::WHITE) ? 1 : 6;
    int promotion_rank = (color == Board::WHITE) ? 7 : 0;

    // Pawn captures
    Bitboard temp_pawns = pawns;
    while (temp_pawns) {
        int from_square = BitboardUtils::pop_lsb(temp_pawns);
        int from_rank = BitboardUtils::get_rank(from_square);
        int from_file = BitboardUtils::get_file(from_square);

        Bitboard pawn_attacks = BitboardUtils::pawn_attacks(from_square, color == Board::WHITE);
        Bitboard captures = pawn_attacks & opponent_pieces;

        add_pawn_moves(from_square, captures, color, board, moves, true);

        if (!captures_only) {
            // Pawn pushes
            int push_rank = from_rank + direction;
            if (push_rank >= 0 && push_rank < 8) {
                int push_square = BitboardUtils::square_index(push_rank, from_file);

                if (!BitboardUtils::get_bit(all_pieces, push_square)) {
                    Bitboard single_push = 0;
                    BitboardUtils::set_bit(single_push, push_square);
                    add_pawn_moves(from_square, single_push, color, board, moves, false);

                    // Double pawn push
                    if (from_rank == start_rank) {
                        int double_push_rank = push_rank + direction;
                        if (double_push_rank >= 0 && double_push_rank < 8) {
                            int double_push_square = BitboardUtils::square_index(double_push_rank, from_file);
                            if (!BitboardUtils::get_bit(all_pieces, double_push_square)) {
                                Bitboard double_push = 0;
                                BitboardUtils::set_bit(double_push, double_push_square);
                                add_pawn_moves(from_square, double_push, color, board, moves, false);
                            }
                        }
                    }
                }
            }
        }
    }
}

void MoveGenerator::generate_knight_moves(const Board& board, std::vector<Move>& moves, bool captures_only) {
    Board::Color color = board.get_active_color();
    Bitboard knights = board.get_piece_bitboard(Board::KNIGHT, color);
    Bitboard own_pieces = board.get_color_bitboard(color);
    Bitboard opponent_pieces = board.get_color_bitboard(
        (color == Board::WHITE) ? Board::BLACK : Board::WHITE);

    while (knights) {
        int from_square = BitboardUtils::pop_lsb(knights);
        Bitboard knight_attacks = BitboardUtils::knight_attacks(from_square);

        // Remove own pieces
        knight_attacks &= ~own_pieces;

        if (captures_only) {
            knight_attacks &= opponent_pieces;
        }

        add_moves_from_bitboard(1ULL << from_square, knight_attacks,
                               Board::KNIGHT, color, board, moves, captures_only);
    }
}

void MoveGenerator::generate_bishop_moves(const Board& board, std::vector<Move>& moves, bool captures_only) {
    Board::Color color = board.get_active_color();
    Bitboard bishops = board.get_piece_bitboard(Board::BISHOP, color);
    Bitboard own_pieces = board.get_color_bitboard(color);
    Bitboard opponent_pieces = board.get_color_bitboard(
        (color == Board::WHITE) ? Board::BLACK : Board::WHITE);
    Bitboard all_pieces = board.get_all_pieces();

    while (bishops) {
        int from_square = BitboardUtils::pop_lsb(bishops);
        Bitboard bishop_attacks = BitboardUtils::bishop_attacks(from_square, all_pieces);

        // Remove own pieces
        bishop_attacks &= ~own_pieces;

        if (captures_only) {
            bishop_attacks &= opponent_pieces;
        }

        add_moves_from_bitboard(1ULL << from_square, bishop_attacks,
                               Board::BISHOP, color, board, moves, captures_only);
    }
}

void MoveGenerator::generate_rook_moves(const Board& board, std::vector<Move>& moves, bool captures_only) {
    Board::Color color = board.get_active_color();
    Bitboard rooks = board.get_piece_bitboard(Board::ROOK, color);
    Bitboard own_pieces = board.get_color_bitboard(color);
    Bitboard opponent_pieces = board.get_color_bitboard(
        (color == Board::WHITE) ? Board::BLACK : Board::WHITE);
    Bitboard all_pieces = board.get_all_pieces();

    while (rooks) {
        int from_square = BitboardUtils::pop_lsb(rooks);
        Bitboard rook_attacks = BitboardUtils::rook_attacks(from_square, all_pieces);

        // Remove own pieces
        rook_attacks &= ~own_pieces;

        if (captures_only) {
            rook_attacks &= opponent_pieces;
        }

        add_moves_from_bitboard(1ULL << from_square, rook_attacks,
                               Board::ROOK, color, board, moves, captures_only);
    }
}

void MoveGenerator::generate_queen_moves(const Board& board, std::vector<Move>& moves, bool captures_only) {
    Board::Color color = board.get_active_color();
    Bitboard queens = board.get_piece_bitboard(Board::QUEEN, color);
    Bitboard own_pieces = board.get_color_bitboard(color);
    Bitboard opponent_pieces = board.get_color_bitboard(
        (color == Board::WHITE) ? Board::BLACK : Board::WHITE);
    Bitboard all_pieces = board.get_all_pieces();

    while (queens) {
        int from_square = BitboardUtils::pop_lsb(queens);
        Bitboard queen_attacks = BitboardUtils::queen_attacks(from_square, all_pieces);

        // Remove own pieces
        queen_attacks &= ~own_pieces;

        if (captures_only) {
            queen_attacks &= opponent_pieces;
        }

        add_moves_from_bitboard(1ULL << from_square, queen_attacks,
                               Board::QUEEN, color, board, moves, captures_only);
    }
}

void MoveGenerator::generate_king_moves(const Board& board, std::vector<Move>& moves, bool captures_only) {
    Board::Color color = board.get_active_color();
    int king_square = board.get_king_position(color);

    if (king_square == -1) return;

    Bitboard own_pieces = board.get_color_bitboard(color);
    Bitboard opponent_pieces = board.get_color_bitboard(
        (color == Board::WHITE) ? Board::BLACK : Board::WHITE);

    Bitboard king_attacks = BitboardUtils::king_attacks(king_square);

    // Remove own pieces
    king_attacks &= ~own_pieces;

    if (captures_only) {
        king_attacks &= opponent_pieces;
    }

    add_moves_from_bitboard(1ULL << king_square, king_attacks,
                           Board::KING, color, board, moves, captures_only);
}

void MoveGenerator::generate_castling_moves(const Board& board, std::vector<Move>& moves) {
    Board::Color color = board.get_active_color();

    if (is_in_check(board, color)) return;

    if (can_castle_kingside(board, color)) {
        int king_rank = (color == Board::WHITE) ? 0 : 7;
        Move castle_move;
        castle_move.from_rank = king_rank;
        castle_move.from_file = 4;
        castle_move.to_rank = king_rank;
        castle_move.to_file = 6;
        castle_move.piece = piece_type_to_char(Board::KING, color);
        castle_move.is_castling = true;
        castle_move.promotion_piece = '.';
        castle_move.is_en_passant = false;
        moves.push_back(castle_move);
    }

    if (can_castle_queenside(board, color)) {
        int king_rank = (color == Board::WHITE) ? 0 : 7;
        Move castle_move;
        castle_move.from_rank = king_rank;
        castle_move.from_file = 4;
        castle_move.to_rank = king_rank;
        castle_move.to_file = 2;
        castle_move.piece = piece_type_to_char(Board::KING, color);
        castle_move.is_castling = true;
        castle_move.promotion_piece = '.';
        castle_move.is_en_passant = false;
        moves.push_back(castle_move);
    }
}

void MoveGenerator::generate_en_passant_moves(const Board& board, std::vector<Move>& moves) {
    if (board.get_en_passant_file() == -1) return;

    Board::Color color = board.get_active_color();
    Bitboard pawns = board.get_piece_bitboard(Board::PAWN, color);

    int en_passant_file = board.get_en_passant_file();
    int en_passant_rank = (color == Board::WHITE) ? 5 : 2;
    int capture_rank = (color == Board::WHITE) ? 4 : 3;

    // Check for pawns that can capture en passant
    if (en_passant_file > 0) {
        int left_pawn_square = BitboardUtils::square_index(capture_rank, en_passant_file - 1);
        if (BitboardUtils::get_bit(pawns, left_pawn_square)) {
            Move en_passant_move;
            en_passant_move.from_rank = capture_rank;
            en_passant_move.from_file = en_passant_file - 1;
            en_passant_move.to_rank = en_passant_rank;
            en_passant_move.to_file = en_passant_file;
            en_passant_move.piece = piece_type_to_char(Board::PAWN, color);
            en_passant_move.is_en_passant = true;
            en_passant_move.is_castling = false;
            en_passant_move.promotion_piece = '.';
            moves.push_back(en_passant_move);
        }
    }

    if (en_passant_file < 7) {
        int right_pawn_square = BitboardUtils::square_index(capture_rank, en_passant_file + 1);
        if (BitboardUtils::get_bit(pawns, right_pawn_square)) {
            Move en_passant_move;
            en_passant_move.from_rank = capture_rank;
            en_passant_move.from_file = en_passant_file + 1;
            en_passant_move.to_rank = en_passant_rank;
            en_passant_move.to_file = en_passant_file;
            en_passant_move.piece = piece_type_to_char(Board::PAWN, color);
            en_passant_move.is_en_passant = true;
            en_passant_move.is_castling = false;
            en_passant_move.promotion_piece = '.';
            moves.push_back(en_passant_move);
        }
    }
}

bool MoveGenerator::is_in_check(const Board& board, Board::Color color) {
    return board.is_in_check(color);
}

bool MoveGenerator::is_legal_move(Board& board, const Move& move) {
    // Make a copy of the board state
    BitboardMoveUndoData undo_data;
    undo_data.castling_rights = board.get_castling_rights();
    undo_data.en_passant_file = board.get_en_passant_file();
    undo_data.halfmove_clock = board.get_halfmove_clock();

    Board::Color moving_color = board.get_active_color();

    // Make the move
    board.make_move(move);

    // Check if the king is in check after the move
    bool is_legal = !is_in_check(board, moving_color);

    // Undo the move
    board.undo_move(move, undo_data);

    return is_legal;
}

bool MoveGenerator::is_square_attacked(const Board& board, int square, Board::Color attacking_color) {
    return board.is_square_attacked(square, attacking_color);
}

Bitboard MoveGenerator::get_attacked_squares(const Board& board, Board::Color color) {
    Bitboard attacked = 0;
    Bitboard all_pieces = board.get_all_pieces();

    // Pawn attacks
    Bitboard pawns = board.get_piece_bitboard(Board::PAWN, color);
    while (pawns) {
        int square = BitboardUtils::pop_lsb(pawns);
        attacked |= BitboardUtils::pawn_attacks(square, color == Board::WHITE);
    }

    // Knight attacks
    Bitboard knights = board.get_piece_bitboard(Board::KNIGHT, color);
    while (knights) {
        int square = BitboardUtils::pop_lsb(knights);
        attacked |= BitboardUtils::knight_attacks(square);
    }

    // Bishop attacks
    Bitboard bishops = board.get_piece_bitboard(Board::BISHOP, color);
    while (bishops) {
        int square = BitboardUtils::pop_lsb(bishops);
        attacked |= BitboardUtils::bishop_attacks(square, all_pieces);
    }

    // Rook attacks
    Bitboard rooks = board.get_piece_bitboard(Board::ROOK, color);
    while (rooks) {
        int square = BitboardUtils::pop_lsb(rooks);
        attacked |= BitboardUtils::rook_attacks(square, all_pieces);
    }

    // Queen attacks
    Bitboard queens = board.get_piece_bitboard(Board::QUEEN, color);
    while (queens) {
        int square = BitboardUtils::pop_lsb(queens);
        attacked |= BitboardUtils::queen_attacks(square, all_pieces);
    }

    // King attacks
    int king_square = board.get_king_position(color);
    if (king_square != -1) {
        attacked |= BitboardUtils::king_attacks(king_square);
    }

    return attacked;
}

int MoveGenerator::count_moves(const Board& board) {
    return generate_all_moves(board).size();
}

bool MoveGenerator::has_legal_moves(Board& board) {
    std::vector<Move> moves = generate_all_moves(board);
    for (const Move& move : moves) {
        if (is_legal_move(board, move)) {
            return true;
        }
    }
    return false;
}

// Helper function implementations
void MoveGenerator::add_moves_from_bitboard(Bitboard from_square, Bitboard to_squares,
                                                   Board::PieceType piece_type, Board::Color color,
                                                   const Board& board, std::vector<Move>& moves, bool captures_only) {
    int from_sq = BitboardUtils::get_lsb_index(from_square);
    int from_rank = BitboardUtils::get_rank(from_sq);
    int from_file = BitboardUtils::get_file(from_sq);

    while (to_squares) {
        int to_sq = BitboardUtils::pop_lsb(to_squares);
        int to_rank = BitboardUtils::get_rank(to_sq);
        int to_file = BitboardUtils::get_file(to_sq);

        Move move;
        move.from_rank = from_rank;
        move.from_file = from_file;
        move.to_rank = to_rank;
        move.to_file = to_file;
        move.piece = piece_type_to_char(piece_type, color);
        move.is_castling = false;
        move.is_en_passant = false;
        move.promotion_piece = '.';

        moves.push_back(move);
    }
}

void MoveGenerator::add_pawn_moves(int from_square, Bitboard to_squares, Board::Color color,
                                          const Board& board, std::vector<Move>& moves, bool is_capture) {
    int from_rank = BitboardUtils::get_rank(from_square);
    int from_file = BitboardUtils::get_file(from_square);

    while (to_squares) {
        int to_square = BitboardUtils::pop_lsb(to_squares);
        int to_rank = BitboardUtils::get_rank(to_square);
        int to_file = BitboardUtils::get_file(to_square);

        if (is_promotion_rank(to_rank, color)) {
            add_promotion_moves(from_square, to_square, color, board, moves, is_capture);
        } else {
            Move move;
            move.from_rank = from_rank;
            move.from_file = from_file;
            move.to_rank = to_rank;
            move.to_file = to_file;
            move.piece = piece_type_to_char(Board::PAWN, color);
            move.is_castling = false;
            move.is_en_passant = false;
            move.promotion_piece = '.';

            moves.push_back(move);
        }
    }
}

void MoveGenerator::add_promotion_moves(int from_square, int to_square, Board::Color color,
                                               const Board& board, std::vector<Move>& moves, bool is_capture) {
    int from_rank = BitboardUtils::get_rank(from_square);
    int from_file = BitboardUtils::get_file(from_square);
    int to_rank = BitboardUtils::get_rank(to_square);
    int to_file = BitboardUtils::get_file(to_square);

    char promotion_pieces[] = {'q', 'r', 'b', 'n'};

    for (char promo_piece : promotion_pieces) {
        Move move;
        move.from_rank = from_rank;
        move.from_file = from_file;
        move.to_rank = to_rank;
        move.to_file = to_file;
        move.piece = piece_type_to_char(Board::PAWN, color);
        move.promotion_piece = (color == Board::WHITE) ? std::toupper(promo_piece) : promo_piece;
        move.is_castling = false;
        move.is_en_passant = false;

        moves.push_back(move);
    }
}

bool MoveGenerator::can_castle_kingside(const Board& board, Board::Color color) {
    uint8_t castling_rights = board.get_castling_rights();
    uint8_t required_right = (color == Board::WHITE) ? 0x01 : 0x04;

    if (!(castling_rights & required_right)) return false;

    int king_rank = (color == Board::WHITE) ? 0 : 7;
    Bitboard all_pieces = board.get_all_pieces();

    // Check if squares between king and rook are empty
    int f_square = BitboardUtils::square_index(king_rank, 5);
    int g_square = BitboardUtils::square_index(king_rank, 6);

    if (BitboardUtils::get_bit(all_pieces, f_square) || BitboardUtils::get_bit(all_pieces, g_square)) {
        return false;
    }

    // Check if king passes through attacked squares
    Board::Color opponent = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;

    if (is_square_attacked(board, BitboardUtils::square_index(king_rank, 4), opponent) ||
        is_square_attacked(board, f_square, opponent) ||
        is_square_attacked(board, g_square, opponent)) {
        return false;
    }

    return true;
}

bool MoveGenerator::can_castle_queenside(const Board& board, Board::Color color) {
    uint8_t castling_rights = board.get_castling_rights();
    uint8_t required_right = (color == Board::WHITE) ? 0x02 : 0x08;

    if (!(castling_rights & required_right)) return false;

    int king_rank = (color == Board::WHITE) ? 0 : 7;
    Bitboard all_pieces = board.get_all_pieces();

    // Check if squares between king and rook are empty
    int b_square = BitboardUtils::square_index(king_rank, 1);
    int c_square = BitboardUtils::square_index(king_rank, 2);
    int d_square = BitboardUtils::square_index(king_rank, 3);

    if (BitboardUtils::get_bit(all_pieces, b_square) ||
        BitboardUtils::get_bit(all_pieces, c_square) ||
        BitboardUtils::get_bit(all_pieces, d_square)) {
        return false;
    }

    // Check if king passes through attacked squares
    Board::Color opponent = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;

    if (is_square_attacked(board, BitboardUtils::square_index(king_rank, 4), opponent) ||
        is_square_attacked(board, c_square, opponent) ||
        is_square_attacked(board, d_square, opponent)) {
        return false;
    }

    return true;
}

char MoveGenerator::piece_type_to_char(Board::PieceType piece_type, Board::Color color) {
    char pieces[] = {'p', 'n', 'b', 'r', 'q', 'k'};
    char piece_char = pieces[piece_type];
    return (color == Board::WHITE) ? std::toupper(piece_char) : piece_char;
}

bool MoveGenerator::is_promotion_rank(int rank, Board::Color color) {
    return (color == Board::WHITE && rank == 7) || (color == Board::BLACK && rank == 0);
}