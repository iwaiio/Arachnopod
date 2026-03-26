#include "cs_operating_mode.hpp"

#include <sstream>

#include "cs_operating_mode_internal.hpp"

namespace cs::operating_mode {

void reset() {
  internal::G_STATE = internal::state_t{};
  internal::G_STATE.background_enabled[internal::target_index(control::target_t::tcs)] = true;
}

void step() {
  internal::update_completion_feedback();
  internal::tick_cooldowns();
}

bool set_focus_target(const control::target_t target) {
  if (!internal::valid_focus_target(target)) {
    return false;
  }

  if (internal::G_STATE.focus == target) {
    return true;
  }

  internal::G_STATE.focus = target;
  internal::clear_target_state(target);
  return true;
}

void clear_focus_target() {
  internal::G_STATE.focus = control::target_t::any;
}

bool set_background_target_enabled(const control::target_t target, const bool enabled) {
  if (!internal::valid_background_target(target)) {
    return false;
  }

  internal::G_STATE.background_enabled[internal::target_index(target)] = enabled;
  if (enabled) {
    internal::clear_target_state(target);
  }
  return true;
}

bool background_target_enabled(const control::target_t target) {
  if (!internal::valid_background_target(target)) {
    return false;
  }

  return internal::G_STATE.background_enabled[internal::target_index(target)];
}

bool target_in_cooldown(const control::target_t target) {
  if (!internal::valid_background_target(target)) {
    return false;
  }

  return internal::G_STATE.cooldown_ticks[internal::target_index(target)] != 0U;
}

bool get_target_status(const control::target_t target, target_status_t& out) {
  return internal::build_target_status(target, out);
}

void log_status(const control::target_t target) {
  auto log_one = [](const control::target_t item) {
    target_status_t status{};
    if (!internal::build_target_status(item, status)) {
      return;
    }

    std::ostringstream oss;
    oss << "cs status: target=" << control::target_to_string(item)
        << " bg=" << (status.background_enabled ? 1 : 0)
        << " focused=" << (status.focused ? 1 : 0)
        << " cooldown=" << (status.in_cooldown ? 1 : 0)
        << " cooldown_ticks=" << status.cooldown_ticks
        << " failure_streak=" << static_cast<unsigned>(status.failure_streak)
        << " last_result=" << (status.has_last_result ? internal::result_to_string(status.last_result) : "none");
    internal::log_info(oss.str());
  };

  std::ostringstream header;
  header << "cs status: mode=" << ((current_mode() == mode_t::focus) ? "focus" : "nominal")
         << " focus=" << control::target_to_string(focus_target());
  internal::log_info(header.str());

  if (target != control::target_t::any) {
    log_one(target);
    return;
  }

  log_one(control::target_t::pss);
  log_one(control::target_t::tcs);
  log_one(control::target_t::tms);
  log_one(control::target_t::mns);
  log_one(control::target_t::ls);
}

mode_t current_mode() {
  return (internal::G_STATE.focus == control::target_t::any) ? mode_t::nominal : mode_t::focus;
}

control::target_t focus_target() {
  return internal::G_STATE.focus;
}

bool is_target_focused(const control::target_t target) {
  return (target != control::target_t::any) && (target == internal::G_STATE.focus);
}

scheduler::queue_class_t queue_for(const control::target_t target) {
  return is_target_focused(target) ? scheduler::queue_class_t::priority : scheduler::queue_class_t::normal;
}

node_policy_t policy_for(const control::target_t target) {
  node_policy_t policy{};
  if (target_in_cooldown(target)) {
    policy.periodic_exchange = false;
    policy.queue = scheduler::queue_class_t::normal;
    return policy;
  }

  policy.periodic_exchange = background_target_enabled(target) || is_target_focused(target);
  policy.queue = is_target_focused(target) ? scheduler::queue_class_t::priority : scheduler::queue_class_t::normal;
  return policy;
}

}  // namespace cs::operating_mode
