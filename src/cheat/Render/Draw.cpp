#include "Render/Draw.hpp"

namespace draw {

ImU32 to_color(const ImVec4& color) {
    return ImGui::ColorConvertFloat4ToU32(color);
}

ImU32 to_color_alpha(const ImVec4& color, float alpha) {
    ImVec4 copy = color;
    copy.w = alpha;
    return ImGui::ColorConvertFloat4ToU32(copy);
}

void line(ImDrawList* list, float x1, float y1, float x2, float y2, ImU32 color, float thickness) {
    list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), color, thickness);
}

void rect(ImDrawList* list, float x, float y, float width, float height, ImU32 color, float thickness, float rounding) {
    list->AddRect(ImVec2(x, y), ImVec2(x + width, y + height), color, rounding, 0, thickness);
}

void filled_rect(ImDrawList* list, float x, float y, float width, float height, ImU32 color, float rounding) {
    list->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + height), color, rounding);
}

void circle(ImDrawList* list, float x, float y, float radius, ImU32 color, float thickness, int segments) {
    list->AddCircle(ImVec2(x, y), radius, color, segments, thickness);
}

ImVec2 measure(const char* text) {
    return ImGui::CalcTextSize(text);
}

void text(ImDrawList* list, float x, float y, ImU32 color, const char* text, bool centered, bool outlined) {
    const ImVec2 size = ImGui::CalcTextSize(text);
    const ImVec2 position(centered ? x - size.x * 0.5f : x, y);

    if (outlined) {
        const ImU32 shadow = IM_COL32(0, 0, 0, 220);
        list->AddText(ImVec2(position.x + 1.0f, position.y + 1.0f), shadow, text);
        list->AddText(ImVec2(position.x - 1.0f, position.y + 1.0f), shadow, text);
        list->AddText(ImVec2(position.x + 1.0f, position.y - 1.0f), shadow, text);
        list->AddText(ImVec2(position.x - 1.0f, position.y - 1.0f), shadow, text);
    }

    list->AddText(position, color, text);
}

} // namespace draw
