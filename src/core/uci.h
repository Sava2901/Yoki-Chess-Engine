#ifndef YOKI_UCI_H
#define YOKI_UCI_H

#include "board.h"
#include "search.h"
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

namespace yoki {

// UCI command types
enum class UCICommand {
    UCI,
    DEBUG,
    ISREADY,
    SETOPTION,
    REGISTER,
    UCINEWGAME,
    POSITION,
    GO,
    STOP,
    PONDERHIT,
    QUIT,
    UNKNOWN
};

// UCI go command parameters
struct GoParams {
    bool infinite = false;
    bool ponder = false;
    int wtime = -1;        // White time in milliseconds
    int btime = -1;        // Black time in milliseconds
    int winc = 0;          // White increment per move
    int binc = 0;          // Black increment per move
    int movestogo = -1;    // Moves to next time control
    int depth = -1;        // Search depth limit
    int nodes = -1;        // Node limit
    int mate = -1;         // Mate in X moves
    int movetime = -1;     // Time for this move in milliseconds
    std::vector<Move> searchmoves; // Restrict search to these moves
    
    GoParams() = default;
};

// UCI engine interface
class UCIEngine {
public:
    UCIEngine();
    
    // Main UCI loop
    void run();

    // Set debug mode
    void set_debug(bool debug);
    
    // Process a single UCI command
    void process_command(const std::string& command);
    
    // UCI command handlers
    void handle_uci();
    void handle_debug(const std::vector<std::string>& tokens);
    void handle_isready();
    void handle_setoption(const std::vector<std::string>& tokens);
    void handle_register(const std::vector<std::string>& tokens);
    void handle_ucinewgame();
    void handle_position(const std::vector<std::string>& tokens);
    void handle_go(const std::vector<std::string>& tokens);
    void handle_stop();
    void handle_ponderhit();
    void handle_quit();
    
    // Output functions
    void send_id();
    void send_options();
    void send_uciok();
    void send_readyok();
    void send_bestmove(const Move& move, const Move& ponder = Move(-1, -1));
    void send_info(const SearchResult& result);
    
private:
    Board board_;
    SearchEngine search_engine_;
    bool debug_mode_;
    bool searching_;
    
    // Engine options
    int hash_size_mb_;
    int threads_;
    bool ponder_enabled_;
    
    // Helper functions
    UCICommand parse_command(const std::string& command);
    std::vector<std::string> tokenize(const std::string& str);
    GoParams parse_go_command(const std::vector<std::string>& tokens);
    int calculate_search_time(const GoParams& params);
    bool parse_position(const std::vector<std::string>& tokens);
    Move parse_move(const std::string& move_str);
    
    // Logging
    void log_debug(const std::string& message);
};

// Utility functions
std::string move_to_uci_string(const Move& move);
Move uci_string_to_move(const std::string& uci_str);
bool is_valid_uci_move(const std::string& move_str);

} // namespace yoki

#endif // YOKI_UCI_H