#pragma once

#include <windows.h>

#include <d3d11.h>
#include <dxgi.h>

namespace hook {

bool install();
void uninstall();

bool menu_visible();
void set_menu_visible(bool value);
void toggle_menu();

HWND game_window();
ID3D11Device* device();
ID3D11DeviceContext* context();

} // namespace hook
