#include "cs_nodes.hpp"

namespace cs::nodes {

void pss_step() {
  apply_periodic_policy(control::target_t::pss);
}

}  // namespace cs::nodes
