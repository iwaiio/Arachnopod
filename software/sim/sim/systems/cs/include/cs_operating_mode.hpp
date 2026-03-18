#pragma once

#include "cs_scheduler.hpp"
#include "i_control_console.hpp"

namespace cs::operating_mode {

enum class mode_t : unsigned char {
  nominal = 0,
  focus,
};

struct node_policy_t {
  bool periodic_exchange{false};
  scheduler::queue_class_t queue{scheduler::queue_class_t::normal};
};

struct target_status_t {
  bool background_enabled{false};
  bool focused{false};
  bool in_cooldown{false};
  std::uint8_t failure_streak{0U};
  std::uint16_t cooldown_ticks{0U};
  bool has_last_result{false};
  scheduler::completion_result_t last_result{scheduler::completion_result_t::none};
};

void reset();
void step();

bool set_focus_target(control::target_t target);
void clear_focus_target();
bool set_background_target_enabled(control::target_t target, bool enabled);
bool background_target_enabled(control::target_t target);
bool target_in_cooldown(control::target_t target);
bool get_target_status(control::target_t target, target_status_t& out);
void log_status(control::target_t target = control::target_t::any);

mode_t current_mode();
control::target_t focus_target();
bool is_target_focused(control::target_t target);
scheduler::queue_class_t queue_for(control::target_t target);
node_policy_t policy_for(control::target_t target);

}  // namespace cs::operating_mode
