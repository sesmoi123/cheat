#pragma once

#include <cstdint>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>

namespace off {

// ---------------------------------------------------------------------------
// Offsets Fortnite (base: patch 42.40).
//
// Surchargeables sans recompiler : place un fichier "offsets.json" a cote de
// la DLL avec les memes noms de champs que ci-dessous, par exemple :
//
//   { "GWorld": 45300712, "Character_Mesh": 752 }
//
// ---------------------------------------------------------------------------
struct Offsets {
    // --- Globals -----------------------------------------------------------
    uintptr_t GWorld = 0x1B156318;
    uintptr_t GNames = 0x0;
    uintptr_t GEngine = 0x0;

    // --- UWorld ------------------------------------------------------------
    uintptr_t World_OwningGameInstance = 0x238;
    uintptr_t World_PersistentLevel = 0x38;
    uintptr_t World_GameState = 0x1C0;

    // --- ULevel ------------------------------------------------------------
    uintptr_t Level_ActorCluster = 0xE0;
    uintptr_t ActorContainer_Actors = 0x28;
    uintptr_t Level_Actors = 0x98;

    // --- UGameInstance / ULocalPlayer --------------------------------------
    uintptr_t GameInstance_LocalPlayers = 0x38;
    uintptr_t LocalPlayer_PlayerController = 0x30;

    // --- APlayerController / AController -----------------------------------
    uintptr_t Controller_ControlRotation = 0x2E8;
    uintptr_t Controller_AcknowledgedPawn = 0x300;
    uintptr_t Controller_PlayerCameraManager = 0x318;

    // --- APlayerCameraManager (FCameraCacheEntry POV) ----------------------
    uintptr_t Camera_CameraCachePrivate = 0x1590;
    uintptr_t CameraCache_POV = 0x10;
    uintptr_t POV_Location = 0x00;
    uintptr_t POV_Rotation = 0x18;
    uintptr_t POV_FOV = 0x30;

    // --- AActor / APawn / ACharacter ---------------------------------------
    uintptr_t Actor_RootComponent = 0x1B0;
    uintptr_t Pawn_PlayerState = 0x290;
    uintptr_t Character_Mesh = 0x2F0;
    uintptr_t Pawn_CurrentWeapon = 0x9A0;
    uintptr_t Pawn_Health = 0x430;
    uintptr_t Pawn_bIsDBNO = 0x552;

    // --- USceneComponent ---------------------------------------------------
    uintptr_t Scene_RelativeLocation = 0x140;
    uintptr_t Scene_ComponentToWorld = 0x1A0;

    // --- USkeletalMeshComponent --------------------------------------------
    uintptr_t Mesh_BoneArray = 0x660;
    uintptr_t Mesh_BoneStride = 0x60;

    // --- APlayerState ------------------------------------------------------
    uintptr_t PlayerState_PlayerName = 0x308;
    uintptr_t PlayerState_TeamIndex = 0xE88;

    // --- AFortWeapon -------------------------------------------------------
    uintptr_t Weapon_FireStartLocation = 0x878;
    uintptr_t Weapon_LastFireLocation = 0x870;
};

inline Offsets& get() {
    static Offsets value;
    return value;
}

inline void load_from_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return;
    }

    nlohmann::json json;
    try {
        file >> json;
    } catch (...) {
        return;
    }

    Offsets& offsets = get();
    auto apply = [&json](const char* key, uintptr_t& field) {
        if (!json.contains(key)) {
            return;
        }
        const auto& node = json[key];
        if (node.is_number_unsigned()) {
            field = node.get<uintptr_t>();
        } else if (node.is_number_integer()) {
            field = static_cast<uintptr_t>(node.get<int64_t>());
        }
    };

    apply("GWorld", offsets.GWorld);
    apply("GNames", offsets.GNames);
    apply("GEngine", offsets.GEngine);

    apply("World_OwningGameInstance", offsets.World_OwningGameInstance);
    apply("World_PersistentLevel", offsets.World_PersistentLevel);
    apply("World_GameState", offsets.World_GameState);

    apply("Level_ActorCluster", offsets.Level_ActorCluster);
    apply("ActorContainer_Actors", offsets.ActorContainer_Actors);
    apply("Level_Actors", offsets.Level_Actors);

    apply("GameInstance_LocalPlayers", offsets.GameInstance_LocalPlayers);
    apply("LocalPlayer_PlayerController", offsets.LocalPlayer_PlayerController);

    apply("Controller_ControlRotation", offsets.Controller_ControlRotation);
    apply("Controller_AcknowledgedPawn", offsets.Controller_AcknowledgedPawn);
    apply("Controller_PlayerCameraManager", offsets.Controller_PlayerCameraManager);

    apply("Camera_CameraCachePrivate", offsets.Camera_CameraCachePrivate);
    apply("CameraCache_POV", offsets.CameraCache_POV);
    apply("POV_Location", offsets.POV_Location);
    apply("POV_Rotation", offsets.POV_Rotation);
    apply("POV_FOV", offsets.POV_FOV);

    apply("Actor_RootComponent", offsets.Actor_RootComponent);
    apply("Pawn_PlayerState", offsets.Pawn_PlayerState);
    apply("Character_Mesh", offsets.Character_Mesh);
    apply("Pawn_CurrentWeapon", offsets.Pawn_CurrentWeapon);
    apply("Pawn_Health", offsets.Pawn_Health);
    apply("Pawn_bIsDBNO", offsets.Pawn_bIsDBNO);

    apply("Scene_RelativeLocation", offsets.Scene_RelativeLocation);
    apply("Scene_ComponentToWorld", offsets.Scene_ComponentToWorld);

    apply("Mesh_BoneArray", offsets.Mesh_BoneArray);
    apply("Mesh_BoneStride", offsets.Mesh_BoneStride);

    apply("PlayerState_PlayerName", offsets.PlayerState_PlayerName);
    apply("PlayerState_TeamIndex", offsets.PlayerState_TeamIndex);

    apply("Weapon_FireStartLocation", offsets.Weapon_FireStartLocation);
    apply("Weapon_LastFireLocation", offsets.Weapon_LastFireLocation);
}

inline bool write_defaults(const std::string& path) {
    std::ifstream existing(path);
    if (existing.is_open()) {
        return false;
    }

    const Offsets& offsets = get();
    nlohmann::json json;

    json["GWorld"] = offsets.GWorld;
    json["GNames"] = offsets.GNames;
    json["GEngine"] = offsets.GEngine;

    json["World_OwningGameInstance"] = offsets.World_OwningGameInstance;
    json["World_PersistentLevel"] = offsets.World_PersistentLevel;
    json["World_GameState"] = offsets.World_GameState;

    json["Level_ActorCluster"] = offsets.Level_ActorCluster;
    json["ActorContainer_Actors"] = offsets.ActorContainer_Actors;
    json["Level_Actors"] = offsets.Level_Actors;

    json["GameInstance_LocalPlayers"] = offsets.GameInstance_LocalPlayers;
    json["LocalPlayer_PlayerController"] = offsets.LocalPlayer_PlayerController;

    json["Controller_ControlRotation"] = offsets.Controller_ControlRotation;
    json["Controller_AcknowledgedPawn"] = offsets.Controller_AcknowledgedPawn;
    json["Controller_PlayerCameraManager"] = offsets.Controller_PlayerCameraManager;

    json["Camera_CameraCachePrivate"] = offsets.Camera_CameraCachePrivate;
    json["CameraCache_POV"] = offsets.CameraCache_POV;
    json["POV_Location"] = offsets.POV_Location;
    json["POV_Rotation"] = offsets.POV_Rotation;
    json["POV_FOV"] = offsets.POV_FOV;

    json["Actor_RootComponent"] = offsets.Actor_RootComponent;
    json["Pawn_PlayerState"] = offsets.Pawn_PlayerState;
    json["Character_Mesh"] = offsets.Character_Mesh;
    json["Pawn_CurrentWeapon"] = offsets.Pawn_CurrentWeapon;
    json["Pawn_Health"] = offsets.Pawn_Health;
    json["Pawn_bIsDBNO"] = offsets.Pawn_bIsDBNO;

    json["Scene_RelativeLocation"] = offsets.Scene_RelativeLocation;
    json["Scene_ComponentToWorld"] = offsets.Scene_ComponentToWorld;

    json["Mesh_BoneArray"] = offsets.Mesh_BoneArray;
    json["Mesh_BoneStride"] = offsets.Mesh_BoneStride;

    json["PlayerState_PlayerName"] = offsets.PlayerState_PlayerName;
    json["PlayerState_TeamIndex"] = offsets.PlayerState_TeamIndex;

    json["Weapon_FireStartLocation"] = offsets.Weapon_FireStartLocation;
    json["Weapon_LastFireLocation"] = offsets.Weapon_LastFireLocation;

    std::ofstream output(path);
    if (!output.is_open()) {
        return false;
    }
    output << json.dump(4) << std::endl;
    return true;
}

} // namespace off
