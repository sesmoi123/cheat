#include "Menu/Menu.hpp"

#include <cstdio>

#include <imgui.h>

#include "Config.hpp"
#include "Render/Hook.hpp"
#include "SDK/UE.hpp"

namespace menu {

namespace {

ImFont* titleFont = nullptr;
ImFont* bodyFont = nullptr;

void push_font(ImFont* font) {
    if (!font) {
        return;
    }
#if defined(IMGUI_VERSION_NUM) && IMGUI_VERSION_NUM >= 19200
    ImGui::PushFont(font, 0.0f);
#else
    ImGui::PushFont(font);
#endif
}

bool key_combo(const char* label, int& key) {
    int current = 0;
    switch (key) {
    case VK_RBUTTON: current = 0; break;
    case VK_LBUTTON: current = 1; break;
    case VK_XBUTTON1: current = 2; break;
    case VK_XBUTTON2: current = 3; break;
    case VK_LMENU: current = 4; break;
    case VK_LSHIFT: current = 5; break;
    case VK_LCONTROL: current = 6; break;
    case VK_CAPITAL: current = 7; break;
    case 'C': current = 8; break;
    case 'V': current = 9; break;
    case 'X': current = 10; break;
    default: current = 0; break;
    }

    const char* items[] = {"Clic droit", "Clic gauche", "Souris 4", "Souris 5", "Alt gauche",
                           "Maj gauche", "Ctrl gauche", "Verr. Maj", "C", "V", "X"};

    if (!ImGui::Combo(label, &current, items, IM_ARRAYSIZE(items))) {
        return false;
    }

    const int keys[] = {VK_RBUTTON, VK_LBUTTON, VK_XBUTTON1, VK_XBUTTON2, VK_LMENU,
                        VK_LSHIFT, VK_LCONTROL, VK_CAPITAL, 'C', 'V', 'X'};
    key = keys[current];
    return true;
}

void esp_tab() {
    config::Config& config = cfg();

    ImGui::Checkbox("Activer l'ESP", &config.esp_enabled);
    ImGui::SeparatorText("Elements");

    ImGui::Columns(2, nullptr, false);
    ImGui::Checkbox("Boite", &config.esp_box);
    ImGui::Checkbox("Skeleton", &config.esp_skeleton);
    ImGui::Checkbox("Snapline", &config.esp_snapline);
    ImGui::Checkbox("Pseudo", &config.esp_name);
    ImGui::NextColumn();
    ImGui::Checkbox("Distance", &config.esp_distance);
    ImGui::Checkbox("Arme", &config.esp_weapon);
    ImGui::Checkbox("Barre de vie", &config.esp_health);
    ImGui::Checkbox("Etiquette (DBNO)", &config.esp_knocked);
    ImGui::Columns(1);

    ImGui::SeparatorText("Filtres");
    ImGui::Checkbox("Afficher mon personnage", &config.esp_local);
    ImGui::Checkbox("Afficher les coequipiers", &config.esp_teammates);
    ImGui::Checkbox("Uniquement visibles", &config.esp_visible_only);

    ImGui::SliderFloat("Distance max (m)", &config.esp_max_distance, 20.0f, 1500.0f, "%.0f");
    ImGui::SliderFloat("Epaisseur", &config.esp_thickness, 0.5f, 4.0f, "%.1f");

    ImGui::SeparatorText("Couleurs");
    ImGui::ColorEdit4("Boite##c", &config.color_box.x, ImGuiColorEditFlags_NoInputs);
    ImGui::SameLine();
    ImGui::ColorEdit4("Skeleton##c", &config.color_skeleton.x, ImGuiColorEditFlags_NoInputs);
    ImGui::SameLine();
    ImGui::ColorEdit4("Snapline##c", &config.color_snapline.x, ImGuiColorEditFlags_NoInputs);

    ImGui::ColorEdit4("Pseudo##c", &config.color_name.x, ImGuiColorEditFlags_NoInputs);
    ImGui::SameLine();
    ImGui::ColorEdit4("Distance##c", &config.color_distance.x, ImGuiColorEditFlags_NoInputs);
    ImGui::SameLine();
    ImGui::ColorEdit4("Coequipier##c", &config.color_teammate.x, ImGuiColorEditFlags_NoInputs);
}

void aim_tab() {
    config::Config& config = cfg();

    ImGui::Checkbox("Activer l'aimbot", &config.aim_enabled);
    ImGui::SeparatorText("Comportement");

    int mode = config.aim_mode;
    ImGui::RadioButton("Memoire (ControlRotation)", &mode, static_cast<int>(config::AimMode::Memory));
    ImGui::SameLine();
    ImGui::RadioButton("Souris", &mode, static_cast<int>(config::AimMode::Mouse));
    config.aim_mode = mode;

    key_combo("Touche", config.aim_key);

    const char* bones[] = {"Tete", "Poitrine", "Bassin"};
    ImGui::Combo("Os vise", &config.aim_bone, bones, IM_ARRAYSIZE(bones));

    ImGui::SliderFloat("Smooth", &config.aim_smooth, 1.0f, 20.0f, "%.1f");
    ImGui::SliderFloat("FOV", &config.aim_fov, 10.0f, 400.0f, "%.0f px");
    ImGui::SliderFloat("Distance max (m)", &config.aim_max_distance, 20.0f, 800.0f, "%.0f");

    if (config.aim_mode == static_cast<int>(config::AimMode::Mouse)) {
        ImGui::SliderFloat("Sensibilite in-game", &config.aim_sensitivity, 0.01f, 5.0f, "%.2f");
    }

    ImGui::SeparatorText("Options");
    ImGui::Checkbox("Uniquement visibles", &config.aim_visible_only);
    ImGui::Checkbox("Afficher le cercle FOV", &config.aim_draw_fov);
    ImGui::Checkbox("Viser les coequipiers", &config.aim_teammates);
}

void bullet_tab() {
    config::Config& config = cfg();

    ImGui::Checkbox("Activer le Bullet TP", &config.bullet_tp);
    ImGui::TextWrapped(
        "Le Bullet TP deplace le point de spawn des projectiles sur la cible : "
        "les balles partent de sa position au lieu de la tienne.");

    ImGui::SeparatorText("Reglages");
    ImGui::Checkbox("Toujours actif (sans touche)", &config.bullet_tp_always);
    key_combo("Touche", config.bullet_tp_key);
    ImGui::SliderFloat("FOV de selection", &config.bullet_tp_fov, 1.0f, 60.0f, "%.1f deg");
    ImGui::SliderFloat("Decalage vertical (cm)", &config.bullet_tp_height, -100.0f, 100.0f, "%.0f");
}

void misc_tab() {
    config::Config& config = cfg();

    ImGui::Checkbox("Watermark", &config.watermark);

    ImGui::SeparatorText("Offsets");
    ImGui::Text("Base : patch 42.40");
    ImGui::Text("GWorld : 0x%llX", static_cast<unsigned long long>(off::get().GWorld));
    ImGui::TextWrapped(
        "Cree un fichier offsets.json a cote de la DLL pour changer les offsets sans recompiler.");

    ImGui::SeparatorText("Config");
    if (ImGui::Button("Sauvegarder")) {
        config::save();
    }
    ImGui::SameLine();
    if (ImGui::Button("Recharger")) {
        config::load();
    }
}

} // namespace

void apply_style() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 9.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 5.0f;
    style.PopupRounding = 5.0f;
    style.ScrollbarRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.TabRounding = 5.0f;

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowPadding = ImVec2(14.0f, 14.0f);
    style.FramePadding = ImVec2(10.0f, 5.0f);
    style.ItemSpacing = ImVec2(9.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(7.0f, 6.0f);
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 12.0f;

    ImVec4* colors = style.Colors;
    const ImVec4 accent(0.60f, 0.40f, 1.00f, 1.00f);
    const ImVec4 accentDim(0.60f, 0.40f, 1.00f, 0.55f);

    colors[ImGuiCol_Text] = ImVec4(0.94f, 0.94f, 0.97f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.58f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 0.96f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.09f, 0.09f, 0.12f, 0.90f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.09f, 0.09f, 0.12f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.60f, 0.40f, 1.00f, 0.35f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.14f, 0.19f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.18f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.22f, 0.40f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.13f, 0.26f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.09f, 0.14f, 0.90f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.11f, 0.11f, 0.15f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = accentDim;
    colors[ImGuiCol_ScrollbarGrabHovered] = accent;
    colors[ImGuiCol_ScrollbarGrabActive] = accent;
    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = accentDim;
    colors[ImGuiCol_SliderGrabActive] = accent;
    colors[ImGuiCol_Button] = ImVec4(0.18f, 0.16f, 0.28f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.22f, 0.46f, 1.00f);
    colors[ImGuiCol_ButtonActive] = accent;
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.17f, 0.32f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.22f, 0.46f, 1.00f);
    colors[ImGuiCol_HeaderActive] = accent;
    colors[ImGuiCol_Separator] = ImVec4(0.60f, 0.40f, 1.00f, 0.30f);
    colors[ImGuiCol_SeparatorHovered] = accentDim;
    colors[ImGuiCol_SeparatorActive] = accent;
    colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.12f, 0.17f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.28f, 0.22f, 0.46f, 1.00f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.24f, 0.20f, 0.40f, 1.00f);
    colors[ImGuiCol_TabDimmed] = ImVec4(0.10f, 0.10f, 0.14f, 1.00f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.18f, 0.16f, 0.28f, 1.00f);
    colors[ImGuiCol_PlotLines] = accent;
    colors[ImGuiCol_PlotLinesHovered] = accent;
    colors[ImGuiCol_PlotHistogram] = accent;
    colors[ImGuiCol_PlotHistogramHovered] = accent;
    colors[ImGuiCol_TextSelectedBg] = accentDim;
    colors[ImGuiCol_NavCursor] = accent;
}

void load_fonts() {
    ImGuiIO& io = ImGui::GetIO();

    titleFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\seguisb.ttf", 21.0f);
    if (!titleFont) {
        titleFont = io.Fonts->AddFontDefault();
    }

    bodyFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 16.0f);
    if (!bodyFont) {
        bodyFont = io.Fonts->AddFontDefault();
    }
}

void render() {
    if (!hook::menu_visible()) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(560.0f, 520.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(120.0f, 120.0f), ImGuiCond_FirstUseEver);

    push_font(bodyFont);

    if (ImGui::Begin("Retrac OGFN", nullptr, ImGuiWindowFlags_NoCollapse)) {
        push_font(titleFont);
        ImGui::TextColored(ImVec4(0.60f, 0.40f, 1.00f, 1.00f), "Retrac OGFN");
        ImGui::PopFont();

        ImGui::SameLine();
        ImGui::TextDisabled("| Fortnite internal");

        ImGui::Separator();

        if (ImGui::BeginTabBar("tabs")) {
            if (ImGui::BeginTabItem("ESP")) {
                esp_tab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Aimbot")) {
                aim_tab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Bullet TP")) {
                bullet_tab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Divers")) {
                misc_tab();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::Separator();
        ImGui::TextDisabled("Inser : ouvrir / fermer le menu");
    }
    ImGui::End();

    if (bodyFont) {
        ImGui::PopFont();
    }
}

void draw_watermark() {
    if (!cfg().watermark) {
        return;
    }

    ImDrawList* list = ImGui::GetBackgroundDrawList();
    const ImVec2 display = ImGui::GetIO().DisplaySize;

    char text[128];
    snprintf(text, sizeof(text), "Retrac OGFN  |  %.0f fps", ImGui::GetIO().Framerate);

    const ImVec2 size = ImGui::CalcTextSize(text);
    const ImVec2 position(display.x - size.x - 20.0f, 16.0f);

    list->AddRectFilled(ImVec2(position.x - 8.0f, position.y - 5.0f),
                        ImVec2(position.x + size.x + 8.0f, position.y + size.y + 5.0f),
                        IM_COL32(15, 15, 20, 190), 5.0f);

    list->AddText(position, IM_COL32(153, 102, 255, 255), text);
}

bool visible() {
    return hook::menu_visible();
}

void toggle() {
    hook::toggle_menu();
}

} // namespace menu
