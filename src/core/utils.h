#ifndef YOKI_UTILS_H
#define YOKI_UTILS_H

#include <string>
#include <vector>
#include <chrono>
#include <iostream>
#include <sstream>
#include <map>

namespace yoki {

// Logging utilities
class Logger {
public:
    enum Level {
        DEBUG = 0,
        INFO = 1,
        WARNING = 2,
        ERROR = 3
    };
    
    static void set_level(Level level);
    static void log(Level level, const std::string& message);
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);
    
private:
    static Level current_level_;
    static std::string level_to_string(Level level);
};

// Timer utility
class Timer {
public:
    Timer();
    
    void start();
    void stop();
    void reset();
    
    std::chrono::milliseconds elapsed() const;
    double elapsed_seconds() const;
    bool is_running() const;
    
private:
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point end_time_;
    bool running_;
};

// String utilities
namespace StringUtils {
    std::vector<std::string> split(const std::string& str, char delimiter);
    std::vector<std::string> split(const std::string& str, const std::string& delimiter);
    std::string trim(const std::string& str);
    std::string to_lower(const std::string& str);
    std::string to_upper(const std::string& str);
    bool starts_with(const std::string& str, const std::string& prefix);
    bool ends_with(const std::string& str, const std::string& suffix);
    std::string join(const std::vector<std::string>& strings, const std::string& delimiter);
    bool is_number(const std::string& str);
}

// Math utilities
namespace MathUtils {
    template<typename T>
    T clamp(T value, T min_val, T max_val) {
        return std::max(min_val, std::min(value, max_val));
    }
    
    int sign(int value);
    double lerp(double a, double b, double t);
    int random_int(int min_val, int max_val);
    double random_double(double min_val = 0.0, double max_val = 1.0);
}

// File utilities
namespace FileUtils {
    bool file_exists(const std::string& filename);
    std::string read_file(const std::string& filename);
    bool write_file(const std::string& filename, const std::string& content);
    std::vector<std::string> read_lines(const std::string& filename);
    bool write_lines(const std::string& filename, const std::vector<std::string>& lines);
}

// Performance utilities
class PerformanceCounter {
public:
    PerformanceCounter(const std::string& name);
    ~PerformanceCounter();
    
    void start();
    void stop();
    void reset();
    
    std::chrono::microseconds get_total_time() const;
    int get_call_count() const;
    double get_average_time_ms() const;
    
    void print_stats() const;
    
private:
    std::string name_;
    Timer timer_;
    std::chrono::microseconds total_time_;
    int call_count_;
};

// Memory utilities
namespace MemoryUtils {
    size_t get_memory_usage_kb();
    void print_memory_usage();
}

// Chess-specific utilities
namespace ChessUtils {
    bool is_valid_file(char file);
    bool is_valid_rank(char rank);
    bool is_valid_square_name(const std::string& square);
    
    char file_to_char(int file);
    char rank_to_char(int rank);
    int char_to_file(char file);
    int char_to_rank(char rank);
    
    std::string format_time_ms(int milliseconds);
    std::string format_nodes(int nodes);
    std::string format_score(int centipawns);
}

// Debug utilities
namespace DebugUtils {
    void print_hex(const void* data, size_t size);
    void print_binary(uint64_t value);
    std::string get_stack_trace();
    
    template<typename T>
    void print_vector(const std::vector<T>& vec, const std::string& name = "") {
        if (!name.empty()) {
            std::cout << name << ": ";
        }
        std::cout << "[";
        for (size_t i = 0; i < vec.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << vec[i];
        }
        std::cout << "]" << std::endl;
    }
}

// Configuration utilities
class Config {
public:
    static Config& instance();
    
    void load_from_file(const std::string& filename);
    void save_to_file(const std::string& filename) const;
    
    void set_string(const std::string& key, const std::string& value);
    void set_int(const std::string& key, int value);
    void set_bool(const std::string& key, bool value);
    void set_double(const std::string& key, double value);
    
    std::string get_string(const std::string& key, const std::string& default_value = "") const;
    int get_int(const std::string& key, int default_value = 0) const;
    bool get_bool(const std::string& key, bool default_value = false) const;
    double get_double(const std::string& key, double default_value = 0.0) const;
    
    bool has_key(const std::string& key) const;
    void remove_key(const std::string& key);
    void clear();
    
    std::vector<std::string> get_all_keys() const;
    
private:
    Config() = default;
    std::map<std::string, std::string> values_;
};

} // namespace yoki

#endif // YOKI_UTILS_H