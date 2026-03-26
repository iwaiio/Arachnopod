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

enum class completion_kind_t : std::uint8_t {
  command = 0,
  data_request,
};

enum class completion_result_t : std::uint8_t {
  none = 0,
  success,
  timeout,
  invalid_eof,
};

struct completion_t {
  bool valid{false};
  control::target_t target{control::target_t::any};
  completion_kind_t kind{completion_kind_t::data_request};
  bool periodic{false};
  bool have_result{false};
  completion_result_t result{completion_result_t::none};
};

struct context_t {
  const ipar::context_t* imsg_ctx{nullptr};
  std::uint16_t* msg_cmd{nullptr};
  std::size_t msg_cmd_blocks{0};
  std::uint16_t* msg_cmd_buf{nullptr};
  std::size_t msg_cmd_buf_blocks{0};
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
                          float command_value = 1.0F,
                          bool has_value = false,
                          queue_class_t queue = queue_class_t::normal);
bool cancel_periodic_exchange(control::target_t target);
bool cancel_task(task_id_t task_id);
std::size_t cancel_target(control::target_t target);
bool try_pop_completion(completion_t& out);

void exchange_control(isysalgo::bus_state_t& bus);

}  // namespace cs::scheduler
