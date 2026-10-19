#pragma once

#include <imgui.h>

namespace draw {

ImU32 rgba(int r, int g, int b, int a);
ImU32 rgba(const float color[4]);

void line(float x1, float y1, float x2, float y2, ImU32 color, float thickness = 1.0f);
void box(float x, float y, float width, float height, ImU32 color, float thickness = 1.0f);
void filled_box(float x, float y, float width, float height, ImU32 color);
void text(float x, float y, const char* value, ImU32 color, bool centered = false);
void outlined_text(float x, float y, const char* value, ImU32 color, bool centered = false);

} // namespace draw
