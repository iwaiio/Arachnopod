#include "cs_scheduler_internal.hpp"

#include <array>
#include <cstdint>
#include <deque>
#include <sstream>

#include "../../base/command_tab.hpp"
#include "../../base/param_tab.hpp"
#include "common.hpp"
#include "i_msg_block.hpp"
#include "i_sys.hpp"
#include "i_sys_algo.hpp"

namespace cs::scheduler::internal {

context_t G_CTX{};
bool G_READY = false;
std::uint32_t G_NEXT_TASK_ID = 1U;
std::size_t G_ACTIVE_TASK_SLOT = k_invalid_slot;

std::array<node_layout_t, 5> G_NODES{{
    {control::target_t::pss, ARP_ADDR_PSS, SYS_PSS, 0U, PSS_COMM_MSG_BLOCKS, 0U, PSS_PARAM_MSG_BLOCKS, "pss"},
    {control::target_t::tcs, ARP_ADDR_TCS, SYS_TCS, 0U, TCS_COMM_MSG_BLOCKS, 0U, TCS_PARAM_MSG_BLOCKS, "tcs"},
    {control::target_t::tms, ARP_ADDR_TMS, SYS_TMS, 0U, TMS_COMM_MSG_BLOCKS, 0U, TMS_PARAM_MSG_BLOCKS, "tms"},
    {control::target_t::mns, ARP_ADDR_MNS, SYS_MNS, 0U, MNS_COMM_MSG_BLOCKS, 0U, MNS_PARAM_MSG_BLOCKS, "mns"},
    {control::target_t::ls, ARP_ADDR_LS, SYS_LS, 0U, LS_COMM_MSG_BLOCKS, 0U, LS_PARAM_MSG_BLOCKS, "ls"},
}};

std::array<task_t, k_max_tasks> G_TASKS{};
std::deque<std::size_t> G_NORMAL_QUEUE{};
std::deque<std::size_t> G_PRIORITY_QUEUE{};
std::deque<completion_t> G_COMPLETIONS{};
std::size_t G_PRIORITY_BURST = 0U;
std::array<std::array<std::uint16_t, CS_COMM_BUF_BLOCKS>, 5> G_PENDING_CMD{};

void log_info(const std::string& message) {
  common::log::info(message);
}

void log_warning(const std::string& message) {
  common::log::warning(message);
}

const char* task_kind_to_string(const task_kind_t kind) {
  switch (kind) {
    case task_kind_t::command:
      return "cmd";
    case task_kind_t::data_request:
    default:
      return "data";
  }
}

completion_kind_t to_public_completion_kind(const task_kind_t kind) {
  return (kind == task_kind_t::command) ? completion_kind_t::command : completion_kind_t::data_request;
}

completion_result_t to_public_completion_result(const isysalgo::exchange_result_t result) {
  switch (result) {
    case isysalgo::exchange_result_t::success:
      return completion_result_t::success;
    case isysalgo::exchange_result_t::timeout:
      return completion_result_t::timeout;
    case isysalgo::exchange_result_t::invalid_eof:
      return completion_result_t::invalid_eof;
    case isysalgo::exchange_result_t::none:
    default:
      return completion_result_t::none;
  }
}

std::uint8_t type_width_bits(const std::uint8_t type) {
  switch (type) {
    case TYPE_D:
      return 1U;
    case TYPE_A:
      return 8U;
    case TYPE_AP:
      return 16U;
    default:
      return 0U;
  }
}

const node_layout_t* find_node_by_target(const control::target_t target) {
  for (const auto& node : G_NODES) {
    if (node.target == target) {
      return &node;
    }
  }
  return nullptr;
}

std::size_t node_slot(const node_layout_t& node) {
  return static_cast<std::size_t>(&node - G_NODES.data());
}

bool command_matches_node(const node_layout_t& node, const std::uint16_t command_id) {
  if ((command_id < Param_max) || (command_id >= Param_Comm_max)) {
    return false;
  }

  const std::uint16_t comm_index = static_cast<std::uint16_t>(command_id - Param_max);
  return static_cast<std::uint8_t>(S_basecommtab[comm_index].sys_id) == node.system_id;
}

std::size_t find_cs_command_base_block(const std::uint8_t system_id) {
  bool found = false;
  std::size_t base = 0;
  for (std::size_t i = 0; i < Comm_max; ++i) {
    const auto& item = S_basecommtab[i];
    if (static_cast<std::uint8_t>(item.sys_id) != system_id) {
      continue;
    }

    const auto block = static_cast<std::size_t>(static_cast<unsigned char>(item.msg_cs_block_n));
    if (!found || block < base) {
      base = block;
      found = true;
    }
  }
  return found ? base : 0U;
}

std::size_t find_cs_param_base_block(const std::uint8_t system_id) {
  bool found = false;
  std::size_t base = 0;
  for (std::size_t i = 0; i < Param_max; ++i) {
    const auto& item = S_baseparamtab[i];
    if (static_cast<std::uint8_t>(item.sys_id) != system_id) {
      continue;
    }

    const auto block = static_cast<std::size_t>(static_cast<unsigned char>(item.msg_cs_block_n));
    if (!found || block < base) {
      base = block;
      found = true;
    }
  }
  return found ? base : 0U;
}

void init_node_layouts() {
  for (auto& node : G_NODES) {
    node.com_cs_base_block = find_cs_command_base_block(node.system_id);
    node.par_cs_base_block = find_cs_param_base_block(node.system_id);
  }
}

bool window_fits(const std::size_t offset, const std::size_t blocks, const std::size_t capacity) {
  return (offset <= capacity) && (blocks <= (capacity - offset));
}

void clear_task_slot(const std::size_t slot) {
  if (slot >= G_TASKS.size()) {
    return;
  }
  G_TASKS[slot] = task_t{};
}

void remove_slot_from_queue(std::deque<std::size_t>& queue, const std::size_t slot) {
  for (auto it = queue.begin(); it != queue.end(); ++it) {
    if (*it == slot) {
      queue.erase(it);
      return;
    }
  }
}

void remove_slot_from_queues(const std::size_t slot) {
  remove_slot_from_queue(G_NORMAL_QUEUE, slot);
  remove_slot_from_queue(G_PRIORITY_QUEUE, slot);
  if (slot < G_TASKS.size()) {
    G_TASKS[slot].queued = false;
  }
}

std::size_t alloc_task_slot() {
  for (std::size_t i = 0; i < G_TASKS.size(); ++i) {
    if (!G_TASKS[i].used) {
      return i;
    }
  }
  return k_invalid_slot;
}

void queue_task_slot(const std::size_t slot) {
  if (slot >= G_TASKS.size()) {
    return;
  }

  auto& task = G_TASKS[slot];
  if (!task.used || task.queued || task.active || !task.enabled) {
    return;
  }

  if (task.queue == task_queue_t::priority) {
    G_PRIORITY_QUEUE.push_back(slot);
  } else {
    G_NORMAL_QUEUE.push_back(slot);
  }
  task.queued = true;
}

std::size_t pop_next_task_slot() {
  auto pop_valid = [](std::deque<std::size_t>& queue) -> std::size_t {
    while (!queue.empty()) {
      const std::size_t slot = queue.front();
      queue.pop_front();
      if (slot >= G_TASKS.size()) {
        continue;
      }

      auto& task = G_TASKS[slot];
      task.queued = false;
      if (task.used && task.enabled) {
        return slot;
      }
    }
    return k_invalid_slot;
  };

  const bool have_priority = !G_PRIORITY_QUEUE.empty();
  const bool have_normal = !G_NORMAL_QUEUE.empty();

  if (have_priority && (!have_normal || (G_PRIORITY_BURST < k_priority_burst_limit))) {
    const std::size_t priority_slot = pop_valid(G_PRIORITY_QUEUE);
    if (priority_slot != k_invalid_slot) {
      ++G_PRIORITY_BURST;
      return priority_slot;
    }
  }

  const std::size_t normal_slot = pop_valid(G_NORMAL_QUEUE);
  if (normal_slot != k_invalid_slot) {
    G_PRIORITY_BURST = 0U;
    return normal_slot;
  }

  if (have_priority) {
    const std::size_t priority_slot = pop_valid(G_PRIORITY_QUEUE);
    if (priority_slot != k_invalid_slot) {
      ++G_PRIORITY_BURST;
      return priority_slot;
    }
  }

  G_PRIORITY_BURST = 0U;
  return k_invalid_slot;
}

task_queue_t to_task_queue(const queue_class_t queue) {
  return (queue == queue_class_t::priority) ? task_queue_t::priority : task_queue_t::normal;
}

task_id_t add_task(const node_layout_t& node,
                   const task_kind_t kind,
                   const task_queue_t queue_kind,
                   const bool periodic,
                   const std::uint16_t command_id,
                   const float command_value,
                   const bool has_value,
                   const bool use_staged_window) {
  const std::size_t slot = alloc_task_slot();
  if (slot == k_invalid_slot) {
    log_warning("cs scheduler: task queue is full");
    return k_invalid_task_id;
  }

  auto& task = G_TASKS[slot];
  task.used = true;
  task.queued = false;
  task.active = false;
  task.exchange_armed = false;
  task.periodic = periodic;
  task.enabled = true;
  task.id = G_NEXT_TASK_ID++;
  task.kind = kind;
  task.queue = queue_kind;
  task.node = &node;
  task.command_id = command_id;
  task.command_value = command_value;
  task.has_value = has_value;
  task.use_staged_window = use_staged_window;
  queue_task_slot(slot);
  return task.id;
}

task_id_t add_staged_command_task(const node_layout_t& node, const task_queue_t queue_kind) {
  return add_task(node, task_kind_t::command, queue_kind, false, 0U, 1.0F, false, true);
}

std::size_t remove_tasks_for_node(const node_layout_t& node) {
  std::size_t removed = 0U;
  for (std::size_t slot = 0; slot < G_TASKS.size(); ++slot) {
    auto& task = G_TASKS[slot];
    if (!task.used || (task.node != &node)) {
      continue;
    }

    if (slot == G_ACTIVE_TASK_SLOT) {
      task.enabled = false;
      task.periodic = false;
      ++removed;
      continue;
    }

    remove_slot_from_queues(slot);
    clear_task_slot(slot);
    ++removed;
  }
  return removed;
}

std::size_t find_task_slot_by_id(const task_id_t task_id) {
  if (task_id == k_invalid_task_id) {
    return k_invalid_slot;
  }

  for (std::size_t slot = 0; slot < G_TASKS.size(); ++slot) {
    if (G_TASKS[slot].used && (G_TASKS[slot].id == task_id)) {
      return slot;
    }
  }
  return k_invalid_slot;
}

std::size_t find_periodic_data_task_slot(const node_layout_t& node) {
  for (std::size_t slot = 0; slot < G_TASKS.size(); ++slot) {
    const auto& task = G_TASKS[slot];
    if (!task.used || !task.periodic || (task.kind != task_kind_t::data_request) || (task.node != &node)) {
      continue;
    }
    return slot;
  }
  return k_invalid_slot;
}

void update_task_queue_class(const std::size_t slot, const task_queue_t queue_kind) {
  if (slot >= G_TASKS.size()) {
    return;
  }

  auto& task = G_TASKS[slot];
  if (!task.used || (task.queue == queue_kind)) {
    return;
  }

  if (task.queued) {
    remove_slot_from_queues(slot);
  }

  task.queue = queue_kind;
  if (!task.active && task.enabled) {
    queue_task_slot(slot);
  }
}

void clear_command_msg_window(const node_layout_t& node) {
  if (G_CTX.msg_cmd == nullptr) {
    return;
  }
  if (!window_fits(node.com_cs_base_block, node.com_blocks, G_CTX.msg_cmd_blocks)) {
    return;
  }

  isys::ISYSCLEAR(G_CTX.msg_cmd + node.com_cs_base_block, node.com_blocks);
}

void clear_command_buffer_window(const node_layout_t& node) {
  if (G_CTX.msg_cmd_buf == nullptr) {
    return;
  }
  if (!window_fits(0U, node.com_blocks, G_CTX.msg_cmd_buf_blocks)) {
    return;
  }

  isys::ISYSCLEAR(G_CTX.msg_cmd_buf, node.com_blocks);
}

void clear_pending_command_window(const node_layout_t& node) {
  const auto slot = node_slot(node);
  if (slot >= G_PENDING_CMD.size()) {
    return;
  }
  isys::ISYSCLEAR(G_PENDING_CMD[slot].data(), node.com_blocks);
}

void load_pending_command_window(const node_layout_t& node) {
  const auto slot = node_slot(node);
  if ((slot >= G_PENDING_CMD.size()) || (G_CTX.msg_cmd_buf == nullptr)) {
    return;
  }
  if (!window_fits(0U, node.com_blocks, G_CTX.msg_cmd_buf_blocks)) {
    return;
  }

  imsgblock::copy_blocks(G_CTX.msg_cmd_buf,
                         G_CTX.msg_cmd_buf_blocks,
                         0,
                         G_PENDING_CMD[slot].data(),
                         G_PENDING_CMD[slot].size(),
                         0,
                         node.com_blocks);
}

void save_pending_command_window(const node_layout_t& node) {
  const auto slot = node_slot(node);
  if ((slot >= G_PENDING_CMD.size()) || (G_CTX.msg_cmd_buf == nullptr)) {
    return;
  }
  if (!window_fits(0U, node.com_blocks, G_CTX.msg_cmd_buf_blocks)) {
    return;
  }

  imsgblock::copy_blocks(G_PENDING_CMD[slot].data(),
                         G_PENDING_CMD[slot].size(),
                         0,
                         G_CTX.msg_cmd_buf,
                         G_CTX.msg_cmd_buf_blocks,
                         0,
                         node.com_blocks);
}

void merge_command_window_to_buffer(const node_layout_t& node) {
  if ((G_CTX.msg_cmd == nullptr) || (G_CTX.msg_cmd_buf == nullptr)) {
    return;
  }

  for (std::size_t i = 0; i < Comm_max; ++i) {
    const auto& item = S_basecommtab[i];
    if (static_cast<std::uint8_t>(item.sys_id) != node.system_id) {
      continue;
    }

    const auto width_bits = type_width_bits(static_cast<std::uint8_t>(item.type));
    if (width_bits == 0U) {
      continue;
    }

    const auto block_n = static_cast<std::size_t>(static_cast<unsigned char>(item.msg_cs_block_n));
    const auto block_offset = static_cast<std::uint8_t>(item.msg_block_offset);

    std::uint32_t raw_value = 0U;
    if (!imsgblock::read_value(G_CTX.msg_cmd, G_CTX.msg_cmd_blocks, block_n, block_offset, width_bits, raw_value)) {
      continue;
    }

    if (raw_value == 0U) {
      continue;
    }

    (void)imsgblock::write_value(G_CTX.msg_cmd_buf,
                                 G_CTX.msg_cmd_buf_blocks,
                                 static_cast<std::size_t>(static_cast<unsigned char>(item.msg_block_n)),
                                 block_offset,
                                 width_bits,
                                 raw_value);
  }
}

task_completion_t finalize_active_task(isysalgo::bus_state_t& bus) {
  if (G_ACTIVE_TASK_SLOT == k_invalid_slot) {
    return {};
  }

  auto& task = G_TASKS[G_ACTIVE_TASK_SLOT];
  if (!task.exchange_armed) {
    return {};
  }
  isysalgo::exchange_result_t result = isysalgo::exchange_result_t::none;
  const bool have_result = isysalgo::IBUS_TAKE_RESULT(bus, result);
  if (!have_result) {
    return {};
  }
  task_completion_t completion{
      .valid = true,
      .id = task.id,
      .target = (task.node != nullptr) ? task.node->target : control::target_t::any,
      .target_name = (task.node != nullptr) ? task.node->name : "",
      .kind = task.kind,
      .periodic = task.periodic,
      .result = result,
      .have_result = have_result,
  };
  task.active = false;
  task.exchange_armed = false;
  isysalgo::IBUS_RESET_FRAME_LAYOUT(bus);

  if ((task.kind == task_kind_t::command) && (task.node != nullptr)) {
    clear_command_msg_window(*task.node);
    if (!task.enabled) {
      clear_command_buffer_window(*task.node);
      clear_pending_command_window(*task.node);
    } else if (result == isysalgo::exchange_result_t::success) {
      clear_command_buffer_window(*task.node);
      clear_pending_command_window(*task.node);
    } else {
      save_pending_command_window(*task.node);
    }
  }

  if (task.used && task.enabled && task.periodic) {
    queue_task_slot(G_ACTIVE_TASK_SLOT);
  } else {
    clear_task_slot(G_ACTIVE_TASK_SLOT);
  }

  G_ACTIVE_TASK_SLOT = k_invalid_slot;
  return completion;
}

void activate_task(const std::size_t slot) {
  if (slot >= G_TASKS.size()) {
    return;
  }

  auto& task = G_TASKS[slot];
  if (!task.used || !task.enabled || (task.node == nullptr)) {
    return;
  }
  if (task.active) {
    std::ostringstream oss;
    oss << "cs scheduler: attempted to start active task id=" << task.id << " target=" << task.node->name;
    log_warning(oss.str());
    return;
  }

  task.active = true;
  task.exchange_armed = false;
  G_ACTIVE_TASK_SLOT = slot;

  std::ostringstream oss;
  oss << "cs scheduler: start task id=" << task.id << " target=" << task.node->name
      << " kind=" << task_kind_to_string(task.kind);
  if (task.kind == task_kind_t::command) {
    oss << " staged_window=" << (task.use_staged_window ? 1 : 0);
  }
  log_info(oss.str());
}

void activate_next_task() {
  const std::size_t slot = pop_next_task_slot();
  if (slot == k_invalid_slot) {
    return;
  }
  activate_task(slot);
}

}  // namespace cs::scheduler::internal
