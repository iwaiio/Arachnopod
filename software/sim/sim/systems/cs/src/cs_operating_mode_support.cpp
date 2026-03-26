#include "cs_operating_mode_internal.hpp"

#include <array>
#include <sstream>

#include "common.hpp"

namespace cs::operating_mode::internal {

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

void clear_target_state(const control::target_t target) {
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
  clear_target_state(target);

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

}  // namespace cs::operating_mode::internal
