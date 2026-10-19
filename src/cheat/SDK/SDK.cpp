#include "SDK/SDK.hpp"

#include "Logger.hpp"
#include "Memory.hpp"
#include "SDK/Offsets.hpp"

#include <algorithm>

namespace sdk {

int screen_width = 1920;
int screen_height = 1080;

namespace {

uintptr_t g_base = 0;
uintptr_t g_controller = 0;
uintptr_t g_localPawn = 0;
int g_localTeam = 0;
bool g_cameraValid = false;

Vector3 g_cameraLocation;
Rotator g_cameraRotation;
double g_cameraFov = 90.0;

std::vector<Player> g_players;

std::wstring read_fstring(uintptr_t address) {
    // FString = TArray<wchar_t> : data a +0, count a +8.
    const uintptr_t data = memory::read<uintptr_t>(address);
    const int32_t count = memory::read<int32_t>(address + 8);
    if (!data || count <= 0 || count > 64) {
        return {};
    }

    std::wstring result;
    result.reserve(static_cast<size_t>(count));
    for (int32_t index = 0; index < count - 1; ++index) {
        const wchar_t character = memory::read<wchar_t>(data + static_cast<uintptr_t>(index) * 2);
        if (!character) {
            break;
        }
        result.push_back(character);
    }
    return result;
}

void read_camera(uintptr_t controller) {
    g_cameraValid = false;

    const uintptr_t manager =
        memory::read<uintptr_t>(controller + offsets::g.PlayerController_PlayerCameraManager);
    if (!manager) {
        return;
    }

    const uintptr_t pov =
        manager + offsets::g.CameraManager_CameraCache + offsets::g.CameraCache_POV;

    g_cameraLocation = memory::read<Vector3>(pov);
    g_cameraRotation = memory::read<Rotator>(pov + 0x18);
    g_cameraFov = memory::read<double>(pov + 0x30);

    if (g_cameraFov < 1.0 || g_cameraFov > 170.0) {
        g_cameraFov = 90.0;
    }

    g_cameraValid = true;
}

void collect_players(uintptr_t world) {
    const uintptr_t levels = memory::read<uintptr_t>(world + offsets::g.World_Levels);
    if (!levels) {
        return;
    }

    const uintptr_t levelData = memory::read<uintptr_t>(levels);
    const int32_t levelCount = memory::read<int32_t>(levels + 8);
    if (!levelData || levelCount <= 0 || levelCount > 64) {
        return;
    }

    for (int32_t levelIndex = 0; levelIndex < levelCount; ++levelIndex) {
        const uintptr_t level =
            memory::read<uintptr_t>(levelData + static_cast<uintptr_t>(levelIndex) * 8);
        if (!level) {
            continue;
        }

        const uintptr_t actors = memory::read<uintptr_t>(level + offsets::g.Level_Actors);
        const int32_t actorCount = memory::read<int32_t>(level + offsets::g.Level_Actors + 8);
        if (!actors || actorCount <= 0 || actorCount > 20000) {
            continue;
        }

        for (int32_t actorIndex = 0; actorIndex < actorCount; ++actorIndex) {
            const uintptr_t actor =
                memory::read<uintptr_t>(actors + static_cast<uintptr_t>(actorIndex) * 8);
            if (!actor) {
                continue;
            }

            const uintptr_t playerState = memory::read<uintptr_t>(actor + offsets::g.Pawn_PlayerState);
            const uintptr_t mesh = memory::read<uintptr_t>(actor + offsets::g.Pawn_Mesh);
            if (!playerState || !mesh) {
                continue;
            }

            const uintptr_t root = memory::read<uintptr_t>(actor + offsets::g.Actor_RootComponent);
            if (!root) {
                continue;
            }

            Player player;
            player.pawn = actor;
            player.playerState = playerState;
            player.mesh = mesh;
            player.location =
                memory::read<Vector3>(root + offsets::g.SceneComponent_RelativeLocation);
            player.name = read_fstring(playerState + offsets::g.PlayerState_PlayerName);
            player.health = memory::read<float>(actor + offsets::g.Pawn_Health);
            player.team = memory::read<int32_t>(playerState + offsets::g.PlayerState_TeamIndex);
            player.isBot = memory::read<uint8_t>(playerState + offsets::g.PlayerState_IsABot) != 0;
            player.isDBNO = memory::read<uint8_t>(playerState + offsets::g.PlayerState_IsDBNO) != 0;
            player.isLocal = actor == g_localPawn;
            player.distance = distance(player.location, g_cameraLocation) / 100.0;

            if (player.health <= 0.0f || player.health > 200.0f) {
                player.health = 100.0f;
            }

            g_players.push_back(player);
        }
    }

    std::sort(g_players.begin(), g_players.end(),
              [](const Player& a, const Player& b) { return a.distance < b.distance; });
}

} // namespace

bool initialize() {
    g_base = memory::module_base(nullptr);
    if (!g_base) {
        logger::error("module principal introuvable");
        return false;
    }

    logger::info("base=%p GWorld=%p", reinterpret_cast<void*>(g_base),
                 reinterpret_cast<void*>(memory::read<uintptr_t>(g_base + offsets::g.GWorld)));
    return true;
}

void update() {
    g_players.clear();
    g_cameraValid = false;

    if (!g_base) {
        return;
    }

    const uintptr_t world = memory::read<uintptr_t>(g_base + offsets::g.GWorld);
    if (!world) {
        return;
    }

    const uintptr_t gameInstance =
        memory::read<uintptr_t>(world + offsets::g.World_OwningGameInstance);
    if (!gameInstance) {
        return;
    }

    const uintptr_t localPlayers =
        memory::read<uintptr_t>(gameInstance + offsets::g.GameInstance_LocalPlayers);
    if (!localPlayers) {
        return;
    }

    const uintptr_t localPlayer = memory::read<uintptr_t>(localPlayers);
    if (!localPlayer) {
        return;
    }

    const uintptr_t controller =
        memory::read<uintptr_t>(localPlayer + offsets::g.LocalPlayer_PlayerController);
    if (!controller) {
        return;
    }

    g_controller = controller;
    g_localPawn =
        memory::read<uintptr_t>(controller + offsets::g.PlayerController_AcknowledgedPawn);

    read_camera(controller);

    if (g_localPawn) {
        const uintptr_t localState =
            memory::read<uintptr_t>(g_localPawn + offsets::g.Pawn_PlayerState);
        g_localTeam =
            localState ? memory::read<int32_t>(localState + offsets::g.PlayerState_TeamIndex) : 0;
    }

    collect_players(world);
}

const std::vector<Player>& players() {
    return g_players;
}

uintptr_t local_pawn() {
    return g_localPawn;
}

uintptr_t local_controller() {
    return g_controller;
}

const Vector3& camera_location() {
    return g_cameraLocation;
}

bool alive() {
    return g_localPawn != 0 && g_controller != 0 && g_cameraValid;
}

int local_team() {
    return g_localTeam;
}

bool world_to_screen(const Vector3& world, Vector2& out) {
    if (!g_cameraValid || screen_width <= 0 || screen_height <= 0) {
        return false;
    }

    const double pitch = deg2rad(g_cameraRotation.pitch);
    const double yaw = deg2rad(g_cameraRotation.yaw);

    const Vector3 forward{std::cos(pitch) * std::cos(yaw), std::cos(pitch) * std::sin(yaw),
                          std::sin(pitch)};
    const Vector3 right{-std::sin(yaw), std::cos(yaw), 0.0};
    const Vector3 up{-std::cos(yaw) * std::sin(pitch), -std::sin(yaw) * std::sin(pitch),
                     std::cos(pitch)};

    const Vector3 delta = subtract(world, g_cameraLocation);

    const double depth = dot(delta, forward);
    if (depth < 1.0) {
        return false;
    }

    const double horizontal = dot(delta, right);
    const double vertical = dot(delta, up);

    const double tanHalf = std::tan(deg2rad(g_cameraFov) / 2.0);
    const double aspect = static_cast<double>(screen_width) / static_cast<double>(screen_height);

    out.x = static_cast<float>(screen_width / 2.0 *
                               (1.0 + horizontal / (depth * tanHalf * aspect)));
    out.y = static_cast<float>(screen_height / 2.0 * (1.0 - vertical / (depth * tanHalf)));

    return true;
}

bool bone_position(uintptr_t mesh, int index, Vector3& out) {
    if (!mesh) {
        return false;
    }

    const uintptr_t array = memory::read<uintptr_t>(mesh + offsets::g.Mesh_BoneArray);
    const int32_t count = memory::read<int32_t>(mesh + offsets::g.Mesh_BoneArray + 8);
    if (!array || index < 0 || index >= count) {
        return false;
    }

    const FTransform bone =
        memory::read<FTransform>(array + static_cast<uintptr_t>(index) * sizeof(FTransform));
    const FTransform toWorld = memory::read<FTransform>(mesh + offsets::g.Mesh_ComponentToWorld);

    out = transform_position(toWorld, bone.translation);
    return true;
}

} // namespace sdk
