#pragma once

#include <imgui.h>

namespace draw {

ImU32 to_color(const ImVec4& color);
ImU32 to_color_alpha(const ImVec4& color, float alpha);

void line(ImDrawList* list, float x1, float y1, float x2, float y2, ImU32 color, float thickness);
void rect(ImDrawList* list, float x, float y, float width, float height, ImU32 color, float thickness, float rounding);
void filled_rect(ImDrawList* list, float x, float y, float width, float height, ImU32 color, float rounding);
void circle(ImDrawList* list, float x, float y, float radius, ImU32 color, float thickness, int segments);
void text(ImDrawList* list, float x, float y, ImU32 color, const char* text, bool centered, bool outlined);
ImVec2 measure(const char* text);

} // namespace draw
