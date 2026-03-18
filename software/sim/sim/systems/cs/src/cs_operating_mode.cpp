#include "cs_operating_mode.hpp"

#include <array>
#include <sstream>

#include "common.hpp"

namespace cs::operating_mode {
namespace {

constexpr std::uint8_t k_timeout_failure_threshold = 3U;
constexpr std::uint8_t k_invalid_eof_failure_threshold = 1U;
constexpr std::uint16_t k_timeout_cooldown_ticks = 96U;
constexpr std::uint16_t k_invalid_eof_cooldown_ticks = 160U;

struct state_t {
  control::target_t focus{control::target_t::any};
  std::array<bool, static_cast<std::size_t>(control::target_t::count)> background_enabled{};
  std::array<std::uint8_t, static_cast<std::size_t>(control::target_t::count)> failure_streak{};
  std::array<std::uint16_t, static_cast<std::size_t>(control::target_t::count)> cooldown_ticks{};
  std::array<bool, static_cast<std::size_t>(control::target_t::count)> has_last_result{};
  std::array<scheduler::completion_result_t, static_cast<std::size_t>(control::target_t::count)> last_result{};
};

state_t G_STATE{};

void log_info(const std::string& message) {
  common::log::info(message);
}

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

void clear_runtime_state(const control::target_t target) {
  const auto idx = target_index(target);
  G_STATE.failure_streak[idx] = 0U;
  G_STATE.cooldown_ticks[idx] = 0U;
}

const char* result_to_string(const scheduler::completion_result_t result) {
  switch (result) {
    case scheduler::completion_result_t::success:
      return "success";
    case scheduler::completion_result_t::timeout:
      return "timeout";
    case scheduler::completion_result_t::invalid_eof:
      return "invalid_eof";
    case scheduler::completion_result_t::none:
    default:
      return "none";
  }
}

std::uint8_t failure_threshold_for(const scheduler::completion_result_t result) {
  switch (result) {
    case scheduler::completion_result_t::invalid_eof:
      return k_invalid_eof_failure_threshold;
    case scheduler::completion_result_t::timeout:
    case scheduler::completion_result_t::none:
    case scheduler::completion_result_t::success:
    default:
      return k_timeout_failure_threshold;
  }
}

std::uint16_t cooldown_ticks_for(const scheduler::completion_result_t result) {
  switch (result) {
    case scheduler::completion_result_t::invalid_eof:
      return k_invalid_eof_cooldown_ticks;
    case scheduler::completion_result_t::timeout:
    case scheduler::completion_result_t::none:
    case scheduler::completion_result_t::success:
    default:
      return k_timeout_cooldown_ticks;
  }
}

void remember_result(const control::target_t target, const scheduler::completion_result_t result) {
  const auto idx = target_index(target);
  G_STATE.has_last_result[idx] = true;
  G_STATE.last_result[idx] = result;
}

bool build_target_status(const control::target_t target, target_status_t& out) {
  if (!valid_background_target(target)) {
    return false;
  }

  const auto idx = target_index(target);
  out = target_status_t{
      .background_enabled = G_STATE.background_enabled[idx],
      .focused = (G_STATE.focus == target),
      .in_cooldown = (G_STATE.cooldown_ticks[idx] != 0U),
      .failure_streak = G_STATE.failure_streak[idx],
      .cooldown_ticks = G_STATE.cooldown_ticks[idx],
      .has_last_result = G_STATE.has_last_result[idx],
      .last_result = G_STATE.last_result[idx],
  };
  return true;
}

void on_exchange_success(const control::target_t target) {
  if (!valid_background_target(target)) {
    return;
  }

  const auto idx = target_index(target);
  remember_result(target, scheduler::completion_result_t::success);
  const bool had_penalty = (G_STATE.failure_streak[idx] != 0U) || (G_STATE.cooldown_ticks[idx] != 0U);
  clear_runtime_state(target);

  if (had_penalty) {
    std::ostringstream oss;
    oss << "cs operating_mode: connectivity restored for " << control::target_to_string(target);
    log_info(oss.str());
  }
}

void on_exchange_failure(const control::target_t target, const scheduler::completion_result_t result) {
  if (!valid_background_target(target)) {
    return;
  }

  const auto idx = target_index(target);
  remember_result(target, result);
  auto& streak = G_STATE.failure_streak[idx];
  auto& cooldown = G_STATE.cooldown_ticks[idx];
  if (streak < 255U) {
    ++streak;
  }

  if (streak < failure_threshold_for(result)) {
    return;
  }

  if (cooldown == 0U) {
    cooldown = cooldown_ticks_for(result);
    std::ostringstream oss;
    oss << "cs operating_mode: cooldown for " << control::target_to_string(target)
        << " after repeated failures, result=" << result_to_string(result);
    log_info(oss.str());
  }
}

void update_completion_feedback() {
  scheduler::completion_t completion{};
  while (scheduler::try_pop_completion(completion)) {
    if (!completion.valid || !completion.have_result) {
      continue;
    }

    if (completion.result == scheduler::completion_result_t::success) {
      on_exchange_success(completion.target);
      continue;
    }

    if (completion.periodic && (completion.kind == scheduler::completion_kind_t::data_request)) {
      on_exchange_failure(completion.target, completion.result);
    }
  }
}

void tick_cooldowns() {
  for (std::size_t i = 0; i < G_STATE.cooldown_ticks.size(); ++i) {
    auto& cooldown = G_STATE.cooldown_ticks[i];
    if (cooldown == 0U) {
      continue;
    }

    --cooldown;
    if (cooldown == 0U) {
      G_STATE.failure_streak[i] = 0U;
      std::ostringstream oss;
      oss << "cs operating_mode: cooldown ended for "
          << control::target_to_string(static_cast<control::target_t>(i));
      log_info(oss.str());
    }
  }
}

}  // namespace

void reset() {
  G_STATE = state_t{};
  G_STATE.background_enabled[target_index(control::target_t::tcs)] = true;
}

void step() {
  update_completion_feedback();
  tick_cooldowns();
}

bool set_focus_target(const control::target_t target) {
  if (!valid_focus_target(target)) {
    return false;
  }

  if (G_STATE.focus == target) {
    return true;
  }

  G_STATE.focus = target;
  clear_runtime_state(target);
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
  if (enabled) {
    clear_runtime_state(target);
  }
  return true;
}

bool background_target_enabled(const control::target_t target) {
  if (!valid_background_target(target)) {
    return false;
  }

  return G_STATE.background_enabled[target_index(target)];
}

bool target_in_cooldown(const control::target_t target) {
  if (!valid_background_target(target)) {
    return false;
  }

  return G_STATE.cooldown_ticks[target_index(target)] != 0U;
}

bool get_target_status(const control::target_t target, target_status_t& out) {
  return build_target_status(target, out);
}

void log_status(const control::target_t target) {
  auto log_one = [](const control::target_t item) {
    target_status_t status{};
    if (!build_target_status(item, status)) {
      return;
    }

    std::ostringstream oss;
    oss << "cs status: target=" << control::target_to_string(item)
        << " bg=" << (status.background_enabled ? 1 : 0)
        << " focused=" << (status.focused ? 1 : 0)
        << " cooldown=" << (status.in_cooldown ? 1 : 0)
        << " cooldown_ticks=" << status.cooldown_ticks
        << " failure_streak=" << static_cast<unsigned>(status.failure_streak)
        << " last_result=" << (status.has_last_result ? result_to_string(status.last_result) : "none");
    log_info(oss.str());
  };

  std::ostringstream header;
  header << "cs status: mode=" << ((current_mode() == mode_t::focus) ? "focus" : "nominal")
         << " focus=" << control::target_to_string(focus_target());
  log_info(header.str());

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
