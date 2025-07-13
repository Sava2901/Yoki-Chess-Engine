#include "MoveGenerator.h"
#include <algorithm>
#include <iostream>
#include <immintrin.h>  // For PEXT if available

// Cache optimization macros
#ifdef _MSC_VER
    #define PREFETCH_READ(addr) _mm_prefetch((const char*)(addr), _MM_HINT_T0)
    #define PREFETCH_WRITE(addr) _mm_prefetch((const char*)(addr), _MM_HINT_T0)
#elif defined(__GNUC__)
    #define PREFETCH_READ(addr) __builtin_prefetch((addr), 0, 3)
    #define PREFETCH_WRITE(addr) __builtin_prefetch((addr), 1, 3)
#else
    #define PREFETCH_READ(addr) ((void)0)
    #define PREFETCH_WRITE(addr) ((void)0)
#endif

// Prefetch multiple cache lines for large data structures
#define PREFETCH_RANGE(addr, size) do { \
    const char* ptr = (const char*)(addr); \
    for (size_t i = 0; i < (size); i += 64) { \
        PREFETCH_READ(ptr + i); \
    } \
} while(0)

// Move ordering scores
static constexpr int MVV_LVA[7][7] = {
    {0, 0, 0, 0, 0, 0, 0},       // Empty
    {15, 14, 13, 12, 11, 10, 0}, // Pawn captures
    {25, 24, 23, 22, 21, 20, 0}, // Knight captures
    {35, 34, 33, 32, 31, 30, 0}, // Bishop captures
    {45, 44, 43, 42, 41, 40, 0}, // Rook captures
    {55, 54, 53, 52, 51, 50, 0}, // Queen captures
    {0, 0, 0, 0, 0, 0, 0}        // King captures (illegal)
};

static constexpr int PROMOTION_BONUS = 1000;
static constexpr int CASTLING_BONUS = 50;
static constexpr int EN_PASSANT_BONUS = 105;

// Check for BMI2 support at compile time
#ifdef __BMI2__
    #define USE_PEXT 1
#else
    #define USE_PEXT 0
#endif

MoveGenerator::MoveGenerator() : nodes_searched(0), moves_generated(0) {
    // Initialize bitboard utilities if not already done
    BitboardUtils::init();
}

std::vector<Move> MoveGenerator::generate_all_moves(const Board& board) {
    std::vector<Move> moves;
    moves.reserve(218); // More precise reservation based on average move count

    // Prefetch critical board data for better cache performance
    Board::Color color = board.get_active_color();
    Board::Color opponent = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;
    
    // Prefetch piece bitboards that will be accessed frequently
    // Note: These return by value, so we prefetch the board's internal data
    Bitboard queen_bb = board.get_piece_bitboard(Board::QUEEN, color);
    Bitboard rook_bb = board.get_piece_bitboard(Board::ROOK, color);
    Bitboard bishop_bb = board.get_piece_bitboard(Board::BISHOP, color);
    Bitboard knight_bb = board.get_piece_bitboard(Board::KNIGHT, color);
    Bitboard pawn_bb = board.get_piece_bitboard(Board::PAWN, color);
    Bitboard color_bb = board.get_color_bitboard(color);
    Bitboard opponent_bb = board.get_color_bitboard(opponent);
    Bitboard all_pieces_bb = board.get_all_pieces();
    
    // Prefetch the actual bitboard data
    PREFETCH_READ(&queen_bb);
    PREFETCH_READ(&rook_bb);
    PREFETCH_READ(&bishop_bb);
    PREFETCH_READ(&knight_bb);
    PREFETCH_READ(&pawn_bb);
    PREFETCH_READ(&color_bb);
    PREFETCH_READ(&opponent_bb);
    PREFETCH_READ(&all_pieces_bb);
    
    // Prefetch magic bitboard attack tables for sliding pieces
    PREFETCH_RANGE(BitboardUtils::get_rook_attacks_table(0), 64 * sizeof(Bitboard*));
    PREFETCH_RANGE(BitboardUtils::get_bishop_attacks_table(0), 64 * sizeof(Bitboard*));

    // Generate moves in order of likely importance for better cache locality
    generate_queen_moves(board, moves);   // Most powerful piece first
    generate_rook_moves(board, moves);
    generate_bishop_moves(board, moves);
    generate_knight_moves(board, moves);
    generate_pawn_moves(board, moves);
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

    // Pre-compute check and pin information for faster legality testing
    Board::Color color = board.get_active_color();
    
    // Prefetch king position and attack tables for legality checking
    int king_square = board.get_king_position(color);
    if (king_square != -1) {
        // Prefetch attack tables around king position for pin detection
        PREFETCH_READ(BitboardUtils::get_rook_attacks_table(king_square));
        PREFETCH_READ(BitboardUtils::get_bishop_attacks_table(king_square));
    }
    
    Bitboard pinned_pieces = get_pinned_pieces(board, color);
    Bitboard check_mask = get_check_mask(board, color);
    bool in_check = (check_mask != FULL_BOARD);
    
    // Prefetch move data for faster iteration
    if (!pseudo_legal.empty()) {
        PREFETCH_RANGE(pseudo_legal.data(), pseudo_legal.size() * sizeof(Move));
    }

    for (const Move& move : pseudo_legal) {
        // Fast legality check using precomputed masks
        if (in_check) {
            if (is_move_legal_in_check(board, move, check_mask, pinned_pieces)) {
                legal_moves.push_back(move);
            }
        } else {
            if (is_legal_move(board, move)) {
                legal_moves.push_back(move);
            }
        }
    }

    // Order moves for better search performance
    // TODO: (Refactor) Check the existing ordering algorithm.
    order_moves(legal_moves, board);
    return legal_moves;
}

std::vector<Move> MoveGenerator::generate_captures(const Board& board) {
    std::vector<Move> moves;
    moves.reserve(32); // More precise reservation for captures

    // Generate captures in MVV-LVA order for better performance
    generate_queen_moves(board, moves, true);   // Queen captures first
    generate_rook_moves(board, moves, true);
    generate_bishop_moves(board, moves, true);
    generate_knight_moves(board, moves, true);
    generate_pawn_moves(board, moves, true);    // Includes promotions
    generate_king_moves(board, moves, true);
    generate_en_passant_moves(board, moves);

    // Order captures by MVV-LVA
    order_captures(moves, board);
    return moves;
}

std::vector<Move> MoveGenerator::generate_quiet_moves(const Board& board) {
    std::vector<Move> moves;
    moves.reserve(186); // More precise reservation for quiet moves

    // Generate quiet moves (non-captures)
    generate_queen_moves(board, moves, false);
    generate_rook_moves(board, moves, false);
    generate_bishop_moves(board, moves, false);
    generate_knight_moves(board, moves, false);
    generate_pawn_moves(board, moves, false);
    generate_king_moves(board, moves, false);
    generate_castling_moves(board, moves);

    // Pre-compute opponent pieces for faster filtering
    Board::Color opponent = (board.get_active_color() == Board::WHITE) ?
                                   Board::BLACK : Board::WHITE;
    Bitboard opponent_pieces = board.get_color_bitboard(opponent);

    // Use more efficient filtering with iterator
    auto new_end = std::remove_if(moves.begin(), moves.end(), [&](const Move& move) {
        int to_square = BitboardUtils::square_index(move.to_rank, move.to_file);
        return BitboardUtils::get_bit(opponent_pieces, to_square);
    });
    moves.erase(new_end, moves.end());

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

    // Prefetch knight attack patterns for all knight squares
    // Note: Knight attacks are precomputed in static tables, prefetching happens during table access

    while (knights) {
        int from_square = BitboardUtils::pop_lsb(knights);
        
        // Knight attacks are precomputed, no additional prefetching needed
        
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

    // Prefetch bishop attack tables for all potential bishop squares
    Bitboard temp_bishops = bishops;
    while (temp_bishops) {
        int square = BitboardUtils::get_lsb_index(temp_bishops);
        PREFETCH_READ(BitboardUtils::get_bishop_attacks_table(square));
        temp_bishops &= temp_bishops - 1; // Clear LSB without modifying original
    }

    while (bishops) {
        int from_square = BitboardUtils::pop_lsb(bishops);
        
        // Prefetch next bishop's attack table if available
        if (bishops) {
            int next_square = BitboardUtils::get_lsb_index(bishops);
            PREFETCH_READ(BitboardUtils::get_bishop_attacks_table(next_square));
        }
        
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

    // Prefetch rook attack tables for all potential rook squares
    Bitboard temp_rooks = rooks;
    while (temp_rooks) {
        int square = BitboardUtils::get_lsb_index(temp_rooks);
        PREFETCH_READ(BitboardUtils::get_rook_attacks_table(square));
        temp_rooks &= temp_rooks - 1; // Clear LSB without modifying original
    }

    while (rooks) {
        int from_square = BitboardUtils::pop_lsb(rooks);
        
        // Prefetch next rook's attack table if available
        if (rooks) {
            int next_square = BitboardUtils::get_lsb_index(rooks);
            PREFETCH_READ(BitboardUtils::get_rook_attacks_table(next_square));
        }
        
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

    // Prefetch both rook and bishop attack tables for all potential queen squares
    Bitboard temp_queens = queens;
    while (temp_queens) {
        int square = BitboardUtils::get_lsb_index(temp_queens);
        PREFETCH_READ(BitboardUtils::get_rook_attacks_table(square));
        PREFETCH_READ(BitboardUtils::get_bishop_attacks_table(square));
        temp_queens &= temp_queens - 1; // Clear LSB without modifying original
    }

    while (queens) {
        int from_square = BitboardUtils::pop_lsb(queens);
        
        // Prefetch next queen's attack tables if available
        if (queens) {
            int next_square = BitboardUtils::get_lsb_index(queens);
            PREFETCH_READ(BitboardUtils::get_rook_attacks_table(next_square));
            PREFETCH_READ(BitboardUtils::get_bishop_attacks_table(next_square));
        }
        
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
        castle_move.captured_piece = '.';  // Castling doesn't capture
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
        castle_move.captured_piece = '.';  // Castling doesn't capture
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
            // Set captured piece for en passant (opponent's pawn)
            Board::Color opponent = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;
            en_passant_move.captured_piece = piece_type_to_char(Board::PAWN, opponent);
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
            // Set captured piece for en passant (opponent's pawn)
            Board::Color opponent = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;
            en_passant_move.captured_piece = piece_type_to_char(Board::PAWN, opponent);
            moves.push_back(en_passant_move);
        }
    }
}

bool MoveGenerator::is_in_check(const Board& board, Board::Color color) {
    return board.is_in_check(color);
}

bool MoveGenerator::is_legal_move(Board& board, const Move& move) {
    Board::Color moving_color = board.get_active_color();

    // Make the move
    BitboardMoveUndoData undo_data = board.apply_move(move);

    // Check if the king is in check after the move
    bool is_legal = !is_in_check(board, moving_color);

    // Undo the move
    board.undo_move(undo_data);

    return is_legal;
}

bool MoveGenerator::is_square_attacked(const Board& board, int square, Board::Color attacking_color) {
    return board.is_square_attacked(square, attacking_color);
}

Bitboard MoveGenerator::get_attacked_squares(const Board& board, Board::Color color) {
    Bitboard attacked = 0;
    Bitboard all_pieces = board.get_all_pieces();

    // Prefetch all piece bitboards for this color
    // Note: These return by value, so we get them once for cache efficiency
    Bitboard pawns_prefetch = board.get_piece_bitboard(Board::PAWN, color);
    Bitboard knights_prefetch = board.get_piece_bitboard(Board::KNIGHT, color);
    Bitboard bishops_prefetch = board.get_piece_bitboard(Board::BISHOP, color);
    Bitboard rooks_prefetch = board.get_piece_bitboard(Board::ROOK, color);
    Bitboard queens_prefetch = board.get_piece_bitboard(Board::QUEEN, color);
    
    PREFETCH_READ(&pawns_prefetch);
    PREFETCH_READ(&knights_prefetch);
    PREFETCH_READ(&bishops_prefetch);
    PREFETCH_READ(&rooks_prefetch);
    PREFETCH_READ(&queens_prefetch);

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
        // Prefetch bishop attack table for this square
        PREFETCH_READ(BitboardUtils::get_bishop_attacks_table(square));
        attacked |= BitboardUtils::bishop_attacks(square, all_pieces);
    }

    // Rook attacks
    Bitboard rooks = board.get_piece_bitboard(Board::ROOK, color);
    while (rooks) {
        int square = BitboardUtils::pop_lsb(rooks);
        // Prefetch rook attack table for this square
        PREFETCH_READ(BitboardUtils::get_rook_attacks_table(square));
        attacked |= BitboardUtils::rook_attacks(square, all_pieces);
    }

    // Queen attacks
    Bitboard queens = board.get_piece_bitboard(Board::QUEEN, color);
    while (queens) {
        int square = BitboardUtils::pop_lsb(queens);
        // Prefetch both rook and bishop attack tables for queen
        PREFETCH_READ(BitboardUtils::get_rook_attacks_table(square));
        PREFETCH_READ(BitboardUtils::get_bishop_attacks_table(square));
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
        
        // Set captured piece if there's a piece on the destination square
        char piece_at_destination = board.get_piece(to_rank, to_file);
        move.captured_piece = (piece_at_destination != '.') ? piece_at_destination : '.';

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
            
            // Set captured piece for captures
            if (is_capture) {
                move.captured_piece = board.get_piece(to_rank, to_file);
            } else {
                move.captured_piece = '.';
            }

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
        
        // Set captured piece for promotion captures
        if (is_capture) {
            move.captured_piece = board.get_piece(to_rank, to_file);
        } else {
            move.captured_piece = '.';
        }

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

// Move ordering implementation
void MoveGenerator::order_moves(std::vector<Move>& moves, const Board& board) {
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        return get_move_score(a, board) > get_move_score(b, board);
    });
}

void MoveGenerator::order_captures(std::vector<Move>& moves, const Board& board) {
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        return get_capture_score(a, board) > get_capture_score(b, board);
    });
}

int MoveGenerator::get_move_score(const Move& move, const Board& board) {
    int score = 0;
    
    // Promotion bonus
    if (move.promotion_piece != '.') {
        score += PROMOTION_BONUS;
        if (move.promotion_piece == 'Q' || move.promotion_piece == 'q') score += 400;
        else if (move.promotion_piece == 'R' || move.promotion_piece == 'r') score += 200;
        else if (move.promotion_piece == 'B' || move.promotion_piece == 'b') score += 100;
        else if (move.promotion_piece == 'N' || move.promotion_piece == 'n') score += 100;
    }
    
    // Capture bonus (MVV-LVA)
    if (move.captured_piece != '.') {
        int victim = char_to_piece_type(move.captured_piece);
        int attacker = char_to_piece_type(move.piece);
        score += MVV_LVA[attacker][victim];
    }
    
    // Special move bonuses
    if (move.is_castling) score += CASTLING_BONUS;
    if (move.is_en_passant) score += EN_PASSANT_BONUS;
    
    return score;
}

int MoveGenerator::get_capture_score(const Move& move, const Board& board) {
    if (move.captured_piece == '.') return 0;
    
    int victim = char_to_piece_type(move.captured_piece);
    int attacker = char_to_piece_type(move.piece);
    return MVV_LVA[attacker][victim];
}

int MoveGenerator::char_to_piece_type(char piece) {
    switch (std::tolower(piece)) {
        case 'p': return 1; // Pawn
        case 'n': return 2; // Knight
        case 'b': return 3; // Bishop
        case 'r': return 4; // Rook
        case 'q': return 5; // Queen
        case 'k': return 6; // King
        default: return 0;  // Empty
    }
}

// Optimized pin and check detection
Bitboard MoveGenerator::get_pinned_pieces(const Board& board, Board::Color color) {
    Bitboard pinned = 0;
    int king_square = board.get_king_position(color);
    if (king_square == -1) return pinned;
    
    Board::Color opponent = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;
    Bitboard own_pieces = board.get_color_bitboard(color);
    Bitboard all_pieces = board.get_all_pieces();
    
    // Check for pins from sliding pieces
    Bitboard enemy_rooks_queens = board.get_piece_bitboard(Board::ROOK, opponent) |
                                  board.get_piece_bitboard(Board::QUEEN, opponent);
    Bitboard enemy_bishops_queens = board.get_piece_bitboard(Board::BISHOP, opponent) |
                                   board.get_piece_bitboard(Board::QUEEN, opponent);
    
    // Rook/Queen pins (horizontal and vertical)
    Bitboard rook_attacks = BitboardUtils::rook_attacks(king_square, all_pieces);
    Bitboard potential_pinners = rook_attacks & enemy_rooks_queens;
    
    while (potential_pinners) {
        int pinner_square = BitboardUtils::pop_lsb(potential_pinners);
        Bitboard between = get_between_squares(king_square, pinner_square) & all_pieces;
        if (BitboardUtils::popcount(between) == 1 && (between & own_pieces)) {
            pinned |= between;
        }
    }
    
    // Bishop/Queen pins (diagonal)
    Bitboard bishop_attacks = BitboardUtils::bishop_attacks(king_square, all_pieces);
    potential_pinners = bishop_attacks & enemy_bishops_queens;
    
    while (potential_pinners) {
        int pinner_square = BitboardUtils::pop_lsb(potential_pinners);
        Bitboard between = get_between_squares(king_square, pinner_square) & all_pieces;
        if (BitboardUtils::popcount(between) == 1 && (between & own_pieces)) {
            pinned |= between;
        }
    }
    
    return pinned;
}

Bitboard MoveGenerator::get_check_mask(const Board& board, Board::Color color) {
    int king_square = board.get_king_position(color);
    if (king_square == -1) return FULL_BOARD;
    
    if (!board.is_in_check(color)) {
        return FULL_BOARD; // No check, all squares are valid
    }
    
    // Find checking pieces and create mask
    Board::Color opponent = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;
    Bitboard checkers = get_attackers_to_square(board, king_square, opponent);
    
    if (BitboardUtils::popcount(checkers) > 1) {
        return 0; // Double check, only king moves are legal
    }
    
    int checker_square = BitboardUtils::get_lsb_index(checkers);
    
    // Single check: can block or capture
    Bitboard mask = checkers; // Can capture the checker
    mask |= get_between_squares(king_square, checker_square); // Can block
    
    return mask;
}

Bitboard MoveGenerator::get_between_squares(int sq1, int sq2) {
    Bitboard between = 0;
    
    int rank1 = BitboardUtils::get_rank(sq1);
    int file1 = BitboardUtils::get_file(sq1);
    int rank2 = BitboardUtils::get_rank(sq2);
    int file2 = BitboardUtils::get_file(sq2);
    
    int rank_dir = (rank2 > rank1) ? 1 : (rank2 < rank1) ? -1 : 0;
    int file_dir = (file2 > file1) ? 1 : (file2 < file1) ? -1 : 0;
    
    int rank = rank1 + rank_dir;
    int file = file1 + file_dir;
    
    while (rank != rank2 || file != file2) {
        BitboardUtils::set_bit(between, BitboardUtils::square_index(rank, file));
        rank += rank_dir;
        file += file_dir;
    }
    
    return between;
}

Bitboard MoveGenerator::get_attackers_to_square(const Board& board, int square, Board::Color attacking_color) {
    Bitboard attackers = 0;
    Bitboard all_pieces = board.get_all_pieces();
    
    // Pawn attacks
    Bitboard pawn_attacks = BitboardUtils::pawn_attacks(square, attacking_color != Board::WHITE);
    attackers |= pawn_attacks & board.get_piece_bitboard(Board::PAWN, attacking_color);
    
    // Knight attacks
    Bitboard knight_attacks = BitboardUtils::knight_attacks(square);
    attackers |= knight_attacks & board.get_piece_bitboard(Board::KNIGHT, attacking_color);
    
    // Bishop/Queen diagonal attacks
    Bitboard bishop_attacks = BitboardUtils::bishop_attacks(square, all_pieces);
    attackers |= bishop_attacks & (board.get_piece_bitboard(Board::BISHOP, attacking_color) |
                                   board.get_piece_bitboard(Board::QUEEN, attacking_color));
    
    // Rook/Queen straight attacks
    Bitboard rook_attacks = BitboardUtils::rook_attacks(square, all_pieces);
    attackers |= rook_attacks & (board.get_piece_bitboard(Board::ROOK, attacking_color) |
                                 board.get_piece_bitboard(Board::QUEEN, attacking_color));
    
    // King attacks
    Bitboard king_attacks = BitboardUtils::king_attacks(square);
    attackers |= king_attacks & board.get_piece_bitboard(Board::KING, attacking_color);
    
    return attackers;
}

bool MoveGenerator::is_move_legal_in_check(const Board& board, const Move& move, Bitboard check_mask, Bitboard pinned_pieces) {
    int from_square = BitboardUtils::square_index(move.from_rank, move.from_file);
    int to_square = BitboardUtils::square_index(move.to_rank, move.to_file);
    
    // King moves need special handling
    if (move.piece == 'K' || move.piece == 'k') {
        // King must move to a safe square
        Board::Color color = (move.piece == 'K') ? Board::WHITE : Board::BLACK;
        Board::Color opponent = (color == Board::WHITE) ? Board::BLACK : Board::WHITE;
        return !is_square_attacked(board, to_square, opponent);
    }
    
    // Non-king moves must either capture the checker or block the check
    if (!(BitboardUtils::get_bit(check_mask, to_square))) {
        return false;
    }
    
    // Check if the moving piece is pinned
    if (BitboardUtils::get_bit(pinned_pieces, from_square)) {
        // Pinned pieces can only move along the pin ray
        int king_square = board.get_king_position(board.get_active_color());
        return are_squares_aligned(king_square, from_square, to_square);
    }
    
    return true;
}

bool MoveGenerator::are_squares_aligned(int sq1, int sq2, int sq3) {
    int rank1 = BitboardUtils::get_rank(sq1);
    int file1 = BitboardUtils::get_file(sq1);
    int rank2 = BitboardUtils::get_rank(sq2);
    int file2 = BitboardUtils::get_file(sq2);
    int rank3 = BitboardUtils::get_rank(sq3);
    int file3 = BitboardUtils::get_file(sq3);
    
    // Check if all three squares are on the same rank, file, or diagonal
    if (rank1 == rank2 && rank2 == rank3) return true; // Same rank
    if (file1 == file2 && file2 == file3) return true; // Same file
    
    // Check diagonal alignment
    int rank_diff1 = rank2 - rank1;
    int file_diff1 = file2 - file1;
    int rank_diff2 = rank3 - rank2;
    int file_diff2 = file3 - file2;
    
    return (rank_diff1 * file_diff2 == rank_diff2 * file_diff1) && 
           (std::abs(rank_diff1) == std::abs(file_diff1)) &&
           (std::abs(rank_diff2) == std::abs(file_diff2));
}

// Magic bitboard attack generation functions
// These functions delegate to BitboardUtils which handles PEXT optimization internally
Bitboard MoveGenerator::get_bishop_attacks(int square, Bitboard occupancy) {
    return BitboardUtils::bishop_attacks(square, occupancy);
}

Bitboard MoveGenerator::get_rook_attacks(int square, Bitboard occupancy) {
    return BitboardUtils::rook_attacks(square, occupancy);
}

Bitboard MoveGenerator::get_queen_attacks(int square, Bitboard occupancy) {
    return get_rook_attacks(square, occupancy) | get_bishop_attacks(square, occupancy);
}