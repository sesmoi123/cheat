#pragma once

#include <cstdint>
#include <string>

namespace offsets {

// Toutes les cles sont surchargeables via offsets.json, place a cote de la DLL
// (donc dans %LOCALAPPDATA%\RetracOGFN). Les valeurs par defaut correspondent
// au patch de reference et doivent etre ajustees apres une grosse mise a jour.
struct Set {
    uintptr_t GWorld = 0x2B33BE8;

    uintptr_t World_OwningGameInstance = 0x238;
    uintptr_t World_Levels = 0x170;

    uintptr_t Level_Actors = 0xA0;

    uintptr_t GameInstance_LocalPlayers = 0x38;
    uintptr_t LocalPlayer_PlayerController = 0x30;

    uintptr_t PlayerController_AcknowledgedPawn = 0x330;
    uintptr_t PlayerController_ControlRotation = 0x288;
    uintptr_t PlayerController_PlayerCameraManager = 0x358;

    uintptr_t CameraManager_CameraCache = 0x1A00;
    uintptr_t CameraCache_POV = 0x10;

    uintptr_t Actor_RootComponent = 0x130;
    uintptr_t SceneComponent_RelativeLocation = 0x128;

    uintptr_t Pawn_PlayerState = 0x290;
    uintptr_t Pawn_Mesh = 0x2F0;
    uintptr_t Pawn_CurrentWeapon = 0x9A0;
    uintptr_t Pawn_Health = 0x3D0;

    uintptr_t Mesh_BoneArray = 0x5A8;
    uintptr_t Mesh_ComponentToWorld = 0x1E0;

    uintptr_t PlayerState_PlayerName = 0x308;
    uintptr_t PlayerState_TeamIndex = 0x3C0;
    uintptr_t PlayerState_IsABot = 0x3C8;
    uintptr_t PlayerState_IsDBNO = 0x4A0;

    uintptr_t Weapon_ProjectileSpawn = 0x9C0;
};

extern Set g;

void load();
void write_example();
std::string path();

} // namespace offsets
