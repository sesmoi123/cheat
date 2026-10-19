add_library(common STATIC
    Logger.hpp Logger.cpp
    Memory.hpp Memory.cpp
    Scanner.hpp Scanner.cpp
    Json.hpp Json.cpp
    Paths.hpp Paths.cpp)

target_include_directories(common PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(common PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0601)
