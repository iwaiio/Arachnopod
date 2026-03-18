#include "cs_scheduler.hpp"

#include <array>
#include <cstdint>
#include <deque>
#include <sstream>

#include "../../base/command_tab.hpp"
#include "../../base/param_tab.hpp"
#include "common.hpp"
#include "i_sys.hpp"
#include "i_sys_algo.hpp"

namespace cs::scheduler {
namespace {

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
  bool periodic{false};
  bool enabled{false};
  std::uint32_t id{0};
  task_kind_t kind{task_kind_t::data_request};
  task_queue_t queue{task_queue_t::normal};
  const node_layout_t* node{nullptr};
  std::uint16_t command_id{0};
  std::int32_t command_value{1};
  bool has_value{false};
};

struct task_completion_t {
  bool valid{false};
  std::uint32_t id{0};
  control::target_t target{control::target_t::any};
  const char* target_name{""};
  task_kind_t kind{task_kind_t::data_request};
  bool periodic{false};
  isysalgo::exchange_result_t result{isysalgo::exchange_result_t::none};
  bool have_result{false};
};

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

const node_layout_t* find_node_by_target(const control::target_t target) {
  for (const auto& node : G_NODES) {
    if (node.target == target) {
      return &node;
    }
  }
  return nullptr;
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
                   const std::uint16_t command_id = 0,
                   const std::int32_t command_value = 1,
                   const bool has_value = false) {
  const std::size_t slot = alloc_task_slot();
  if (slot == k_invalid_slot) {
    log_warning("cs scheduler: task queue is full");
    return k_invalid_task_id;
  }

  auto& task = G_TASKS[slot];
  task.used = true;
  task.queued = false;
  task.active = false;
  task.periodic = periodic;
  task.enabled = true;
  task.id = G_NEXT_TASK_ID++;
  task.kind = kind;
  task.queue = queue_kind;
  task.node = &node;
  task.command_id = command_id;
  task.command_value = command_value;
  task.has_value = has_value;
  queue_task_slot(slot);
  return task.id;
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

void clear_command_window(const node_layout_t& node) {
  if ((G_CTX.msg_com == nullptr) || (G_CTX.msg_com_buf == nullptr)) {
    return;
  }
  if (!window_fits(node.com_cs_base_block, node.com_blocks, G_CTX.msg_com_blocks) ||
      !window_fits(node.com_cs_base_block, node.com_blocks, G_CTX.msg_com_buf_blocks)) {
    return;
  }

  isys::ISYSCLEAR(G_CTX.msg_com + node.com_cs_base_block, node.com_blocks);
  isys::ISYSCLEAR(G_CTX.msg_com_buf + node.com_cs_base_block, node.com_blocks);
}

task_completion_t finalize_active_task(isysalgo::bus_state_t& bus) {
  if (G_ACTIVE_TASK_SLOT == k_invalid_slot) {
    return {};
  }

  auto& task = G_TASKS[G_ACTIVE_TASK_SLOT];
  isysalgo::exchange_result_t result = isysalgo::exchange_result_t::none;
  const bool have_result = isysalgo::IBUS_TAKE_RESULT(bus, result);
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
  isysalgo::IBUS_RESET_FRAME_LAYOUT(bus);

  if ((task.kind == task_kind_t::command) && (task.node != nullptr)) {
    clear_command_window(*task.node);
  }

  if (task.used && task.enabled && task.periodic) {
    queue_task_slot(G_ACTIVE_TASK_SLOT);
  } else {
    clear_task_slot(G_ACTIVE_TASK_SLOT);
  }

  G_ACTIVE_TASK_SLOT = k_invalid_slot;
  return completion;
}

void prepare_command_task(const task_t& task, isysalgo::bus_state_t& bus) {
  if ((task.node == nullptr) || (G_CTX.ipar_ctx == nullptr)) {
    return;
  }

  clear_command_window(*task.node);
  (void)isysalgo::IBUS_SET_COM_FRAME(bus, task.node->com_cs_base_block, task.node->com_blocks);
  (void)isysalgo::IBUS_SET_PAR_FRAME(bus, task.node->par_cs_base_block, task.node->par_blocks);
  (void)ipar::IGEN(*G_CTX.ipar_ctx, task.command_id, task.has_value ? task.command_value : 1);
  isysalgo::ISYS_SYNC_MSGCOM_TO_BUF(bus);
}

void prepare_data_task(const task_t& task, isysalgo::bus_state_t& bus) {
  if (task.node == nullptr) {
    return;
  }

  (void)isysalgo::IBUS_SET_COM_FRAME(bus, task.node->com_cs_base_block, task.node->com_blocks);
  (void)isysalgo::IBUS_SET_PAR_FRAME(bus, task.node->par_cs_base_block, task.node->par_blocks);
}

void start_task(const std::size_t slot, isysalgo::bus_state_t& bus) {
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

  isysalgo::IBUS_RESET_FRAME_LAYOUT(bus);
  if (task.kind == task_kind_t::command) {
    prepare_command_task(task, bus);
    bus.msg_flag = ARP_FLAG_CMD_REQ;
  } else {
    prepare_data_task(task, bus);
    bus.msg_flag = ARP_FLAG_DATA_REQ;
  }

  bus.target_addr = task.node->bus_addr;
  bus.exchange_flag = isysalgo::exchange_flag_t::tx;
  bus.tx_flag = isysalgo::tx_flag_t::tx_sof;
  bus.rx_flag = isysalgo::rx_flag_t::rx_sof;
  bus.msg_adr_flag = false;
  bus.msg_n_blocks = 0;
  bus.exchange_sub_clock = 0;
  bus.exchange_wait_clock = 0;
  isysalgo::IBUS_CLEAR_RESULT(bus);

  task.active = true;
  G_ACTIVE_TASK_SLOT = slot;

  std::ostringstream oss;
  oss << "cs scheduler: start task id=" << task.id << " target=" << task.node->name
      << " kind=" << task_kind_to_string(task.kind);
  log_info(oss.str());
}

void start_next_task(isysalgo::bus_state_t& bus) {
  const std::size_t slot = pop_next_task_slot();
  if (slot == k_invalid_slot) {
    return;
  }
  start_task(slot, bus);
}

}  // namespace

void init(const context_t& ctx) {
  G_CTX = ctx;
  init_node_layouts();
  G_READY = true;
}

void reset() {
  G_TASKS = {};
  G_NORMAL_QUEUE.clear();
  G_PRIORITY_QUEUE.clear();
  G_COMPLETIONS.clear();
  G_ACTIVE_TASK_SLOT = k_invalid_slot;
  G_NEXT_TASK_ID = 1U;
  G_PRIORITY_BURST = 0U;
}

task_id_t enqueue_exchange(const control::target_t target,
                           const queue_class_t queue,
                           const bool periodic) {
  const auto* node = find_node_by_target(target);
  if (node == nullptr) {
    return k_invalid_task_id;
  }

  return add_task(*node, task_kind_t::data_request, to_task_queue(queue), periodic);
}

task_id_t ensure_periodic_exchange(const control::target_t target, const queue_class_t queue) {
  const auto* node = find_node_by_target(target);
  if (node == nullptr) {
    return k_invalid_task_id;
  }

  const std::size_t existing_slot = find_periodic_data_task_slot(*node);
  if (existing_slot != k_invalid_slot) {
    auto& task = G_TASKS[existing_slot];
    task.enabled = true;
    update_task_queue_class(existing_slot, to_task_queue(queue));
    return task.id;
  }

  return add_task(*node, task_kind_t::data_request, to_task_queue(queue), true);
}

task_id_t enqueue_command(const control::target_t target,
                          const std::uint16_t command_id,
                          const std::int32_t command_value,
                          const bool has_value,
                          const queue_class_t queue) {
  const auto* node = find_node_by_target(target);
  if (node == nullptr) {
    return k_invalid_task_id;
  }

  return add_task(*node,
                  task_kind_t::command,
                  to_task_queue(queue),
                  false,
                  command_id,
                  command_value,
                  has_value);
}

bool cancel_periodic_exchange(const control::target_t target) {
  const auto* node = find_node_by_target(target);
  if (node == nullptr) {
    return false;
  }

  const std::size_t slot = find_periodic_data_task_slot(*node);
  if (slot == k_invalid_slot) {
    return false;
  }

  auto& task = G_TASKS[slot];
  if (slot == G_ACTIVE_TASK_SLOT) {
    task.enabled = false;
    task.periodic = false;
    return true;
  }

  remove_slot_from_queues(slot);
  clear_task_slot(slot);
  return true;
}

bool cancel_task(const task_id_t task_id) {
  const std::size_t slot = find_task_slot_by_id(task_id);
  if (slot == k_invalid_slot) {
    return false;
  }

  auto& task = G_TASKS[slot];
  if (slot == G_ACTIVE_TASK_SLOT) {
    task.enabled = false;
    task.periodic = false;
    return true;
  }

  remove_slot_from_queues(slot);
  clear_task_slot(slot);
  return true;
}

std::size_t cancel_target(const control::target_t target) {
  const auto* node = find_node_by_target(target);
  if (node == nullptr) {
    return 0U;
  }

  return remove_tasks_for_node(*node);
}

bool try_pop_completion(completion_t& out) {
  if (G_COMPLETIONS.empty()) {
    return false;
  }

  out = G_COMPLETIONS.front();
  G_COMPLETIONS.pop_front();
  return true;
}

void exchange_control(isysalgo::bus_state_t& bus) {
  if (!G_READY) {
    return;
  }

  const auto completion = finalize_active_task(bus);
  if (completion.valid) {
    G_COMPLETIONS.push_back(completion_t{
        .valid = true,
        .target = completion.target,
        .kind = to_public_completion_kind(completion.kind),
        .periodic = completion.periodic,
        .have_result = completion.have_result,
        .result = to_public_completion_result(completion.result),
    });

    std::ostringstream oss;
    oss << "cs scheduler: finish task id=" << completion.id << " target=" << completion.target_name
        << " kind=" << task_kind_to_string(completion.kind)
        << " result=" << isysalgo::IBUS_RESULT_TO_STRING(completion.result);
    if (!completion.have_result) {
      oss << " pending_result=0";
    }
    log_info(oss.str());
  }
  start_next_task(bus);
}

}  // namespace cs::scheduler
