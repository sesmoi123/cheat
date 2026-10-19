#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "SDK/Math.hpp"

namespace sdk {

struct Player {
    uintptr_t pawn = 0;
    uintptr_t playerState = 0;
    uintptr_t mesh = 0;
    std::wstring name;
    Vector3 location;
    double distance = 0.0;
    float health = 100.0f;
    int team = 0;
    bool isLocal = false;
    bool isBot = false;
    bool isDBNO = false;
};

bool initialize();
void update();

const std::vector<Player>& players();
uintptr_t local_pawn();
uintptr_t local_controller();
const Vector3& camera_location();
bool alive();
int local_team();

bool world_to_screen(const Vector3& world, Vector2& out);
bool bone_position(uintptr_t mesh, int index, Vector3& out);

extern int screen_width;
extern int screen_height;

} // namespace sdk
