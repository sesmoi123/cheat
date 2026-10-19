#pragma once

#include <vector>

#include "SDK/UE.hpp"

namespace bullet_tp {

void update(const sdk::Camera& camera, const std::vector<sdk::Player>& players);

} // namespace bullet_tp
