#include "Menu/Menu.hpp"

#include "Config.hpp"
#include "Render/Render.hpp"
#include "SDK/SDK.hpp"

#include <imgui.h>

#include <cstdio>

namespace menu {

namespace {

void key_button(const char* label, int& key) {
    static int* capturing = nullptr;

    char text[96];
    std::snprintf(text, sizeof(text), "%s : %s##%s", label,
                  capturing == &key ? "appuie..." : "definir", label);

    if (ImGui::Button(text, ImVec2(180, 0))) {
        capturing = capturing == &key ? nullptr : &key;
    }

    if (capturing == &key) {
        for (int candidate = 1; candidate < 256; ++candidate) {
            if (GetAsyncKeyState(candidate) & 0x8000) {
                key = candidate;
                capturing = nullptr;
                config::save();
                break;
            }
        }
    }
}

} // namespace

void draw() {
    if (!render::menu_open()) {
        if (config::g.watermark) {
            ImGui::GetBackgroundDrawList()->AddText({12, 12}, IM_COL32(200, 160, 255, 230),
                                                    "Retrac OGFN");
        }
        return;
    }

    ImGui::SetNextWindowSize({460, 520}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Retrac OGFN", &config::g.menu_open);

    if (ImGui::BeginTabBar("onglets")) {
        if (ImGui::BeginTabItem("ESP")) {
            ImGui::Checkbox("Activer", &config::g.esp_enabled);
            ImGui::Checkbox("Boite", &config::g.esp_box);
            ImGui::Checkbox("Squelette", &config::g.esp_skeleton);
            ImGui::Checkbox("Snapline", &config::g.esp_snapline);
            ImGui::Checkbox("Pseudo", &config::g.esp_name);
            ImGui::Checkbox("Distance", &config::g.esp_distance);
            ImGui::Checkbox("Barre de vie", &config::g.esp_health);
            ImGui::Checkbox("Filtrer les coequipiers", &config::g.esp_team_filter);
            ImGui::SliderFloat("Distance max", &config::g.esp_max_distance, 25.0f, 800.0f, "%.0f m");
            ImGui::ColorEdit4("Couleur", config::g.esp_color);
            ImGui::ColorEdit4("Couleur equipe", config::g.esp_team_color);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Aimbot")) {
            ImGui::Checkbox("Activer", &config::g.aim_enabled);
            ImGui::Checkbox("Maintenir la touche", &config::g.aim_hold);
            key_button("Touche", config::g.aim_key);

            const char* bones[] = {"Tete", "Poitrine", "Bassin"};
            ImGui::Combo("Os", &config::g.aim_bone, bones, 3);

            ImGui::SliderFloat("FOV", &config::g.aim_fov, 10.0f, 600.0f, "%.0f px");
            ImGui::SliderFloat("Smooth", &config::g.aim_smooth, 0.0f, 20.0f, "%.1f");
            ImGui::Checkbox("Mode memoire", &config::g.aim_memory);
            ImGui::Checkbox("Verif visibilite", &config::g.aim_visible_only);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Bullet TP")) {
            ImGui::Checkbox("Activer", &config::g.bullet_tp);
            ImGui::Checkbox("Toujours actif", &config::g.bullet_always);
            key_button("Touche", config::g.bullet_key);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Divers")) {
            key_button("Touche menu", config::g.menu_key);
            ImGui::Checkbox("Watermark", &config::g.watermark);
            ImGui::SliderFloat("Hauteur joueur", &config::g.esp_head_offset, 40.0f, 140.0f, "%.0f");
            if (ImGui::Button("Sauvegarder")) {
                config::save();
            }
            ImGui::SameLine();
            if (ImGui::Button("Recharger")) {
                config::load();
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::Text("Joueurs : %d", static_cast<int>(sdk::players().size()));
    ImGui::End();
}

} // namespace menu
