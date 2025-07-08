#ifndef FENUTILS_H
#define FENUTILS_H

#include <string>
#include <vector>

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
    // Main FEN validation and parsing
    static bool is_valid_fen(const std::string& fen);
    static FENComponents parse_fen(const std::string& fen);
    
    // Individual component validation
    static bool validate_piece_placement(const std::string& placement);
    static bool validate_active_color(const std::string& color);
    static bool validate_castling_rights(const std::string& rights);
    static bool validate_en_passant(const std::string& target);
    static bool validate_number(const std::string& num);
    
    // FEN generation
    static std::string create_fen(const FENComponents& components);
    
private:
    // Helper methods
    static std::vector<std::string> split_fen(const std::string& fen);
};

#endif // FENUTILS_H