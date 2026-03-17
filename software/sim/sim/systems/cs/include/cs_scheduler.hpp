#pragma once

#include <cstddef>
#include <cstdint>

#include "i_control_console.hpp"
#include "i_par.hpp"

namespace isysalgo {
struct bus_state_t;
}

namespace cs::scheduler {

using task_id_t = std::uint32_t;

enum class queue_class_t : std::uint8_t {
  normal = 0,
  priority,
};

struct context_t {
  const ipar::context_t* ipar_ctx{nullptr};
  std::uint16_t* msg_com{nullptr};
  std::size_t msg_com_blocks{0};
  std::uint16_t* msg_com_buf{nullptr};
  std::size_t msg_com_buf_blocks{0};
};

void init(const context_t& ctx);
void reset();

task_id_t enqueue_exchange(control::target_t target,
                           queue_class_t queue = queue_class_t::normal,
                           bool periodic = false);
task_id_t ensure_periodic_exchange(control::target_t target,
                                   queue_class_t queue = queue_class_t::normal);
task_id_t enqueue_command(control::target_t target,
                          std::uint16_t command_id,
                          std::int32_t command_value = 1,
                          bool has_value = false,
                          queue_class_t queue = queue_class_t::normal);
bool cancel_periodic_exchange(control::target_t target);
bool cancel_task(task_id_t task_id);
std::size_t cancel_target(control::target_t target);

void local_algorithm();
void exchange_control(isysalgo::bus_state_t& bus);

}  // namespace cs::scheduler
