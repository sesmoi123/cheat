set(CHEAT_SOURCES
    dllmain.cpp
    Core.hpp Core.cpp
    Config.hpp Config.cpp
    SDK/Offsets.hpp SDK/Offsets.cpp
    SDK/Math.hpp
    SDK/SDK.hpp SDK/SDK.cpp
    Render/Render.hpp Render/Render.cpp
    Render/Draw.hpp Render/Draw.cpp
    Features/ESP.hpp Features/ESP.cpp
    Features/Aimbot.hpp Features/Aimbot.cpp
    Features/BulletTP.hpp Features/BulletTP.cpp
    Menu/Menu.hpp Menu/Menu.cpp)

set(IMGUI_SOURCES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_win32.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_dx11.cpp")

add_library(RetracCheat SHARED ${CHEAT_SOURCES} ${IMGUI_SOURCES})

target_include_directories(RetracCheat PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends")

target_link_libraries(RetracCheat PRIVATE common d3d11 dxgi dxguid)

set_target_properties(RetracCheat PROPERTIES PREFIX "" OUTPUT_NAME "RetracCheat")
