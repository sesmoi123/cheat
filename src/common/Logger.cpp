#include "Logger.hpp"

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <mutex>

namespace logger {

namespace {

std::mutex g_mutex;
FILE* g_file = nullptr;

void emit(const char* level, const char* format, va_list args) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_file) {
        return;
    }

    SYSTEMTIME now{};
    GetLocalTime(&now);
    std::fprintf(g_file, "[%02u:%02u:%02u] [%s] ", now.wHour, now.wMinute, now.wSecond, level);
    std::vfprintf(g_file, format, args);
    std::fputc('\n', g_file);
    std::fflush(g_file);
}

} // namespace

void init(const std::string& path) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_file) {
        return;
    }

    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);

    if (fopen_s(&g_file, path.c_str(), "w") != 0) {
        g_file = nullptr;
    }
}

void shutdown() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_file) {
        std::fclose(g_file);
        g_file = nullptr;
    }
}

void info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    emit("info", format, args);
    va_end(args);
}

void warn(const char* format, ...) {
    va_list args;
    va_start(args, format);
    emit("warn", format, args);
    va_end(args);
}

void error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    emit("err ", format, args);
    va_end(args);
}

} // namespace logger
