#pragma once

#include <array>
#include <cstddef>

namespace cs::systems::internal {

using step_fn_t = void (*)();

extern const std::array<step_fn_t, 5> G_NODE_STEP_ORDER;

void run_node_step_order();

}  // namespace cs::systems::internal
