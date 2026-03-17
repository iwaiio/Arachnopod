#include "cs_scheduler.hpp"

#include <array>
#include <cctype>
#include <cstdint>
#include <deque>
#include <sstream>
#include <string_view>

#include "../../base/command_tab.hpp"
#include "../../base/param_tab.hpp"
#include "cs_operating_mode.hpp"
#include "common.hpp"
#include "i_control_console.hpp"
#include "i_sys.hpp"
#include "i_sys_algo.hpp"

namespace cs::scheduler {
namespace {

constexpr std::size_t k_max_tasks = 32U;
constexpr std::size_t k_invalid_slot = static_cast<std::size_t>(-1);
constexpr task_id_t k_invalid_task_id = 0U;

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

void log_info(const std::string& message) {
  common::log::info(message);
}

void log_warning(const std::string& message) {
  common::log::warning(message);
}

bool iequals_ascii(const std::string_view lhs, const std::string_view rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }

  for (std::size_t i = 0; i < lhs.size(); ++i) {
    const auto l = static_cast<unsigned char>(lhs[i]);
    const auto r = static_cast<unsigned char>(rhs[i]);
    if (std::tolower(l) != std::tolower(r)) {
      return false;
    }
  }

  return true;
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

  const std::size_t priority_slot = pop_valid(G_PRIORITY_QUEUE);
  if (priority_slot != k_invalid_slot) {
    return priority_slot;
  }

  return pop_valid(G_NORMAL_QUEUE);
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

std::uint16_t find_command_id_by_key(const node_layout_t& node, const std::string_view key) {
  for (std::size_t i = 0; i < Comm_max; ++i) {
    const auto& entry = S_basecommtab[i];
    if (static_cast<std::uint8_t>(entry.sys_id) != node.system_id) {
      continue;
    }
    if ((entry.key != nullptr) && iequals_ascii(entry.key, key)) {
      return static_cast<std::uint16_t>(Param_max + i);
    }
  }
  return 0U;
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

void finalize_active_task(isysalgo::bus_state_t& bus) {
  if (G_ACTIVE_TASK_SLOT == k_invalid_slot) {
    return;
  }

  auto& task = G_TASKS[G_ACTIVE_TASK_SLOT];
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

  task.active = true;
  G_ACTIVE_TASK_SLOT = slot;

  std::ostringstream oss;
  oss << "cs scheduler: start task id=" << task.id << " target=" << task.node->name
      << " kind=" << ((task.kind == task_kind_t::command) ? "cmd" : "data");
  log_info(oss.str());
}

void start_next_task(isysalgo::bus_state_t& bus) {
  const std::size_t slot = pop_next_task_slot();
  if (slot == k_invalid_slot) {
    return;
  }
  start_task(slot, bus);
}

void handle_exchange_command(const control::command_t& command) {
  control::target_t target = control::target_t::any;
  if (!control::try_parse_target(command.arg0, target)) {
    log_warning("control: unknown exchange target: " + command.arg0);
    return;
  }
  const auto* node = find_node_by_target(target);
  if (node == nullptr) {
    log_warning("control: unsupported exchange target: " + command.arg0);
    return;
  }

  (void)enqueue_exchange(target);
}

void handle_enable_command(const control::command_t& command) {
  control::target_t target = control::target_t::any;
  if (!control::try_parse_target(command.arg0, target)) {
    log_warning("control: unknown enable target: " + command.arg0);
    return;
  }
  if (!operating_mode::set_background_target_enabled(target, true)) {
    log_warning("control: unsupported enable target: " + command.arg0);
    return;
  }
  log_info("control: periodic exchange enabled for " + command.arg0);
}

void handle_disable_command(const control::command_t& command) {
  control::target_t target = control::target_t::any;
  if (!control::try_parse_target(command.arg0, target)) {
    log_warning("control: unknown disable target: " + command.arg0);
    return;
  }
  if (!operating_mode::set_background_target_enabled(target, false)) {
    log_warning("control: unsupported disable target: " + command.arg0);
    return;
  }
  log_info("control: periodic exchange disabled for " + command.arg0);
}

void handle_cmd_command(const control::command_t& command) {
  control::target_t target = control::target_t::any;
  if (!control::try_parse_target(command.arg0, target)) {
    log_warning("control: unknown cmd target: " + command.arg0);
    return;
  }
  const auto* node = find_node_by_target(target);
  if (node == nullptr) {
    log_warning("control: unsupported cmd target: " + command.arg0);
    return;
  }

  if (command.arg1.empty()) {
    log_warning("control: cmd requires command key");
    return;
  }

  const std::uint16_t command_id = find_command_id_by_key(*node, command.arg1);
  if (command_id == 0U) {
    log_warning("control: unknown command key: " + command.arg1);
    return;
  }

  (void)enqueue_command(target, command_id, command.value, command.has_value);
}

void handle_focus_command(const control::command_t& command) {
  if (command.arg0.empty() || iequals_ascii(command.arg0, "clear") || iequals_ascii(command.arg0, "none")) {
    operating_mode::clear_focus_target();
    log_info("control: cs focus cleared");
    return;
  }

  control::target_t target = control::target_t::any;
  if (!control::try_parse_target(command.arg0, target)) {
    log_warning("control: unknown focus target: " + command.arg0);
    return;
  }

  if (!operating_mode::set_focus_target(target)) {
    log_warning("control: unsupported focus target: " + command.arg0);
    return;
  }

  log_info("control: cs focus set to " + command.arg0);
}

void handle_nominal_command() {
  operating_mode::clear_focus_target();
  log_info("control: cs nominal mode");
}

void drain_control_queue() {
  control::command_t command{};
  while (control::pop_command(control::target_t::cs, command)) {
    if ((command.verb == "exchange") || (command.verb == "ex")) {
      handle_exchange_command(command);
      continue;
    }

    if (command.verb == "enable") {
      handle_enable_command(command);
      continue;
    }

    if (command.verb == "disable") {
      handle_disable_command(command);
      continue;
    }

    if (command.verb == "focus") {
      handle_focus_command(command);
      continue;
    }

    if (command.verb == "nominal") {
      handle_nominal_command();
      continue;
    }

    if (command.verb == "cmd") {
      handle_cmd_command(command);
      continue;
    }

    if (command.verb == "help") {
      log_info("control: supported cs commands: exchange <target>, enable <target>, disable <target>, focus <target|clear>, nominal, cmd <target> <KEY> [value=N]");
      continue;
    }

    log_warning("control: unsupported cs command: " + command.raw);
  }
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
  G_ACTIVE_TASK_SLOT = k_invalid_slot;
  G_NEXT_TASK_ID = 1U;
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

void local_algorithm() {
  if (!G_READY) {
    return;
  }
  drain_control_queue();
}

void exchange_control(isysalgo::bus_state_t& bus) {
  if (!G_READY) {
    return;
  }

  finalize_active_task(bus);
  start_next_task(bus);
}

}  // namespace cs::scheduler
