#include "cs_control_internal.hpp"

#include <cctype>
#include <cstdint>
#include <sstream>
#include <string_view>

#include "../../base/command_tab.hpp"
#include "common.hpp"
#include "cs.hpp"
#include "cs_operating_mode.hpp"
#include "cs_scheduler.hpp"
#include "cs_scheduler_internal.hpp"

namespace cs::control_bridge::internal {

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

std::uint8_t system_id_for_target(const control::target_t target) {
  switch (target) {
    case control::target_t::pss:
      return SYS_PSS;
    case control::target_t::tcs:
      return SYS_TCS;
    case control::target_t::tms:
      return SYS_TMS;
    case control::target_t::mns:
      return SYS_MNS;
    case control::target_t::ls:
      return SYS_LS;
    default:
      return SYS_NONE;
  }
}

std::uint16_t find_command_id(const control::target_t target, const std::string_view key) {
  const std::uint8_t system_id = system_id_for_target(target);
  if (system_id == SYS_NONE) {
    return 0U;
  }

  for (std::size_t i = 0; i < Comm_max; ++i) {
    const auto& entry = S_basecommtab[i];
    if (static_cast<std::uint8_t>(entry.sys_id) != system_id) {
      continue;
    }
    if ((entry.key != nullptr) && iequals_ascii(entry.key, key)) {
      return static_cast<std::uint16_t>(Param_max + i);
    }
  }

  return 0U;
}

void log_command(const control::command_t& command) {
  std::ostringstream oss;
  oss << "cs control: verb=" << command.verb;
  if (!command.arg0.empty()) {
    oss << " arg0=" << command.arg0;
  }
  if (!command.arg1.empty()) {
    oss << " arg1=" << command.arg1;
  }
  if (command.has_value) {
    oss << " value=" << command.value;
  }
  log_info(oss.str());
}

void handle_exchange_command(const control::command_t& command) {
  control::target_t target = control::target_t::any;
  if (!control::try_parse_target(command.arg0, target)) {
    log_warning("control: unknown exchange target: " + command.arg0);
    return;
  }

  if (scheduler::enqueue_exchange(target) == 0U) {
    log_warning("control: unsupported exchange target: " + command.arg0);
  }
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

  if (command.arg1.empty()) {
    log_warning("control: cmd requires command key");
    return;
  }

  const std::uint16_t command_id = find_command_id(target, command.arg1);
  if (command_id == 0U) {
    log_warning("control: unknown command key: " + command.arg1);
    return;
  }

  if (scheduler::enqueue_command(target, command_id, command.value, command.has_value) == 0U) {
    log_warning("control: unsupported cmd target: " + command.arg0);
  }
}

void handle_stagecmd_command(const control::command_t& command) {
  control::target_t target = control::target_t::any;
  if (!control::try_parse_target(command.arg0, target)) {
    log_warning("control: unknown stagecmd target: " + command.arg0);
    return;
  }

  if (command.arg1.empty()) {
    log_warning("control: stagecmd requires command key");
    return;
  }

  const std::uint16_t command_id = find_command_id(target, command.arg1);
  if (command_id == 0U) {
    log_warning("control: unknown command key: " + command.arg1);
    return;
  }

  const auto status = ipar::IMSGSET(cs::imsg_context(), command_id, command.has_value ? command.value : 1.0F);
  if (status != ipar::status_t::ok) {
    std::ostringstream oss;
    oss << "control: failed to stage command key=" << command.arg1 << " status=" << ipar::to_string(status);
    log_warning(oss.str());
    return;
  }

  std::ostringstream oss;
  oss << "control: staged command target=" << command.arg0 << " key=" << command.arg1;
  if (command.has_value) {
    oss << " value=" << command.value;
  }
  log_info(oss.str());
}

void handle_sendcmd_command(const control::command_t& command) {
  control::target_t target = control::target_t::any;
  if (!control::try_parse_target(command.arg0, target)) {
    log_warning("control: unknown sendcmd target: " + command.arg0);
    return;
  }

  const auto* node = scheduler::internal::find_node_by_target(target);
  if (node == nullptr) {
    log_warning("control: unsupported sendcmd target: " + command.arg0);
    return;
  }

  if (scheduler::internal::add_staged_command_task(*node, scheduler::internal::task_queue_t::normal) == 0U) {
    log_warning("control: failed to queue sendcmd for target: " + command.arg0);
    return;
  }

  log_info("control: queued staged command window for " + command.arg0);
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

void handle_status_command(const control::command_t& command) {
  if (command.arg0.empty()) {
    operating_mode::log_status();
    return;
  }

  control::target_t target = control::target_t::any;
  if (!control::try_parse_target(command.arg0, target)) {
    log_warning("control: unknown status target: " + command.arg0);
    return;
  }

  operating_mode::log_status(target);
}

}  // namespace cs::control_bridge::internal
