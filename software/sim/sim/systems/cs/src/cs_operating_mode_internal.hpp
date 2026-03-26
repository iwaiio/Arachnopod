#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "cs_operating_mode.hpp"

namespace cs::operating_mode::internal {

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

extern state_t G_STATE;

void log_info(const std::string& message);
bool valid_focus_target(control::target_t target);
bool valid_background_target(control::target_t target);
std::size_t target_index(control::target_t target);
void clear_target_state(control::target_t target);
const char* result_to_string(scheduler::completion_result_t result);
std::uint8_t failure_threshold_for(scheduler::completion_result_t result);
std::uint16_t cooldown_ticks_for(scheduler::completion_result_t result);
void remember_result(control::target_t target, scheduler::completion_result_t result);
bool build_target_status(control::target_t target, target_status_t& out);
void on_exchange_success(control::target_t target);
void on_exchange_failure(control::target_t target, scheduler::completion_result_t result);
void update_completion_feedback();
void tick_cooldowns();

}  // namespace cs::operating_mode::internal
