#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>

#include "cs_scheduler.hpp"

namespace isysalgo {
enum class exchange_result_t : std::uint8_t;
struct bus_state_t;
}

namespace cs::scheduler::internal {

constexpr std::size_t k_max_tasks = 32U;
constexpr std::size_t k_invalid_slot = static_cast<std::size_t>(-1);
constexpr task_id_t k_invalid_task_id = 0U;
constexpr std::size_t k_priority_burst_limit = 3U;

enum class task_kind_t : std::uint8_t {
  command = 0,
  data_request,
};

enum class task_queue_t : std::uint8_t {
  normal = 0,
  priority,
};

struct node_layout_t {
  control::target_t target{control::target_t::any};
  std::uint8_t bus_addr{0};
  std::uint8_t system_id{SYS_NONE};
  std::size_t com_cs_base_block{0};
  std::size_t com_blocks{0};
  std::size_t par_cs_base_block{0};
  std::size_t par_blocks{0};
  const char* name{""};
};

struct task_t {
  bool used{false};
  bool queued{false};
  bool active{false};
  bool exchange_armed{false};
  bool periodic{false};
  bool enabled{false};
  bool use_staged_window{false};
  std::uint32_t id{0};
  task_kind_t kind{task_kind_t::data_request};
  task_queue_t queue{task_queue_t::normal};
  const node_layout_t* node{nullptr};
  std::uint16_t command_id{0};
  float command_value{1.0F};
  bool has_value{false};
};

struct task_completion_t {
  bool valid{false};
  std::uint32_t id{0};
  control::target_t target{control::target_t::any};
  const char* target_name{""};
  task_kind_t kind{task_kind_t::data_request};
  bool periodic{false};
  isysalgo::exchange_result_t result;
  bool have_result{false};
};

extern context_t G_CTX;
extern bool G_READY;
extern std::uint32_t G_NEXT_TASK_ID;
extern std::size_t G_ACTIVE_TASK_SLOT;
extern std::array<node_layout_t, 5> G_NODES;
extern std::array<task_t, k_max_tasks> G_TASKS;
extern std::deque<std::size_t> G_NORMAL_QUEUE;
extern std::deque<std::size_t> G_PRIORITY_QUEUE;
extern std::deque<completion_t> G_COMPLETIONS;
extern std::size_t G_PRIORITY_BURST;
extern std::array<std::array<std::uint16_t, CS_COMM_BUF_BLOCKS>, 5> G_PENDING_CMD;

void log_info(const std::string& message);
void log_warning(const std::string& message);
const char* task_kind_to_string(task_kind_t kind);
completion_kind_t to_public_completion_kind(task_kind_t kind);
completion_result_t to_public_completion_result(isysalgo::exchange_result_t result);
std::uint8_t type_width_bits(std::uint8_t type);
const node_layout_t* find_node_by_target(control::target_t target);
std::size_t node_slot(const node_layout_t& node);
bool command_matches_node(const node_layout_t& node, std::uint16_t command_id);
std::size_t find_cs_command_base_block(std::uint8_t system_id);
std::size_t find_cs_param_base_block(std::uint8_t system_id);
void init_node_layouts();
bool window_fits(std::size_t offset, std::size_t blocks, std::size_t capacity);
void clear_task_slot(std::size_t slot);
void remove_slot_from_queue(std::deque<std::size_t>& queue, std::size_t slot);
void remove_slot_from_queues(std::size_t slot);
std::size_t alloc_task_slot();
void queue_task_slot(std::size_t slot);
std::size_t pop_next_task_slot();
task_queue_t to_task_queue(queue_class_t queue);
task_id_t add_task(const node_layout_t& node,
                   task_kind_t kind,
                   task_queue_t queue_kind,
                   bool periodic,
                   std::uint16_t command_id = 0,
                   float command_value = 1.0F,
                   bool has_value = false,
                   bool use_staged_window = false);
task_id_t add_staged_command_task(const node_layout_t& node, task_queue_t queue_kind);
std::size_t remove_tasks_for_node(const node_layout_t& node);
std::size_t find_task_slot_by_id(task_id_t task_id);
std::size_t find_periodic_data_task_slot(const node_layout_t& node);
void update_task_queue_class(std::size_t slot, task_queue_t queue_kind);
void clear_command_msg_window(const node_layout_t& node);
void clear_command_buffer_window(const node_layout_t& node);
void clear_pending_command_window(const node_layout_t& node);
void load_pending_command_window(const node_layout_t& node);
void save_pending_command_window(const node_layout_t& node);
void merge_command_window_to_buffer(const node_layout_t& node);
task_completion_t finalize_active_task(isysalgo::bus_state_t& bus);
void activate_task(std::size_t slot);
void activate_next_task();

}  // namespace cs::scheduler::internal
