#pragma once

#include <cstdlib>
#include <fstream>
#include <string>

#include <windows.h>

#include <imgui.h>
#include <nlohmann/json.hpp>

namespace config {

enum class AimMode {
    Memory = 0,
    Mouse = 1,
};

struct Config {
    // --- ESP ---------------------------------------------------------------
    bool esp_enabled = true;
    bool esp_box = true;
    bool esp_skeleton = false;
    bool esp_snapline = true;
    bool esp_name = true;
    bool esp_distance = true;
    bool esp_weapon = true;
    bool esp_health = true;
    bool esp_knocked = true;
    bool esp_local = false;
    bool esp_teammates = false;
    bool esp_visible_only = false;
    float esp_max_distance = 600.0f;
    float esp_thickness = 1.5f;

    ImVec4 color_box{0.60f, 0.40f, 1.00f, 1.00f};
    ImVec4 color_skeleton{1.00f, 1.00f, 1.00f, 0.90f};
    ImVec4 color_snapline{0.60f, 0.40f, 1.00f, 0.70f};
    ImVec4 color_name{1.00f, 1.00f, 1.00f, 1.00f};
    ImVec4 color_distance{0.80f, 0.80f, 0.80f, 1.00f};
    ImVec4 color_teammate{0.30f, 1.00f, 0.40f, 1.00f};
    ImVec4 color_visible{1.00f, 0.85f, 0.20f, 1.00f};

    // --- Aimbot ------------------------------------------------------------
    bool aim_enabled = true;
    int aim_mode = static_cast<int>(AimMode::Memory);
    int aim_key = VK_RBUTTON;
    int aim_bone = 0;
    float aim_smooth = 6.0f;
    float aim_fov = 120.0f;
    float aim_max_distance = 350.0f;
    float aim_sensitivity = 1.0f;
    bool aim_visible_only = true;
    bool aim_draw_fov = true;
    bool aim_teammates = false;

    // --- Bullet TP ---------------------------------------------------------
    bool bullet_tp = false;
    bool bullet_tp_always = false;
    int bullet_tp_key = VK_XBUTTON2;
    float bullet_tp_fov = 12.0f;
    float bullet_tp_height = 0.0f;

    // --- Menu --------------------------------------------------------------
    bool watermark = true;
    int menu_key = VK_INSERT;
};

inline Config& get() {
    static Config value;
    return value;
}

inline std::string directory() {
    const char* localAppData = std::getenv("LOCALAPPDATA");
    const std::string root = localAppData ? std::string(localAppData) : std::string(".");
    return root + "\\RetracOGFN";
}

inline std::string file_path() {
    return directory() + "\\config.json";
}

inline void ensure_directory() {
    CreateDirectoryA(directory().c_str(), nullptr);
}

inline nlohmann::json to_json(const Config& config) {
    nlohmann::json json;

    json["esp_enabled"] = config.esp_enabled;
    json["esp_box"] = config.esp_box;
    json["esp_skeleton"] = config.esp_skeleton;
    json["esp_snapline"] = config.esp_snapline;
    json["esp_name"] = config.esp_name;
    json["esp_distance"] = config.esp_distance;
    json["esp_weapon"] = config.esp_weapon;
    json["esp_health"] = config.esp_health;
    json["esp_knocked"] = config.esp_knocked;
    json["esp_local"] = config.esp_local;
    json["esp_teammates"] = config.esp_teammates;
    json["esp_visible_only"] = config.esp_visible_only;
    json["esp_max_distance"] = config.esp_max_distance;
    json["esp_thickness"] = config.esp_thickness;

    json["color_box"] = {config.color_box.x, config.color_box.y, config.color_box.z, config.color_box.w};
    json["color_skeleton"] = {config.color_skeleton.x, config.color_skeleton.y, config.color_skeleton.z, config.color_skeleton.w};
    json["color_snapline"] = {config.color_snapline.x, config.color_snapline.y, config.color_snapline.z, config.color_snapline.w};
    json["color_name"] = {config.color_name.x, config.color_name.y, config.color_name.z, config.color_name.w};
    json["color_distance"] = {config.color_distance.x, config.color_distance.y, config.color_distance.z, config.color_distance.w};
    json["color_teammate"] = {config.color_teammate.x, config.color_teammate.y, config.color_teammate.z, config.color_teammate.w};
    json["color_visible"] = {config.color_visible.x, config.color_visible.y, config.color_visible.z, config.color_visible.w};

    json["aim_enabled"] = config.aim_enabled;
    json["aim_mode"] = config.aim_mode;
    json["aim_key"] = config.aim_key;
    json["aim_bone"] = config.aim_bone;
    json["aim_smooth"] = config.aim_smooth;
    json["aim_fov"] = config.aim_fov;
    json["aim_max_distance"] = config.aim_max_distance;
    json["aim_sensitivity"] = config.aim_sensitivity;
    json["aim_visible_only"] = config.aim_visible_only;
    json["aim_draw_fov"] = config.aim_draw_fov;
    json["aim_teammates"] = config.aim_teammates;

    json["bullet_tp"] = config.bullet_tp;
    json["bullet_tp_always"] = config.bullet_tp_always;
    json["bullet_tp_key"] = config.bullet_tp_key;
    json["bullet_tp_fov"] = config.bullet_tp_fov;
    json["bullet_tp_height"] = config.bullet_tp_height;

    json["watermark"] = config.watermark;
    json["menu_key"] = config.menu_key;

    return json;
}

inline void save() {
    ensure_directory();

    std::ofstream file(file_path());
    if (!file.is_open()) {
        return;
    }
    file << to_json(get()).dump(4) << std::endl;
}

inline void load() {
    std::ifstream file(file_path());
    if (!file.is_open()) {
        return;
    }

    nlohmann::json json;
    try {
        file >> json;
    } catch (...) {
        return;
    }

    Config& config = get();

    auto boolean = [&json](const char* key, bool& field) {
        if (json.contains(key) && json[key].is_boolean()) {
            field = json[key].get<bool>();
        }
    };
    auto number = [&json](const char* key, int& field) {
        if (json.contains(key) && json[key].is_number_integer()) {
            field = json[key].get<int>();
        }
    };
    auto floating = [&json](const char* key, float& field) {
        if (json.contains(key) && json[key].is_number()) {
            field = json[key].get<float>();
        }
    };
    auto color = [&json](const char* key, ImVec4& field) {
        if (!json.contains(key) || !json[key].is_array() || json[key].size() != 4) {
            return;
        }
        field.x = json[key][0].get<float>();
        field.y = json[key][1].get<float>();
        field.z = json[key][2].get<float>();
        field.w = json[key][3].get<float>();
    };

    boolean("esp_enabled", config.esp_enabled);
    boolean("esp_box", config.esp_box);
    boolean("esp_skeleton", config.esp_skeleton);
    boolean("esp_snapline", config.esp_snapline);
    boolean("esp_name", config.esp_name);
    boolean("esp_distance", config.esp_distance);
    boolean("esp_weapon", config.esp_weapon);
    boolean("esp_health", config.esp_health);
    boolean("esp_knocked", config.esp_knocked);
    boolean("esp_local", config.esp_local);
    boolean("esp_teammates", config.esp_teammates);
    boolean("esp_visible_only", config.esp_visible_only);
    floating("esp_max_distance", config.esp_max_distance);
    floating("esp_thickness", config.esp_thickness);

    color("color_box", config.color_box);
    color("color_skeleton", config.color_skeleton);
    color("color_snapline", config.color_snapline);
    color("color_name", config.color_name);
    color("color_distance", config.color_distance);
    color("color_teammate", config.color_teammate);
    color("color_visible", config.color_visible);

    boolean("aim_enabled", config.aim_enabled);
    number("aim_mode", config.aim_mode);
    number("aim_key", config.aim_key);
    number("aim_bone", config.aim_bone);
    floating("aim_smooth", config.aim_smooth);
    floating("aim_fov", config.aim_fov);
    floating("aim_max_distance", config.aim_max_distance);
    floating("aim_sensitivity", config.aim_sensitivity);
    boolean("aim_visible_only", config.aim_visible_only);
    boolean("aim_draw_fov", config.aim_draw_fov);
    boolean("aim_teammates", config.aim_teammates);

    boolean("bullet_tp", config.bullet_tp);
    boolean("bullet_tp_always", config.bullet_tp_always);
    number("bullet_tp_key", config.bullet_tp_key);
    floating("bullet_tp_fov", config.bullet_tp_fov);
    floating("bullet_tp_height", config.bullet_tp_height);

    boolean("watermark", config.watermark);
    number("menu_key", config.menu_key);
}

} // namespace config

inline config::Config& cfg() {
    return config::get();
}
