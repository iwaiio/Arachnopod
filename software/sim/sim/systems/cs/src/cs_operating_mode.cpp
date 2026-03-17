#include "cs_operating_mode.hpp"

#include <array>

namespace cs::operating_mode {
namespace {

struct state_t {
  control::target_t focus{control::target_t::any};
  std::array<bool, static_cast<std::size_t>(control::target_t::count)> background_enabled{};
};

state_t G_STATE{};

bool valid_focus_target(const control::target_t target) {
  switch (target) {
    case control::target_t::pss:
    case control::target_t::tcs:
    case control::target_t::tms:
    case control::target_t::mns:
    case control::target_t::ls:
      return true;
    default:
      return false;
  }
}

bool valid_background_target(const control::target_t target) {
  switch (target) {
    case control::target_t::pss:
    case control::target_t::tcs:
    case control::target_t::tms:
    case control::target_t::mns:
    case control::target_t::ls:
      return true;
    default:
      return false;
  }
}

std::size_t target_index(const control::target_t target) {
  return static_cast<std::size_t>(target);
}

}  // namespace

void reset() {
  G_STATE = state_t{};
  G_STATE.background_enabled[target_index(control::target_t::tcs)] = true;
}

void step() {}

bool set_focus_target(const control::target_t target) {
  if (!valid_focus_target(target)) {
    return false;
  }

  if (G_STATE.focus == target) {
    return true;
  }

  G_STATE.focus = target;
  return true;
}

void clear_focus_target() {
  G_STATE.focus = control::target_t::any;
}

bool set_background_target_enabled(const control::target_t target, const bool enabled) {
  if (!valid_background_target(target)) {
    return false;
  }

  G_STATE.background_enabled[target_index(target)] = enabled;
  return true;
}

bool background_target_enabled(const control::target_t target) {
  if (!valid_background_target(target)) {
    return false;
  }

  return G_STATE.background_enabled[target_index(target)];
}

mode_t current_mode() {
  return (G_STATE.focus == control::target_t::any) ? mode_t::nominal : mode_t::focus;
}

control::target_t focus_target() {
  return G_STATE.focus;
}

bool is_target_focused(const control::target_t target) {
  return (target != control::target_t::any) && (target == G_STATE.focus);
}

scheduler::queue_class_t queue_for(const control::target_t target) {
  return is_target_focused(target) ? scheduler::queue_class_t::priority : scheduler::queue_class_t::normal;
}

node_policy_t policy_for(const control::target_t target) {
  node_policy_t policy{};

  policy.periodic_exchange = background_target_enabled(target) || is_target_focused(target);
  policy.queue = is_target_focused(target) ? scheduler::queue_class_t::priority : scheduler::queue_class_t::normal;

  return policy;
}

}  // namespace cs::operating_mode
