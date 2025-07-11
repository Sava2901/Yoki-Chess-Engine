#ifndef FENUTILS_H
#define FENUTILS_H

#include <string>
#include <string_view>
#include <vector>
#include <array>

// Forward declaration
class Board;

struct FENComponents {
    std::string piece_placement;
    char active_color;
    std::string castling_rights;
    std::string en_passant_target;
    int halfmove_clock;
    int fullmove_number;
};

class FENUtils {
public:
    // Constants
    static constexpr int BOARD_SIZE = 8;
    static constexpr int FEN_COMPONENTS = 6;
    static constexpr std::array<char, 12> VALID_PIECES = {'p', 'r', 'n', 'b', 'q', 'k', 'P', 'R', 'N', 'B', 'Q', 'K'};
    
    // Main FEN validation and parsing
    static bool is_valid_fen(std::string_view fen);
    static FENComponents parse_fen(std::string_view fen);
    
    // Individual component validation
    static bool validate_piece_placement(std::string_view placement);
    static bool validate_active_color(std::string_view color);
    static bool validate_castling_rights(std::string_view rights);
    static bool validate_en_passant(std::string_view target);
    static bool validate_number(std::string_view num);
    
    // FEN generation
    static std::string create_fen(const FENComponents& components);
    static std::string create_fen(std::string_view piece_placement, char active_color, 
                                  std::string_view castling_rights, std::string_view en_passant_target, 
                                  int halfmove_clock, int fullmove_number);
    
    // Piece validation
    static bool is_valid_piece(char piece);
    
private:
    // Helper methods
    static std::vector<std::string> split_fen(std::string_view fen);
};

#endif // FENUTILS_H