#include "cs_systems_internal.hpp"

#include <array>

#include "cs_nodes.hpp"

namespace cs::systems::internal {

const std::array<step_fn_t, 5> G_NODE_STEP_ORDER{{
    nodes::tcs_step,
    nodes::pss_step,
    nodes::tms_step,
    nodes::mns_step,
    nodes::ls_step,
}};

void run_node_step_order() {
  for (const auto step_fn : G_NODE_STEP_ORDER) {
    if (step_fn != nullptr) {
      step_fn();
    }
  }
}

}  // namespace cs::systems::internal
