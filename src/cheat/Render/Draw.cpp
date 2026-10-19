#include "Render/Draw.hpp"

namespace draw {

namespace {

ImDrawList* list() {
    return ImGui::GetBackgroundDrawList();
}

float center_offset(const char* value, bool centered, float& x) {
    if (!centered) {
        return x;
    }
    x -= ImGui::CalcTextSize(value).x / 2.0f;
    return x;
}

} // namespace

ImU32 rgba(int r, int g, int b, int a) {
    return IM_COL32(r, g, b, a);
}

ImU32 rgba(const float color[4]) {
    return IM_COL32(static_cast<int>(color[0] * 255.0f), static_cast<int>(color[1] * 255.0f),
                    static_cast<int>(color[2] * 255.0f), static_cast<int>(color[3] * 255.0f));
}

void line(float x1, float y1, float x2, float y2, ImU32 color, float thickness) {
    list()->AddLine({x1, y1}, {x2, y2}, color, thickness);
}

void box(float x, float y, float width, float height, ImU32 color, float thickness) {
    list()->AddRect({x, y}, {x + width, y + height}, color, 0.0f, 0, thickness);
}

void filled_box(float x, float y, float width, float height, ImU32 color) {
    list()->AddRectFilled({x, y}, {x + width, y + height}, color);
}

void text(float x, float y, const char* value, ImU32 color, bool centered) {
    list()->AddText({center_offset(value, centered, x), y}, color, value);
}

void outlined_text(float x, float y, const char* value, ImU32 color, bool centered) {
    center_offset(value, centered, x);

    const ImU32 outline = IM_COL32(0, 0, 0, 220);
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx || dy) {
                list()->AddText({x + static_cast<float>(dx), y + static_cast<float>(dy)}, outline,
                                value);
            }
        }
    }
    list()->AddText({x, y}, color, value);
}

} // namespace draw
