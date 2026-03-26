#pragma once

#include <cstdint>
#include <string_view>

#include "i_control_console.hpp"

namespace cs::control_bridge::internal {

void log_info(const std::string& message);
void log_warning(const std::string& message);
bool iequals_ascii(std::string_view lhs, std::string_view rhs);
std::uint8_t system_id_for_target(control::target_t target);
std::uint16_t find_command_id(control::target_t target, std::string_view key);
void log_command(const control::command_t& command);
void handle_exchange_command(const control::command_t& command);
void handle_enable_command(const control::command_t& command);
void handle_disable_command(const control::command_t& command);
void handle_cmd_command(const control::command_t& command);
void handle_stagecmd_command(const control::command_t& command);
void handle_sendcmd_command(const control::command_t& command);
void handle_focus_command(const control::command_t& command);
void handle_nominal_command();
void handle_status_command(const control::command_t& command);

}  // namespace cs::control_bridge::internal
