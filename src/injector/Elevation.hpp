#pragma once

#include <windows.h>

namespace elevation {

bool is_elevated();
bool is_process_elevated(DWORD pid);
bool relaunch_elevated(const wchar_t* extraArgs);

} // namespace elevation
