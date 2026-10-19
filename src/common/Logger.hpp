#pragma once

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <string>

namespace logger {

inline std::mutex& mutex() {
    static std::mutex value;
    return value;
}

inline std::ofstream& stream() {
    static std::ofstream value;
    return value;
}

inline bool& opened() {
    static bool value = false;
    return value;
}

inline void init(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mutex());
    stream().open(filePath, std::ios::out | std::ios::trunc);
    opened() = stream().is_open();
}

inline void shutdown() {
    std::lock_guard<std::mutex> lock(mutex());
    if (stream().is_open()) {
        stream().flush();
        stream().close();
    }
    opened() = false;
}

inline void raw(const char* level, const std::string& message) {
    const std::string line = std::string("[") + level + "] " + message;
    std::lock_guard<std::mutex> lock(mutex());
    if (opened()) {
        stream() << line << '\n';
        stream().flush();
    }
    OutputDebugStringA((line + "\n").c_str());
}

inline void info(const char* format, ...) {
    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    raw("info ", buffer);
}

inline void warn(const char* format, ...) {
    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    raw("warn ", buffer);
}

inline void error(const char* format, ...) {
    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    raw("error", buffer);
}

} // namespace logger
