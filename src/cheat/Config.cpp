#include "Config.hpp"

#include "Json.hpp"
#include "Paths.hpp"

namespace config {

Settings g;

std::string path() {
    return paths::to_narrow(paths::data_directory() + L"\\config.json");
}

void load() {
    json::Value root;
    if (!json::read_file(path(), root)) {
        save();
        return;
    }

    g.menu_key = static_cast<int>(root.number("menu_key", g.menu_key));
    g.watermark = root.boolean("watermark", g.watermark);

    g.pad_enabled = root.boolean("pad_enabled", g.pad_enabled);
    g.menu_pad_combo = static_cast<uint32_t>(root.number("menu_pad_combo", g.menu_pad_combo));
    g.aim_pad_button = static_cast<uint32_t>(root.number("aim_pad_button", g.aim_pad_button));
    g.bullet_pad_button =
        static_cast<uint32_t>(root.number("bullet_pad_button", g.bullet_pad_button));

    g.esp_enabled = root.boolean("esp_enabled", g.esp_enabled);
    g.esp_box = root.boolean("esp_box", g.esp_box);
    g.esp_skeleton = root.boolean("esp_skeleton", g.esp_skeleton);
    g.esp_snapline = root.boolean("esp_snapline", g.esp_snapline);
    g.esp_name = root.boolean("esp_name", g.esp_name);
    g.esp_distance = root.boolean("esp_distance", g.esp_distance);
    g.esp_health = root.boolean("esp_health", g.esp_health);
    g.esp_team_filter = root.boolean("esp_team_filter", g.esp_team_filter);
    g.esp_max_distance = static_cast<float>(root.number("esp_max_distance", g.esp_max_distance));
    g.esp_head_offset = static_cast<float>(root.number("esp_head_offset", g.esp_head_offset));

    g.aim_enabled = root.boolean("aim_enabled", g.aim_enabled);
    g.aim_hold = root.boolean("aim_hold", g.aim_hold);
    g.aim_key = static_cast<int>(root.number("aim_key", g.aim_key));
    g.aim_bone = static_cast<int>(root.number("aim_bone", g.aim_bone));
    g.aim_fov = static_cast<float>(root.number("aim_fov", g.aim_fov));
    g.aim_smooth = static_cast<float>(root.number("aim_smooth", g.aim_smooth));
    g.aim_memory = root.boolean("aim_memory", g.aim_memory);
    g.aim_visible_only = root.boolean("aim_visible_only", g.aim_visible_only);

    g.bullet_tp = root.boolean("bullet_tp", g.bullet_tp);
    g.bullet_always = root.boolean("bullet_always", g.bullet_always);
    g.bullet_key = static_cast<int>(root.number("bullet_key", g.bullet_key));
}

void save() {
    json::Value root;

    root.set_number("menu_key", g.menu_key);
    root.set_boolean("watermark", g.watermark);

    root.set_boolean("pad_enabled", g.pad_enabled);
    root.set_number("menu_pad_combo", g.menu_pad_combo);
    root.set_number("aim_pad_button", g.aim_pad_button);
    root.set_number("bullet_pad_button", g.bullet_pad_button);

    root.set_boolean("esp_enabled", g.esp_enabled);
    root.set_boolean("esp_box", g.esp_box);
    root.set_boolean("esp_skeleton", g.esp_skeleton);
    root.set_boolean("esp_snapline", g.esp_snapline);
    root.set_boolean("esp_name", g.esp_name);
    root.set_boolean("esp_distance", g.esp_distance);
    root.set_boolean("esp_health", g.esp_health);
    root.set_boolean("esp_team_filter", g.esp_team_filter);
    root.set_number("esp_max_distance", g.esp_max_distance);
    root.set_number("esp_head_offset", g.esp_head_offset);

    root.set_boolean("aim_enabled", g.aim_enabled);
    root.set_boolean("aim_hold", g.aim_hold);
    root.set_number("aim_key", g.aim_key);
    root.set_number("aim_bone", g.aim_bone);
    root.set_number("aim_fov", g.aim_fov);
    root.set_number("aim_smooth", g.aim_smooth);
    root.set_boolean("aim_memory", g.aim_memory);
    root.set_boolean("aim_visible_only", g.aim_visible_only);

    root.set_boolean("bullet_tp", g.bullet_tp);
    root.set_boolean("bullet_always", g.bullet_always);
    root.set_number("bullet_key", g.bullet_key);

    json::write_file(path(), root);
}

} // namespace config
