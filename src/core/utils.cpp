#include "utils.h"
#include <algorithm>
#include <fstream>
#include <random>
#include <iomanip>
#include <map>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <unistd.h>
#include <sys/resource.h>
#endif

namespace yoki {

// Logger implementation
Logger::Level Logger::current_level_ = Logger::INFO;

void Logger::set_level(Level level) {
    current_level_ = level;
}

void Logger::log(Level level, const std::string& message) {
    if (level >= current_level_) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::cout << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S")
                  << "." << std::setfill('0') << std::setw(3) << ms.count()
                  << "] [" << level_to_string(level) << "] " << message << std::endl;
    }
}

void Logger::debug(const std::string& message) {
    log(Logger::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(Logger::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(Logger::WARNING, message);
}

//TODO: fix logger for error

// void Logger::error(const std::string& message) {
//     log(Logger::ERROR, message);
// }

std::string Logger::level_to_string(Level level) {
    switch (level) {
        case Logger::DEBUG: return "DEBUG";
        case Logger::INFO: return "INFO";
        case Logger::WARNING: return "WARN";
        // case Logger::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

// Timer implementation
Timer::Timer() : running_(false) {
    reset();
}

void Timer::start() {
    start_time_ = std::chrono::steady_clock::now();
    running_ = true;
}

void Timer::stop() {
    if (running_) {
        end_time_ = std::chrono::steady_clock::now();
        running_ = false;
    }
}

void Timer::reset() {
    start_time_ = std::chrono::steady_clock::now();
    end_time_ = start_time_;
    running_ = false;
}

std::chrono::milliseconds Timer::elapsed() const {
    auto end = running_ ? std::chrono::steady_clock::now() : end_time_;
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start_time_);
}

double Timer::elapsed_seconds() const {
    return elapsed().count() / 1000.0;
}

bool Timer::is_running() const {
    return running_;
}

// String utilities
namespace StringUtils {
    std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;
        
        while (std::getline(ss, token, delimiter)) {
            tokens.push_back(token);
        }
        
        return tokens;
    }
    
    std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
        std::vector<std::string> tokens;
        size_t start = 0;
        size_t end = str.find(delimiter);
        
        while (end != std::string::npos) {
            tokens.push_back(str.substr(start, end - start));
            start = end + delimiter.length();
            end = str.find(delimiter, start);
        }
        
        tokens.push_back(str.substr(start));
        return tokens;
    }
    
    std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        
        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }
    
    std::string to_lower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
    
    std::string to_upper(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }
    
    bool starts_with(const std::string& str, const std::string& prefix) {
        return str.length() >= prefix.length() && 
               str.substr(0, prefix.length()) == prefix;
    }
    
    bool ends_with(const std::string& str, const std::string& suffix) {
        return str.length() >= suffix.length() && 
               str.substr(str.length() - suffix.length()) == suffix;
    }
    
    std::string join(const std::vector<std::string>& strings, const std::string& delimiter) {
        if (strings.empty()) return "";
        
        std::ostringstream oss;
        oss << strings[0];
        
        for (size_t i = 1; i < strings.size(); ++i) {
            oss << delimiter << strings[i];
        }
        
        return oss.str();
    }
    
    bool is_number(const std::string& str) {
        if (str.empty()) return false;
        
        size_t start = 0;
        if (str[0] == '-' || str[0] == '+') {
            start = 1;
            if (str.length() == 1) return false;
        }
        
        bool has_dot = false;
        for (size_t i = start; i < str.length(); ++i) {
            if (str[i] == '.') {
                if (has_dot) return false;
                has_dot = true;
            } else if (!std::isdigit(str[i])) {
                return false;
            }
        }
        
        return true;
    }
}

// Math utilities
namespace MathUtils {
    int sign(int value) {
        return (value > 0) ? 1 : (value < 0) ? -1 : 0;
    }
    
    double lerp(double a, double b, double t) {
        return a + t * (b - a);
    }
    
    int random_int(int min_val, int max_val) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(min_val, max_val);
        return dis(gen);
    }
    
    double random_double(double min_val, double max_val) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dis(min_val, max_val);
        return dis(gen);
    }
}

// File utilities
namespace FileUtils {
    bool file_exists(const std::string& filename) {
        std::ifstream file(filename);
        return file.good();
    }
    
    std::string read_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return "";
        }
        
        std::ostringstream oss;
        oss << file.rdbuf();
        return oss.str();
    }
    
    bool write_file(const std::string& filename, const std::string& content) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        file << content;
        return file.good();
    }
    
    std::vector<std::string> read_lines(const std::string& filename) {
        std::vector<std::string> lines;
        std::ifstream file(filename);
        
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                lines.push_back(line);
            }
        }
        
        return lines;
    }
    
    bool write_lines(const std::string& filename, const std::vector<std::string>& lines) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        for (const std::string& line : lines) {
            file << line << std::endl;
        }
        
        return file.good();
    }
}

// Performance counter implementation
PerformanceCounter::PerformanceCounter(const std::string& name) 
    : name_(name), total_time_(0), call_count_(0) {
}

PerformanceCounter::~PerformanceCounter() {
    if (call_count_ > 0) {
        print_stats();
    }
}

void PerformanceCounter::start() {
    timer_.start();
}

void PerformanceCounter::stop() {
    timer_.stop();
    total_time_ += std::chrono::duration_cast<std::chrono::microseconds>(timer_.elapsed());
    call_count_++;
}

void PerformanceCounter::reset() {
    total_time_ = std::chrono::microseconds(0);
    call_count_ = 0;
    timer_.reset();
}

std::chrono::microseconds PerformanceCounter::get_total_time() const {
    return total_time_;
}

int PerformanceCounter::get_call_count() const {
    return call_count_;
}

double PerformanceCounter::get_average_time_ms() const {
    if (call_count_ == 0) return 0.0;
    return total_time_.count() / (1000.0 * call_count_);
}

void PerformanceCounter::print_stats() const {
    std::cout << "Performance [" << name_ << "]: "
              << "calls=" << call_count_
              << ", total=" << (total_time_.count() / 1000.0) << "ms"
              << ", avg=" << get_average_time_ms() << "ms"
              << std::endl;
}

// Memory utilities
namespace MemoryUtils {
    size_t get_memory_usage_kb() {
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            return pmc.WorkingSetSize / 1024;
        }
#else
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
            return usage.ru_maxrss; // Already in KB on Linux
        }
#endif
        return 0;
    }
    
    void print_memory_usage() {
        size_t memory_kb = get_memory_usage_kb();
        std::cout << "Memory usage: " << memory_kb << " KB (" 
                  << (memory_kb / 1024.0) << " MB)" << std::endl;
    }
}

// Chess utilities
namespace ChessUtils {
    bool is_valid_file(char file) {
        return file >= 'a' && file <= 'h';
    }
    
    bool is_valid_rank(char rank) {
        return rank >= '1' && rank <= '8';
    }
    
    bool is_valid_square_name(const std::string& square) {
        return square.length() == 2 && 
               is_valid_file(square[0]) && 
               is_valid_rank(square[1]);
    }
    
    char file_to_char(int file) {
        return 'a' + file;
    }
    
    char rank_to_char(int rank) {
        return '1' + rank;
    }
    
    int char_to_file(char file) {
        return file - 'a';
    }
    
    int char_to_rank(char rank) {
        return rank - '1';
    }
    
    std::string format_time_ms(int milliseconds) {
        if (milliseconds < 1000) {
            return std::to_string(milliseconds) + "ms";
        } else if (milliseconds < 60000) {
            return std::to_string(milliseconds / 1000.0) + "s";
        } else {
            int minutes = milliseconds / 60000;
            int seconds = (milliseconds % 60000) / 1000;
            return std::to_string(minutes) + "m" + std::to_string(seconds) + "s";
        }
    }
    
    std::string format_nodes(int nodes) {
        if (nodes < 1000) {
            return std::to_string(nodes);
        } else if (nodes < 1000000) {
            return std::to_string(nodes / 1000.0) + "K";
        } else {
            return std::to_string(nodes / 1000000.0) + "M";
        }
    }
    
    std::string format_score(int centipawns) {
        if (std::abs(centipawns) > 10000) {
            // Mate score
            int mate_in = (20000 - std::abs(centipawns)) / 2;
            return "#" + std::to_string(centipawns > 0 ? mate_in : -mate_in);
        } else {
            return std::to_string(centipawns / 100.0);
        }
    }
}

// Debug utilities
namespace DebugUtils {
    void print_hex(const void* data, size_t size) {
        const unsigned char* bytes = static_cast<const unsigned char*>(data);
        for (size_t i = 0; i < size; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << static_cast<int>(bytes[i]) << " ";
            if ((i + 1) % 16 == 0) std::cout << std::endl;
        }
        if (size % 16 != 0) std::cout << std::endl;
        std::cout << std::dec;
    }
    
    void print_binary(uint64_t value) {
        for (int i = 63; i >= 0; --i) {
            std::cout << ((value >> i) & 1);
            if (i % 8 == 0 && i > 0) std::cout << " ";
        }
        std::cout << std::endl;
    }
    
    std::string get_stack_trace() {
        // Platform-specific stack trace implementation would go here
        return "Stack trace not implemented for this platform";
    }
}

// Config implementation
Config& Config::instance() {
    static Config instance;
    return instance;
}

void Config::load_from_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;
    
    std::string line;
    while (std::getline(file, line)) {
        line = StringUtils::trim(line);
        if (line.empty() || line[0] == '#') continue;
        
        size_t eq_pos = line.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = StringUtils::trim(line.substr(0, eq_pos));
            std::string value = StringUtils::trim(line.substr(eq_pos + 1));
            values_[key] = value;
        }
    }
}

void Config::save_to_file(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    for (const auto& pair : values_) {
        file << pair.first << " = " << pair.second << std::endl;
    }
}

void Config::set_string(const std::string& key, const std::string& value) {
    values_[key] = value;
}

void Config::set_int(const std::string& key, int value) {
    values_[key] = std::to_string(value);
}

void Config::set_bool(const std::string& key, bool value) {
    values_[key] = value ? "true" : "false";
}

void Config::set_double(const std::string& key, double value) {
    values_[key] = std::to_string(value);
}

std::string Config::get_string(const std::string& key, const std::string& default_value) const {
    auto it = values_.find(key);
    return (it != values_.end()) ? it->second : default_value;
}

int Config::get_int(const std::string& key, int default_value) const {
    auto it = values_.find(key);
    if (it != values_.end()) {
        try {
            return std::stoi(it->second);
        } catch (...) {
            return default_value;
        }
    }
    return default_value;
}

bool Config::get_bool(const std::string& key, bool default_value) const {
    auto it = values_.find(key);
    if (it != values_.end()) {
        std::string value = StringUtils::to_lower(it->second);
        return value == "true" || value == "1" || value == "yes";
    }
    return default_value;
}

double Config::get_double(const std::string& key, double default_value) const {
    auto it = values_.find(key);
    if (it != values_.end()) {
        try {
            return std::stod(it->second);
        } catch (...) {
            return default_value;
        }
    }
    return default_value;
}

bool Config::has_key(const std::string& key) const {
    return values_.find(key) != values_.end();
}

void Config::remove_key(const std::string& key) {
    values_.erase(key);
}

void Config::clear() {
    values_.clear();
}

std::vector<std::string> Config::get_all_keys() const {
    std::vector<std::string> keys;
    for (const auto& pair : values_) {
        keys.push_back(pair.first);
    }
    return keys;
}

} // namespace yoki