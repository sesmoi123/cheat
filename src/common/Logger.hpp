#pragma once

#include <string>

namespace logger {

void init(const std::string& path);
void shutdown();

void info(const char* format, ...);
void warn(const char* format, ...);
void error(const char* format, ...);

} // namespace logger
