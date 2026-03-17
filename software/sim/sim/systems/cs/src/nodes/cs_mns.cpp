#include "cs_nodes.hpp"

namespace cs::nodes {

void mns_step() {
  apply_periodic_policy(control::target_t::mns);
}

}  // namespace cs::nodes
