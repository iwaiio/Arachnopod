#include "cs_nodes.hpp"

namespace cs::nodes {

void tms_step() {
  apply_periodic_policy(control::target_t::tms);
}

}  // namespace cs::nodes
