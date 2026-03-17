#include "cs_systems.hpp"

#include "cs_nodes.hpp"

namespace cs::systems {

void reset() {}

void step() {
  nodes::tcs_step();
  nodes::pss_step();
  nodes::tms_step();
  nodes::mns_step();
  nodes::ls_step();
}

}  // namespace cs::systems
