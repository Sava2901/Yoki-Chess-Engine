#include "uci.h"
#include "movegen.h"
#include <algorithm>
#include <thread>
#include <future>

namespace yoki {

UCIEngine::UCIEngine() 
    : debug_mode_(false), searching_(false), hash_size_mb_(64), threads_(1), ponder_enabled_(false) {
    board_.reset();
}

void UCIEngine::set_debug(bool debug) {
    debug_mode_ = debug;
}

void UCIEngine::run() {
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        
        process_command(line);
        
        if (line == "quit") {
            break;
        }
    }
}

void UCIEngine::process_command(const std::string& command) {
    std::vector<std::string> tokens = tokenize(command);
    if (tokens.empty()) return;
    
    UCICommand cmd = parse_command(tokens[0]);
    
    switch (cmd) {
        case UCICommand::UCI:
            handle_uci();
            break;
        case UCICommand::DEBUG:
            handle_debug(tokens);
            break;
        case UCICommand::ISREADY:
            handle_isready();
            break;
        case UCICommand::SETOPTION:
            handle_setoption(tokens);
            break;
        case UCICommand::REGISTER:
            handle_register(tokens);
            break;
        case UCICommand::UCINEWGAME:
            handle_ucinewgame();
            break;
        case UCICommand::POSITION:
            handle_position(tokens);
            break;
        case UCICommand::GO:
            handle_go(tokens);
            break;
        case UCICommand::STOP:
            handle_stop();
            break;
        case UCICommand::PONDERHIT:
            handle_ponderhit();
            break;
        case UCICommand::QUIT:
            handle_quit();
            break;
        case UCICommand::UNKNOWN:
        default:
            log_debug("Unknown command: " + command);
            break;
    }
}

void UCIEngine::handle_uci() {
    send_id();
    send_options();
    send_uciok();
}

void UCIEngine::handle_debug(const std::vector<std::string>& tokens) {
    if (tokens.size() >= 2) {
        debug_mode_ = (tokens[1] == "on");
    }
}

void UCIEngine::handle_isready() {
    send_readyok();
}

void UCIEngine::handle_setoption(const std::vector<std::string>& tokens) {
    // Parse: setoption name <name> value <value>
    if (tokens.size() < 5) return;
    
    std::string name;
    std::string value;
    bool parsing_name = false;
    bool parsing_value = false;
    
    for (size_t i = 1; i < tokens.size(); ++i) {
        if (tokens[i] == "name") {
            parsing_name = true;
            parsing_value = false;
        } else if (tokens[i] == "value") {
            parsing_name = false;
            parsing_value = true;
        } else if (parsing_name) {
            if (!name.empty()) name += " ";
            name += tokens[i];
        } else if (parsing_value) {
            if (!value.empty()) value += " ";
            value += tokens[i];
        }
    }
    
    // Handle specific options
    if (name == "Hash") {
        try {
            hash_size_mb_ = std::stoi(value);
            log_debug("Set Hash to " + value + " MB");
        } catch (...) {
            log_debug("Invalid Hash value: " + value);
        }
    } else if (name == "Threads") {
        try {
            threads_ = std::stoi(value);
            log_debug("Set Threads to " + value);
        } catch (...) {
            log_debug("Invalid Threads value: " + value);
        }
    } else if (name == "Ponder") {
        ponder_enabled_ = (value == "true");
        log_debug("Set Ponder to " + value);
    }
}

void UCIEngine::handle_register(const std::vector<std::string>& tokens) {
    // Registration not required for this engine
    (void)tokens;
}

void UCIEngine::handle_ucinewgame() {
    board_.reset();
    // Clear transposition table, history, etc.
    log_debug("New game started");
}

void UCIEngine::handle_position(const std::vector<std::string>& tokens) {
    if (!parse_position(tokens)) {
        log_debug("Failed to parse position command");
    }
}

void UCIEngine::handle_go(const std::vector<std::string>& tokens) {
    if (searching_) {
        log_debug("Already searching, ignoring go command");
        return;
    }
    
    GoParams params = parse_go_command(tokens);
    
    // Calculate search time
    int search_time_ms = calculate_search_time(params);
    
    searching_ = true;
    
    // Start search in a separate thread
    std::thread search_thread([this, params, search_time_ms]() {
        SearchResult result;
        
        if (params.depth > 0) {
            result = search_engine_.search_depth(board_, params.depth);
        } else {
            result = search_engine_.search(board_, search_time_ms);
        }
        
        searching_ = false;
        
        if (result.best_move.from != -1 && result.best_move.to != -1) {
            send_info(result);
            send_bestmove(result.best_move);
        } else {
            send_bestmove(Move(-1, -1)); // No move found
        }
    });
    
    search_thread.detach();
}

void UCIEngine::handle_stop() {
    searching_ = false;
    log_debug("Search stopped");
}

void UCIEngine::handle_ponderhit() {
    // Convert pondering to normal search
    log_debug("Ponder hit");
}

void UCIEngine::handle_quit() {
    searching_ = false;
    log_debug("Engine quitting");
}

void UCIEngine::send_id() {
    std::cout << "id name Yoki Chess Engine 1.0" << std::endl;
    std::cout << "id author Yoki Chess Team" << std::endl;
}

void UCIEngine::send_options() {
    std::cout << "option name Hash type spin default 64 min 1 max 1024" << std::endl;
    std::cout << "option name Threads type spin default 1 min 1 max 16" << std::endl;
    std::cout << "option name Ponder type check default false" << std::endl;
}

void UCIEngine::send_uciok() {
    std::cout << "uciok" << std::endl;
}

void UCIEngine::send_readyok() {
    std::cout << "readyok" << std::endl;
}

void UCIEngine::send_bestmove(const Move& move, const Move& ponder) {
    std::cout << "bestmove ";
    
    if (move.from == -1 || move.to == -1) {
        std::cout << "(none)";
    } else {
        std::cout << move_to_uci_string(move);
    }
    
    if (ponder.from != -1 && ponder.to != -1) {
        std::cout << " ponder " << move_to_uci_string(ponder);
    }
    
    std::cout << std::endl;
}

void UCIEngine::send_info(const SearchResult& result) {
    std::cout << "info";
    
    if (result.depth > 0) {
        std::cout << " depth " << result.depth;
    }
    
    std::cout << " score cp " << result.score;
    std::cout << " nodes " << result.nodes_searched;
    std::cout << " time " << result.time_taken.count();
    
    if (result.time_taken.count() > 0) {
        int nps = (result.nodes_searched * 1000) / result.time_taken.count();
        std::cout << " nps " << nps;
    }
    
    if (result.best_move.from != -1 && result.best_move.to != -1) {
        std::cout << " pv " << move_to_uci_string(result.best_move);
    }
    
    std::cout << std::endl;
}

UCICommand UCIEngine::parse_command(const std::string& command) {
    if (command == "uci") return UCICommand::UCI;
    if (command == "debug") return UCICommand::DEBUG;
    if (command == "isready") return UCICommand::ISREADY;
    if (command == "setoption") return UCICommand::SETOPTION;
    if (command == "register") return UCICommand::REGISTER;
    if (command == "ucinewgame") return UCICommand::UCINEWGAME;
    if (command == "position") return UCICommand::POSITION;
    if (command == "go") return UCICommand::GO;
    if (command == "stop") return UCICommand::STOP;
    if (command == "ponderhit") return UCICommand::PONDERHIT;
    if (command == "quit") return UCICommand::QUIT;
    
    return UCICommand::UNKNOWN;
}

std::vector<std::string> UCIEngine::tokenize(const std::string& str) {
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

GoParams UCIEngine::parse_go_command(const std::vector<std::string>& tokens) {
    GoParams params;
    
    for (size_t i = 1; i < tokens.size(); ++i) {
        const std::string& token = tokens[i];
        
        if (token == "infinite") {
            params.infinite = true;
        } else if (token == "ponder") {
            params.ponder = true;
        } else if (token == "wtime" && i + 1 < tokens.size()) {
            params.wtime = std::stoi(tokens[++i]);
        } else if (token == "btime" && i + 1 < tokens.size()) {
            params.btime = std::stoi(tokens[++i]);
        } else if (token == "winc" && i + 1 < tokens.size()) {
            params.winc = std::stoi(tokens[++i]);
        } else if (token == "binc" && i + 1 < tokens.size()) {
            params.binc = std::stoi(tokens[++i]);
        } else if (token == "movestogo" && i + 1 < tokens.size()) {
            params.movestogo = std::stoi(tokens[++i]);
        } else if (token == "depth" && i + 1 < tokens.size()) {
            params.depth = std::stoi(tokens[++i]);
        } else if (token == "nodes" && i + 1 < tokens.size()) {
            params.nodes = std::stoi(tokens[++i]);
        } else if (token == "mate" && i + 1 < tokens.size()) {
            params.mate = std::stoi(tokens[++i]);
        } else if (token == "movetime" && i + 1 < tokens.size()) {
            params.movetime = std::stoi(tokens[++i]);
        } else if (token == "searchmoves") {
            // Parse remaining tokens as moves
            for (size_t j = i + 1; j < tokens.size(); ++j) {
                Move move = parse_move(tokens[j]);
                if (move.from != -1 && move.to != -1) {
                    params.searchmoves.push_back(move);
                }
            }
            break;
        }
    }
    
    return params;
}

int UCIEngine::calculate_search_time(const GoParams& params) {
    if (params.infinite) {
        return std::numeric_limits<int>::max();
    }
    
    if (params.movetime > 0) {
        return params.movetime;
    }
    
    // Calculate time based on remaining time and increment
    Color side = board_.get_side_to_move();
    int remaining_time = (side == WHITE) ? params.wtime : params.btime;
    int increment = (side == WHITE) ? params.winc : params.binc;
    
    if (remaining_time <= 0) {
        return 5000; // Default 5 seconds
    }
    
    // Simple time management: use 1/30 of remaining time plus increment
    int allocated_time = remaining_time / 30 + increment;
    
    // Ensure minimum and maximum bounds
    allocated_time = std::max(100, std::min(allocated_time, remaining_time / 2));
    
    return allocated_time;
}

bool UCIEngine::parse_position(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) return false;
    
    size_t moves_index = 0;
    
    if (tokens[1] == "startpos") {
        board_.reset();
        moves_index = 2;
    } else if (tokens[1] == "fen") {
        if (tokens.size() < 8) return false;
        
        // Reconstruct FEN string
        std::string fen = tokens[2] + " " + tokens[3] + " " + tokens[4] + " " + 
                         tokens[5] + " " + tokens[6] + " " + tokens[7];
        
        if (!board_.load_fen(fen)) {
            return false;
        }
        
        moves_index = 8;
    } else {
        return false;
    }
    
    // Apply moves if "moves" keyword is present
    if (moves_index < tokens.size() && tokens[moves_index] == "moves") {
        for (size_t i = moves_index + 1; i < tokens.size(); ++i) {
            Move move = parse_move(tokens[i]);
            if (move.from == -1 || move.to == -1) {
                log_debug("Invalid move: " + tokens[i]);
                return false;
            }
            
            if (!board_.make_move(move)) {
                log_debug("Illegal move: " + tokens[i]);
                return false;
            }
        }
    }
    
    return true;
}

Move UCIEngine::parse_move(const std::string& move_str) {
    return parse_move_string(move_str);
}

void UCIEngine::log_debug(const std::string& message) {
    if (debug_mode_) {
        std::cout << "info string " << message << std::endl;
    }
}

// Utility functions
std::string move_to_uci_string(const Move& move) {
    if (move.from == -1 || move.to == -1) {
        return "0000";
    }
    
    return move.to_string();
}

Move uci_string_to_move(const std::string& uci_str) {
    return parse_move_string(uci_str);
}

bool is_valid_uci_move(const std::string& move_str) {
    return is_valid_move_format(move_str);
}

} // namespace yoki