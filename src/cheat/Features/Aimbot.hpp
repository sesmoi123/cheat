#pragma once

#include <vector>

#include "SDK/UE.hpp"

namespace aimbot {

void update(const sdk::Camera& camera, const std::vector<sdk::Player>& players);
void draw_fov(const sdk::Camera& camera);
bool has_target();
const sdk::Player& target();

} // namespace aimbot
