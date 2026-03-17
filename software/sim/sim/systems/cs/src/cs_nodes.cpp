#include "cs_nodes.hpp"

#include "cs_operating_mode.hpp"
#include "cs_scheduler.hpp"

namespace cs::nodes {

void apply_periodic_policy(const control::target_t target) {
  const auto policy = operating_mode::policy_for(target);
  if (policy.periodic_exchange) {
    (void)scheduler::ensure_periodic_exchange(target, policy.queue);
  } else {
    (void)scheduler::cancel_periodic_exchange(target);
  }
}

}  // namespace cs::nodes
