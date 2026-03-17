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

void reset();
void step();

bool set_focus_target(control::target_t target);
void clear_focus_target();
bool set_background_target_enabled(control::target_t target, bool enabled);
bool background_target_enabled(control::target_t target);

mode_t current_mode();
control::target_t focus_target();
bool is_target_focused(control::target_t target);
scheduler::queue_class_t queue_for(control::target_t target);
node_policy_t policy_for(control::target_t target);

}  // namespace cs::operating_mode
