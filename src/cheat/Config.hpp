#pragma once

#include <windows.h>

#include <cstdint>
#include <string>

namespace config {

struct Settings {
    bool menu_open = true;
    int menu_key = VK_INSERT;

    bool watermark = true;

    bool pad_enabled = true;
    uint32_t menu_pad_combo = 0x0300;    // LB | RB
    uint32_t aim_pad_button = 0x10000;   // LT
    uint32_t bullet_pad_button = 0x8000; // Y

    bool esp_enabled = true;
    bool esp_box = true;
    bool esp_skeleton = true;
    bool esp_snapline = false;
    bool esp_name = true;
    bool esp_distance = true;
    bool esp_health = true;
    bool esp_team_filter = true;
    float esp_max_distance = 300.0f;
    float esp_head_offset = 88.0f;
    float esp_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float esp_team_color[4] = {0.4f, 0.8f, 1.0f, 1.0f};

    bool aim_enabled = false;
    bool aim_hold = true;
    int aim_key = VK_RBUTTON;
    int aim_bone = 0; // 0 tete, 1 poitrine, 2 bassin
    float aim_fov = 120.0f;
    float aim_smooth = 4.0f;
    bool aim_memory = true;
    bool aim_visible_only = false;

    bool bullet_tp = false;
    bool bullet_always = false;
    int bullet_key = VK_XBUTTON2;

    float bone_head = 0.88f;
    float bone_chest = 0.62f;
    float bone_pelvis = 0.45f;
};

extern Settings g;

void load();
void save();

std::string path();

} // namespace config
