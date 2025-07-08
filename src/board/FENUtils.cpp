#include "FENUtils.h"
#include <iostream>
#include <sstream>
#include <cctype>
#include <algorithm>

bool FENUtils::is_valid_fen(const std::string& fen) {
    if (fen.empty()) {
        std::cout << "FEN validation failed: Empty string" << std::endl;
        return false;
    }
    
    // Split FEN into its 6 components
    std::vector<std::string> parts = split_fen(fen);
    
    if (parts.size() != 6) {
        std::cout << "FEN validation failed: Expected 6 parts, got " << parts.size() << std::endl;
        return false;
    }
    
    // Validate piece placement (part 0)
    if (!validate_piece_placement(parts[0])) {
        std::cout << "FEN validation failed: Invalid piece placement" << std::endl;
        return false;
    }
    
    // Validate active color (part 1)
    if (!validate_active_color(parts[1])) {
        std::cout << "FEN validation failed: Invalid active color '" << parts[1] << "'" << std::endl;
        return false;
    }
    
    // Validate castling rights (part 2)
    if (!validate_castling_rights(parts[2])) {
        std::cout << "FEN validation failed: Invalid castling rights" << std::endl;
        return false;
    }
    
    // Validate en passant target (part 3)
    if (!validate_en_passant(parts[3])) {
        std::cout << "FEN validation failed: Invalid en passant target" << std::endl;
        return false;
    }
    
    // Validate halfmove clock (part 4)
    if (!validate_number(parts[4])) {
        std::cout << "FEN validation failed: Invalid halfmove clock" << std::endl;
        return false;
    }
    
    // Validate fullmove number (part 5)
    if (!validate_number(parts[5]) || std::stoi(parts[5]) < 1) {
        std::cout << "FEN validation failed: Invalid fullmove number" << std::endl;
        return false;
    }
    
    return true;
}

FENComponents FENUtils::parse_fen(const std::string& fen) {
    std::cout << "Parsing FEN: " << fen << std::endl;
    
    std::vector<std::string> parts = split_fen(fen);
    
    FENComponents components;
    
    // Parse piece placement
    components.piece_placement = parts[0];
    
    // Parse active color
    components.active_color = (parts[1] == "w") ? 'w' : 'b';
    std::cout << "Active color: " << components.active_color << std::endl;
    
    // Parse castling rights
    components.castling_rights = parts[2];
    std::cout << "Castling rights: " << components.castling_rights << std::endl;
    
    // Parse en passant target
    components.en_passant_target = parts[3];
    std::cout << "En passant target: " << components.en_passant_target << std::endl;
    
    // Parse halfmove clock
    components.halfmove_clock = std::stoi(parts[4]);
    std::cout << "Halfmove clock: " << components.halfmove_clock << std::endl;
    
    // Parse fullmove number
    components.fullmove_number = std::stoi(parts[5]);
    std::cout << "Fullmove number: " << components.fullmove_number << std::endl;
    
    return components;
}

bool FENUtils::validate_piece_placement(const std::string& placement) {
    std::vector<std::string> ranks;
    std::istringstream iss(placement);
    std::string rank;
    
    // Split by '/'
    while (std::getline(iss, rank, '/')) {
        ranks.push_back(rank);
    }
    
    if (ranks.size() != 8) {
        return false;
    }
    
    for (const std::string& r : ranks) {
        int file_count = 0;
        for (char c : r) {
            if (std::isdigit(c)) {
                int empty_squares = c - '0';
                if (empty_squares < 1 || empty_squares > 8) {
                    return false;
                }
                file_count += empty_squares;
            } else if (c == 'p' || c == 'r' || c == 'n' || c == 'b' || c == 'q' || c == 'k' ||
                      c == 'P' || c == 'R' || c == 'N' || c == 'B' || c == 'Q' || c == 'K') {
                file_count++;
            } else {
                return false; // Invalid character
            }
        }
        if (file_count != 8) {
            return false; // Each rank must have exactly 8 squares
        }
    }
    
    return true;
}

bool FENUtils::validate_active_color(const std::string& color) {
    return color == "w" || color == "b";
}

bool FENUtils::validate_castling_rights(const std::string& rights) {
    if (rights == "-") {
        return true;
    }

    // Check for valid characters and no duplicates
    std::string valid_chars = "KQkq";
    for (char c : rights) {
        if (valid_chars.find(c) == std::string::npos) {
            return false;
        }
    }
    
    // Check for duplicates
    std::string sorted_rights = rights;
    std::sort(sorted_rights.begin(), sorted_rights.end());
    for (size_t i = 1; i < sorted_rights.length(); i++) {
        if (sorted_rights[i] == sorted_rights[i-1]) {
            return false;
        }
    }
    
    return true;
}

bool FENUtils::validate_en_passant(const std::string& target) {
    if (target == "-") {
        return true;
    }
    
    if (target.length() != 2) {
        return false;
    }
    
    char file = target[0];
    char rank = target[1];
    
    // File must be a-h, rank must be 3 or 6
    return (file >= 'a' && file <= 'h') && (rank == '3' || rank == '6');
}

bool FENUtils::validate_number(const std::string& num) {
    if (num.empty()) {
        return false;
    }

    if (!std::all_of(num.begin(), num.end(), ::isdigit)) {
        return false;
    }
    
    return true;
}

std::string FENUtils::create_fen(const FENComponents& components) {
    return components.piece_placement + " " +
           components.active_color + " " +
           components.castling_rights + " " +
           components.en_passant_target + " " +
           std::to_string(components.halfmove_clock) + " " +
           std::to_string(components.fullmove_number);
}

std::vector<std::string> FENUtils::split_fen(const std::string& fen) {
    std::istringstream iss(fen);
    std::vector<std::string> parts;
    std::string part;
    
    while (iss >> part) {
        parts.push_back(part);
    }
    
    return parts;
}