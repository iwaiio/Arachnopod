#include "cs_nodes.hpp"

namespace cs::nodes {

void ls_step() {
  apply_periodic_policy(control::target_t::ls);
}

}  // namespace cs::nodes
