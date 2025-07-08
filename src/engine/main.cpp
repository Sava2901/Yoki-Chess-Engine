#include "core/uci.h"
#include "core/utils.h"
#include <iostream>
#include <string>
#include <memory>

using namespace yoki;

int main(int argc, char* argv[]) {
    // Initialize logging
    Logger::set_level(Logger::INFO);
    
    // Print engine information
    Logger::info("Yoki Chess Engine v1.0.0");
    Logger::info("UCI-compatible chess engine");
    Logger::info("Starting engine...");
    
    try {
        // Create UCI engine instance
        auto uci_engine = std::make_unique<UCIEngine>();
        
        // Check for command line arguments
        bool debug_mode = false;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--debug" || arg == "-d") {
                debug_mode = true;
                Logger::set_level(Logger::DEBUG);
                Logger::debug("Debug mode enabled");
            } else if (arg == "--help" || arg == "-h") {
                std::cout << "Yoki Chess Engine v1.0.0\n";
                std::cout << "Usage: yoki-engine [options]\n";
                std::cout << "Options:\n";
                std::cout << "  --debug, -d    Enable debug mode\n";
                std::cout << "  --help, -h     Show this help message\n";
                std::cout << "  --version, -v  Show version information\n";
                return 0;
            } else if (arg == "--version" || arg == "-v") {
                std::cout << "Yoki Chess Engine v1.0.0\n";
                std::cout << "Built with C++17\n";
                std::cout << "UCI Protocol Compatible\n";
                return 0;
            }
        }
        
        // Set debug mode if requested
        if (debug_mode) {
            uci_engine->set_debug(true);
        }
        
        Logger::info("Engine initialized successfully");
        Logger::info("Waiting for UCI commands...");
        
        // Start the UCI main loop
        uci_engine->run();
        
        Logger::info("Engine shutting down");
        
    } catch (const std::exception& e) {
        Logger::error("Engine error: " + std::string(e.what()));
        return 1;
    } catch (...) {
        Logger::error("Unknown engine error occurred");
        return 1;
    }
    
    return 0;
}