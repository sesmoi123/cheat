add_library(RetracCheat SHARED
    dllmain.cpp
    Cheat.hpp
    Cheat.cpp
    Config.hpp
    SDK/Offsets.hpp
    SDK/Math.hpp
    SDK/UE.hpp
    Features/ESP.hpp
    Features/ESP.cpp
    Features/Aimbot.hpp
    Features/Aimbot.cpp
    Features/BulletTP.hpp
    Features/BulletTP.cpp
    Render/Hook.hpp
    Render/Hook.cpp
    Render/Draw.hpp
    Render/Draw.cpp
    Menu/Menu.hpp
    Menu/Menu.cpp)

target_include_directories(RetracCheat PRIVATE
    "${CMAKE_SOURCE_DIR}/src/common"
    "${CMAKE_SOURCE_DIR}/src/cheat")

target_link_libraries(RetracCheat PRIVATE common imgui_lib d3d11 dxgi)

set_target_properties(RetracCheat PROPERTIES
    OUTPUT_NAME "RetracOGFN"
    PREFIX "")
