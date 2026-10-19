#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "Memory.hpp"
#include "SDK/Math.hpp"
#include "SDK/Offsets.hpp"

namespace sdk {

struct TArray {
    uintptr_t data = 0;
    int32_t count = 0;
    int32_t capacity = 0;
};

struct Camera {
    FVector location;
    FRotator rotation;
    double fov = 80.0;
    bool valid = false;
};

struct Player {
    uintptr_t actor = 0;
    uintptr_t mesh = 0;
    uintptr_t state = 0;
    uintptr_t weapon = 0;
    FVector location;
    FVector head;
    FVector feet;
    std::string name;
    double distance = 0.0;
    float health = 100.0f;
    int team = -1;
    bool visible = true;
    bool knocked = false;
    bool is_local = false;
    bool valid = false;
};

namespace bones {
constexpr int Head = 110;
constexpr int Neck = 66;
constexpr int Chest = 37;
constexpr int Pelvis = 2;
constexpr int LeftShoulder = 9;
constexpr int LeftElbow = 10;
constexpr int LeftHand = 11;
constexpr int RightShoulder = 38;
constexpr int RightElbow = 39;
constexpr int RightHand = 40;
constexpr int LeftThigh = 74;
constexpr int LeftKnee = 75;
constexpr int LeftFoot = 76;
constexpr int RightThigh = 80;
constexpr int RightKnee = 81;
constexpr int RightFoot = 82;
} // namespace bones

inline uintptr_t get_world() {
    return mem::read<uintptr_t>(mem::module_base() + off::get().GWorld);
}

inline uintptr_t get_game_instance() {
    const uintptr_t world = get_world();
    return world ? mem::read<uintptr_t>(world + off::get().World_OwningGameInstance) : 0;
}

inline uintptr_t get_local_player() {
    const uintptr_t instance = get_game_instance();
    if (!instance) {
        return 0;
    }
    const uintptr_t players = instance + off::get().GameInstance_LocalPlayers;
    const uintptr_t data = mem::read<uintptr_t>(players);
    return data ? mem::read<uintptr_t>(data) : 0;
}

inline uintptr_t get_local_controller() {
    const uintptr_t player = get_local_player();
    return player ? mem::read<uintptr_t>(player + off::get().LocalPlayer_PlayerController) : 0;
}

inline uintptr_t get_local_pawn() {
    const uintptr_t controller = get_local_controller();
    if (!controller) {
        return 0;
    }
    uintptr_t pawn = mem::read<uintptr_t>(controller + off::get().Controller_AcknowledgedPawn);
    if (!pawn) {
        pawn = mem::read<uintptr_t>(controller + 0x2B0);
    }
    return pawn;
}

inline uintptr_t get_camera_manager() {
    const uintptr_t controller = get_local_controller();
    return controller ? mem::read<uintptr_t>(controller + off::get().Controller_PlayerCameraManager) : 0;
}

inline Camera get_camera() {
    Camera camera;

    const uintptr_t manager = get_camera_manager();
    if (!manager) {
        return camera;
    }

    const uintptr_t cache = manager + off::get().Camera_CameraCachePrivate + off::get().CameraCache_POV;
    camera.location = mem::read<FVector>(cache + off::get().POV_Location);
    camera.rotation = mem::read<FRotator>(cache + off::get().POV_Rotation);
    camera.fov = mem::read<float>(cache + off::get().POV_FOV);

    if (camera.fov < 1.0f || camera.fov > 180.0f) {
        camera.fov = 80.0;
    }

    camera.valid = !camera.location.empty();
    return camera;
}

inline std::string read_fstring(uintptr_t address) {
    const uintptr_t data = mem::read<uintptr_t>(address);
    const int32_t count = mem::read<int32_t>(address + 8);
    if (!data || count <= 0 || count > 256) {
        return {};
    }

    std::vector<wchar_t> buffer(static_cast<size_t>(count) + 1, L'\0');
    if (!mem::read_bytes(data, buffer.data(), static_cast<size_t>(count) * sizeof(wchar_t))) {
        return {};
    }

    std::string result;
    result.reserve(static_cast<size_t>(count));
    for (int32_t i = 0; i < count; ++i) {
        const wchar_t character = buffer[static_cast<size_t>(i)];
        if (character == L'\0') {
            break;
        }
        result.push_back(character >= 32 && character < 127 ? static_cast<char>(character) : '?');
    }
    return result;
}

inline FVector get_bone(uintptr_t mesh, int index) {
    if (!mesh || index < 0) {
        return {};
    }

    const uintptr_t boneArray = mem::read<uintptr_t>(mesh + off::get().Mesh_BoneArray + 0x10);
    if (!boneArray) {
        return {};
    }

    const FTransform bone =
        mem::read<FTransform>(boneArray + static_cast<uintptr_t>(index) * off::get().Mesh_BoneStride);
    const FTransform componentToWorld = mem::read<FTransform>(mesh + off::get().Scene_ComponentToWorld);

    const FMatrix result = multiply(to_matrix_with_scale(bone), to_matrix_with_scale(componentToWorld));
    return {result.M[3][0], result.M[3][1], result.M[3][2]};
}

inline std::vector<uintptr_t> gather_actor_pointers() {
    std::vector<uintptr_t> actors;

    const uintptr_t world = get_world();
    if (!world) {
        return actors;
    }

    const uintptr_t level = mem::read<uintptr_t>(world + off::get().World_PersistentLevel);
    if (!level) {
        return actors;
    }

    uintptr_t arrayAddress = 0;
    const uintptr_t container = mem::read<uintptr_t>(level + off::get().Level_ActorCluster);
    if (container) {
        arrayAddress = container + off::get().ActorContainer_Actors;
    } else {
        arrayAddress = level + off::get().Level_Actors;
    }

    const TArray list = mem::read<TArray>(arrayAddress);
    if (!list.data || list.count <= 0) {
        return actors;
    }

    const int32_t count = std::min<int32_t>(list.count, 65536);
    actors.reserve(static_cast<size_t>(count));

    for (int32_t i = 0; i < count; ++i) {
        const uintptr_t actor =
            mem::read<uintptr_t>(list.data + static_cast<uintptr_t>(i) * sizeof(uintptr_t));
        if (actor) {
            actors.push_back(actor);
        }
    }

    return actors;
}

inline std::vector<Player> gather_players(const Camera& camera) {
    std::vector<Player> players;

    const uintptr_t localPawn = get_local_pawn();

    for (const uintptr_t actor : gather_actor_pointers()) {
        const uintptr_t mesh = mem::read<uintptr_t>(actor + off::get().Character_Mesh);
        if (!mesh) {
            continue;
        }

        const uintptr_t state = mem::read<uintptr_t>(actor + off::get().Pawn_PlayerState);
        if (!state) {
            continue;
        }

        const uintptr_t root = mem::read<uintptr_t>(actor + off::get().Actor_RootComponent);
        if (!root) {
            continue;
        }

        Player player;
        player.actor = actor;
        player.mesh = mesh;
        player.state = state;
        player.weapon = mem::read<uintptr_t>(actor + off::get().Pawn_CurrentWeapon);
        player.location = mem::read<FVector>(root + off::get().Scene_RelativeLocation);
        if (player.location.empty()) {
            continue;
        }

        player.head = get_bone(mesh, bones::Head);
        if (player.head.empty()) {
            player.head = player.location + FVector{0.0, 0.0, 175.0};
        }
        player.feet = player.location;

        player.name = read_fstring(state + off::get().PlayerState_PlayerName);
        player.team = mem::read<int32_t>(state + off::get().PlayerState_TeamIndex);

        const float health = mem::read<float>(actor + off::get().Pawn_Health);
        player.health = (health > 0.0f && health <= 200.0f) ? health : 100.0f;
        player.knocked = mem::read<uint8_t>(actor + off::get().Pawn_bIsDBNO) != 0;

        player.is_local = (actor == localPawn);
        player.distance = player.location.distance(camera.location) / 100.0;
        player.valid = true;

        players.push_back(std::move(player));
    }

    return players;
}

inline int local_team() {
    const uintptr_t pawn = get_local_pawn();
    if (!pawn) {
        return -1;
    }
    const uintptr_t state = mem::read<uintptr_t>(pawn + off::get().Pawn_PlayerState);
    if (!state) {
        return -1;
    }
    const int team = mem::read<int32_t>(state + off::get().PlayerState_TeamIndex);
    return (team > 0 && team < 1000) ? team : -1;
}

inline bool world_to_screen(const FVector& world, const Camera& camera, double screenWidth,
                            double screenHeight, FVector2D& out) {
    if (!camera.valid) {
        return false;
    }
    return sdk::world_to_screen(world, camera.location, camera.rotation, camera.fov, screenWidth,
                                screenHeight, out);
}

} // namespace sdk
