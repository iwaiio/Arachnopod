#include "cs_systems.hpp"

#include "cs_systems_internal.hpp"

namespace cs::systems {

void reset() {}

void step() {
  internal::run_node_step_order();
}

}  // namespace cs::systems
