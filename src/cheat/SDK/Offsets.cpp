#include "SDK/Offsets.hpp"

#include "Core.hpp"
#include "Json.hpp"
#include "Logger.hpp"
#include "Paths.hpp"

namespace offsets {

Set g;

namespace {

struct Entry {
    const char* name;
    uintptr_t Set::* field;
};

const Entry kEntries[] = {
    {"GWorld", &Set::GWorld},
    {"World_OwningGameInstance", &Set::World_OwningGameInstance},
    {"World_Levels", &Set::World_Levels},
    {"Level_Actors", &Set::Level_Actors},
    {"GameInstance_LocalPlayers", &Set::GameInstance_LocalPlayers},
    {"LocalPlayer_PlayerController", &Set::LocalPlayer_PlayerController},
    {"PlayerController_AcknowledgedPawn", &Set::PlayerController_AcknowledgedPawn},
    {"PlayerController_ControlRotation", &Set::PlayerController_ControlRotation},
    {"PlayerController_PlayerCameraManager", &Set::PlayerController_PlayerCameraManager},
    {"CameraManager_CameraCache", &Set::CameraManager_CameraCache},
    {"CameraCache_POV", &Set::CameraCache_POV},
    {"Actor_RootComponent", &Set::Actor_RootComponent},
    {"SceneComponent_RelativeLocation", &Set::SceneComponent_RelativeLocation},
    {"Pawn_PlayerState", &Set::Pawn_PlayerState},
    {"Pawn_Mesh", &Set::Pawn_Mesh},
    {"Pawn_CurrentWeapon", &Set::Pawn_CurrentWeapon},
    {"Pawn_Health", &Set::Pawn_Health},
    {"Mesh_BoneArray", &Set::Mesh_BoneArray},
    {"Mesh_ComponentToWorld", &Set::Mesh_ComponentToWorld},
    {"PlayerState_PlayerName", &Set::PlayerState_PlayerName},
    {"PlayerState_TeamIndex", &Set::PlayerState_TeamIndex},
    {"PlayerState_IsABot", &Set::PlayerState_IsABot},
    {"PlayerState_IsDBNO", &Set::PlayerState_IsDBNO},
    {"Weapon_ProjectileSpawn", &Set::Weapon_ProjectileSpawn},
};

} // namespace

std::string path() {
    return paths::to_narrow(core::dll_directory() + L"\\offsets.json");
}

void load() {
    json::Value root;
    if (!json::read_file(path(), root)) {
        logger::warn("offsets.json absent : valeurs par defaut");
        return;
    }

    int applied = 0;
    for (const Entry& entry : kEntries) {
        if (root.has(entry.name)) {
            g.*(entry.field) = static_cast<uintptr_t>(root.number(entry.name, 0.0));
            ++applied;
        }
    }

    logger::info("offsets.json : %d valeurs surchargees", applied);
}

void write_example() {
    if (paths::file_exists(paths::to_wide(path()))) {
        return;
    }

    json::Value root;
    for (const Entry& entry : kEntries) {
        root.set_number(entry.name, static_cast<double>(g.*(entry.field)));
    }
    json::write_file(path(), root);
}

} // namespace offsets
