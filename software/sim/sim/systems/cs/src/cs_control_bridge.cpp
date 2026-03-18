#include "cs_control_bridge.hpp"

#include <cctype>
#include <cstdint>
#include <sstream>
#include <string_view>

#include "../../base/command_tab.hpp"
#include "cs_operating_mode.hpp"
#include "cs_scheduler.hpp"
#include "common.hpp"
#include "i_control_console.hpp"

namespace cs::control_bridge {
namespace {

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

}  // namespace

void step() {
  control::command_t command{};
  while (control::pop_command(control::target_t::cs, command)) {
    log_command(command);

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

    if (command.verb == "status") {
      handle_status_command(command);
      continue;
    }

    if (command.verb == "cmd") {
      handle_cmd_command(command);
      continue;
    }

    if (command.verb == "help") {
      log_info(
          "control: supported cs commands: exchange <target>, enable <target>, disable <target>, focus <target|clear>, nominal, status [target], cmd <target> <KEY> [value=N]");
      continue;
    }

    log_warning("control: unsupported cs command: " + command.raw);
  }
}

}  // namespace cs::control_bridge
