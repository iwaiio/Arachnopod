#include "cs_nodes.hpp"

namespace cs::nodes {

void tcs_step() {
  apply_periodic_policy(control::target_t::tcs);
}

}  // namespace cs::nodes
