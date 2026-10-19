#include "Menu/Menu.hpp"

#include "Config.hpp"
#include "Input/Pad.hpp"
#include "Render/Render.hpp"
#include "SDK/SDK.hpp"

#include <imgui.h>

#include <cstdio>

namespace menu {

namespace {

int* g_capturingKey = nullptr;
uint32_t* g_capturingPad = nullptr;

void key_button(const char* label, int& key) {
    char text[96];
    std::snprintf(text, sizeof(text), "%s : %s##%s", label,
                  g_capturingKey == &key ? "appuie..." : "definir", label);

    if (ImGui::Button(text, ImVec2(180, 0))) {
        g_capturingKey = g_capturingKey == &key ? nullptr : &key;
        g_capturingPad = nullptr;
    }

    if (g_capturingKey != &key) {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        g_capturingKey = nullptr;
        return;
    }

    for (int candidate = 1; candidate < 256; ++candidate) {
        // La touche menu est ignoree : sinon elle s'auto-assigne.
        if (candidate == config::g.menu_key) {
            continue;
        }
        if (GetAsyncKeyState(candidate) & 0x8000) {
            key = candidate;
            g_capturingKey = nullptr;
            config::save();
            break;
        }
    }
}

void pad_combo_button(const char* label, uint32_t& mask) {
    char text[96];
    std::snprintf(text, sizeof(text), "%s : %s##pad%s", label,
                  g_capturingPad == &mask ? "appuie..." : "definir", label);

    if (ImGui::Button(text, ImVec2(180, 0))) {
        g_capturingPad = g_capturingPad == &mask ? nullptr : &mask;
        g_capturingKey = nullptr;
    }

    if (g_capturingPad != &mask) {
        return;
    }

    const uint32_t held = pad::held();
    if (pad::pressed() && held) {
        mask = held;
        g_capturingPad = nullptr;
        config::save();
    }
}

} // namespace

void apply_style() {
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.WindowPadding = ImVec2(12, 12);
    style.ItemSpacing = ImVec2(8, 7);

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.08f, 0.95f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.06f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.09f, 0.26f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.72f, 0.42f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.72f, 0.42f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.82f, 0.55f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.22f, 0.12f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.32f, 0.18f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.42f, 0.24f, 0.62f, 1.00f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.14f, 0.09f, 0.22f, 1.00f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.32f, 0.18f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.24f, 0.13f, 0.38f, 1.00f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.30f, 0.20f, 0.45f, 0.60f);
}

void load_fonts() {
    ImGuiIO& io = ImGui::GetIO();

    // Police systeme si presente, sinon police par defaut.
    if (GetFileAttributesW(L"C:\\Windows\\Fonts\\segoeui.ttf") != INVALID_FILE_ATTRIBUTES) {
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 16.0f);
    } else {
        io.Fonts->AddFontDefault();
    }
}

void draw() {
    if (!render::menu_open()) {
        if (config::g.watermark) {
            ImDrawList* list = ImGui::GetBackgroundDrawList();
            list->AddText({12, 12}, IM_COL32(200, 160, 255, 230), "Retrac OGFN");
            if (config::g.pad_enabled) {
                list->AddText({12, 32}, IM_COL32(150, 150, 160, 200),
                              pad::connected() ? "manette connectee" : "manette absente");
            }
        }
        return;
    }

    ImGui::SetNextWindowSize({500, 560}, ImGuiCond_FirstUseEver);
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
            key_button("Touche clavier", config::g.aim_key);
            if (config::g.pad_enabled) {
                pad_combo_button("Bouton manette", config::g.aim_pad_button);
            }

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
            key_button("Touche clavier", config::g.bullet_key);
            if (config::g.pad_enabled) {
                pad_combo_button("Bouton manette", config::g.bullet_pad_button);
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Manette")) {
            ImGui::Checkbox("Activer la manette", &config::g.pad_enabled);
            ImGui::SeparatorText("Rebind");
            pad_combo_button("Ouvrir / fermer le menu", config::g.menu_pad_combo);
            pad_combo_button("Aimbot", config::g.aim_pad_button);
            pad_combo_button("Bullet TP", config::g.bullet_pad_button);
            ImGui::SeparatorText("Etat");
            ImGui::Text("Manette : %s", pad::connected() ? "connectee" : "absente");
            ImGui::TextWrapped(
                "Le jeu lit la manette en meme temps que le cheat. Choisis un combo rarement "
                "utilise en jeu (LB+RB par defaut) pour ouvrir le menu.");
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

    ImGui::Separator();
    ImGui::Text("Joueurs : %d", static_cast<int>(sdk::players().size()));
    ImGui::SameLine();
    ImGui::Text(" | Manette : %s", pad::connected() ? "OK" : "-");

    ImGui::End();
}

} // namespace menu
