#include "core/board.h"
#include "core/movegen.h"
#include "core/utils.h"
#include <iostream>
#include <string>
#include <vector>
#include <memory>

using namespace yoki;

struct ValidationResult {
    bool is_valid;
    std::string error_message;
    std::string move_notation;
};

class MoveValidator {
public:
    MoveValidator() : board_(std::make_unique<Board>()), movegen_(std::make_unique<MoveGenerator>()) {}
    
    ValidationResult validate_move_from_fen(const std::string& fen, const std::string& move_str) {
        ValidationResult result;
        result.is_valid = false;
        
        try {
            // Set up board from FEN
            if (!board_->load_fen(fen)) {
                result.error_message = "Invalid FEN string: " + fen;
                return result;
            }
            
            // Parse move string (expecting format like "e2e4", "e7e8q" for promotion)
            Move move = parse_move_string(move_str);
            if (move.from == move.to) {
                result.error_message = "Invalid move format: " + move_str;
                return result;
            }
            
            // Generate legal moves
            movegen_ = std::make_unique<MoveGenerator>(*board_);
            std::vector<Move> legal_moves = movegen_->generate_legal_moves();
            
            // Check if the move is in the legal moves list
            bool found = false;
            for (const Move& legal_move : legal_moves) {
                if (moves_equal(move, legal_move)) {
                    found = true;
                    result.move_notation = move_to_string(legal_move);
                    break;
                }
            }
            
            if (found) {
                result.is_valid = true;
                result.error_message = "";
            } else {
                result.error_message = "Illegal move: " + move_str;
            }
            
        } catch (const std::exception& e) {
            result.error_message = "Validation error: " + std::string(e.what());
        }
        
        return result;
    }
    
    std::vector<std::string> get_legal_moves_from_fen(const std::string& fen) {
        std::vector<std::string> move_strings;
        
        try {
            if (!board_->load_fen(fen)) {
                return move_strings; // Return empty vector for invalid FEN
            }
            
            movegen_ = std::make_unique<MoveGenerator>(*board_);
            std::vector<Move> legal_moves = movegen_->generate_legal_moves();
            
            for (const Move& move : legal_moves) {
                move_strings.push_back(move_to_string(move));
            }
            
        } catch (const std::exception& e) {
            // Return empty vector on error
        }
        
        return move_strings;
    }
    
private:
    std::unique_ptr<Board> board_;
    std::unique_ptr<MoveGenerator> movegen_;
    
    Move parse_move_string(const std::string& move_str) {
        Move move;
        move.from = move.to = 64; // Invalid squares
        
        if (move_str.length() < 4) {
            return move;
        }
        
        // Parse from square (e.g., "e2")
        if (move_str[0] >= 'a' && move_str[0] <= 'h' && 
            move_str[1] >= '1' && move_str[1] <= '8') {
            int file = move_str[0] - 'a';
            int rank = move_str[1] - '1';
            move.from = rank * 8 + file;
        } else {
            return move;
        }
        
        // Parse to square (e.g., "e4")
        if (move_str[2] >= 'a' && move_str[2] <= 'h' && 
            move_str[3] >= '1' && move_str[3] <= '8') {
            int file = move_str[2] - 'a';
            int rank = move_str[3] - '1';
            move.to = rank * 8 + file;
        } else {
            return move;
        }
        
        // Parse promotion piece (if present)
        move.promotion = PieceType::EMPTY;
        if (move_str.length() >= 5) {
            char promo = std::tolower(move_str[4]);
            switch (promo) {
                case 'q': move.promotion = PieceType::QUEEN; break;
                case 'r': move.promotion = PieceType::ROOK; break;
                case 'b': move.promotion = PieceType::BISHOP; break;
                case 'n': move.promotion = PieceType::KNIGHT; break;
            }
        }
        
        return move;
    }
    
    bool moves_equal(const Move& a, const Move& b) {
        return a.from == b.from && a.to == b.to && a.promotion == b.promotion;
    }
    
    std::string move_to_string(const Move& move) {
        std::string result;
        
        // From square
        result += static_cast<char>('a' + (move.from % 8));
        result += static_cast<char>('1' + (move.from / 8));
        
        // To square
        result += static_cast<char>('a' + (move.to % 8));
        result += static_cast<char>('1' + (move.to / 8));
        
        // Promotion piece
        if (move.promotion != PieceType::EMPTY) {
            switch (move.promotion) {
                case PieceType::QUEEN: result += 'q'; break;
                case PieceType::ROOK: result += 'r'; break;
                case PieceType::BISHOP: result += 'b'; break;
                case PieceType::KNIGHT: result += 'n'; break;
                default: break;
            }
        }
        
        return result;
    }
};

void print_usage() {
    std::cout << "Yoki Move Validator v1.0.0\n";
    std::cout << "Usage: yoki-validator [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --validate <fen> <move>  Validate a specific move\n";
    std::cout << "  --list-moves <fen>       List all legal moves\n";
    std::cout << "  --interactive           Interactive mode\n";
    std::cout << "  --help, -h              Show this help message\n";
    std::cout << "  --version, -v           Show version information\n";
    std::cout << "\nExamples:\n";
    std::cout << "  yoki-validator --validate \"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\" e2e4\n";
    std::cout << "  yoki-validator --list-moves \"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\"\n";
}

void interactive_mode() {
    MoveValidator validator;
    std::string input;
    
    std::cout << "Yoki Move Validator - Interactive Mode\n";
    std::cout << "Commands:\n";
    std::cout << "  validate <fen> <move>  - Validate a move\n";
    std::cout << "  list <fen>             - List legal moves\n";
    std::cout << "  quit                   - Exit\n";
    std::cout << "\n";
    
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, input)) {
            break;
        }
        
        input = StringUtils::trim(input);
        if (input.empty()) continue;
        
        std::vector<std::string> tokens = StringUtils::split(input, ' ');
        if (tokens.empty()) continue;
        
        std::string command = StringUtils::to_lower(tokens[0]);
        
        if (command == "quit" || command == "exit") {
            break;
        } else if (command == "validate" && tokens.size() >= 3) {
            std::string fen = tokens[1];
            std::string move = tokens[2];
            
            // Handle quoted FEN strings
            if (fen.front() == '"' && tokens.size() > 3) {
                // Reconstruct FEN from multiple tokens
                fen = "";
                for (size_t i = 1; i < tokens.size() - 1; ++i) {
                    if (i > 1) fen += " ";
                    fen += tokens[i];
                }
                move = tokens.back();
                // Remove quotes
                if (fen.front() == '"') fen = fen.substr(1);
                if (fen.back() == '"') fen = fen.substr(0, fen.length() - 1);
            }
            
            ValidationResult result = validator.validate_move_from_fen(fen, move);
            if (result.is_valid) {
                std::cout << "VALID: " << result.move_notation << std::endl;
            } else {
                std::cout << "INVALID: " << result.error_message << std::endl;
            }
        } else if (command == "list" && tokens.size() >= 2) {
            std::string fen = tokens[1];
            
            // Handle quoted FEN strings
            if (fen.front() == '"' && tokens.size() > 2) {
                fen = "";
                for (size_t i = 1; i < tokens.size(); ++i) {
                    if (i > 1) fen += " ";
                    fen += tokens[i];
                }
                // Remove quotes
                if (fen.front() == '"') fen = fen.substr(1);
                if (fen.back() == '"') fen = fen.substr(0, fen.length() - 1);
            }
            
            std::vector<std::string> moves = validator.get_legal_moves_from_fen(fen);
            if (moves.empty()) {
                std::cout << "No legal moves (invalid FEN or checkmate/stalemate)" << std::endl;
            } else {
                std::cout << "Legal moves (" << moves.size() << "): ";
                for (size_t i = 0; i < moves.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << moves[i];
                }
                std::cout << std::endl;
            }
        } else {
            std::cout << "Unknown command. Type 'quit' to exit." << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    // Initialize logging
    Logger::set_level(Logger::WARNING); // Minimal logging for validator
    
    if (argc < 2) {
        interactive_mode();
        return 0;
    }
    
    std::string command = argv[1];
    
    if (command == "--help" || command == "-h") {
        print_usage();
        return 0;
    } else if (command == "--version" || command == "-v") {
        std::cout << "Yoki Move Validator v1.0.0\n";
        std::cout << "Built with C++17\n";
        return 0;
    } else if (command == "--interactive") {
        interactive_mode();
        return 0;
    } else if (command == "--validate" && argc >= 4) {
        std::string fen = argv[2];
        std::string move = argv[3];
        
        MoveValidator validator;
        ValidationResult result = validator.validate_move_from_fen(fen, move);
        
        if (result.is_valid) {
            std::cout << "VALID" << std::endl;
            return 0;
        } else {
            std::cout << "INVALID: " << result.error_message << std::endl;
            return 1;
        }
    } else if (command == "--list-moves" && argc >= 3) {
        std::string fen = argv[2];
        
        MoveValidator validator;
        std::vector<std::string> moves = validator.get_legal_moves_from_fen(fen);
        
        if (moves.empty()) {
            std::cout << "No legal moves" << std::endl;
            return 1;
        } else {
            for (const std::string& move : moves) {
                std::cout << move << std::endl;
            }
            return 0;
        }
    } else {
        std::cerr << "Invalid arguments. Use --help for usage information." << std::endl;
        return 1;
    }
}