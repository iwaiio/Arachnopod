#include "cs_control_bridge.hpp"

#include "cs_control_internal.hpp"

namespace cs::control_bridge {

void step() {
  control::command_t command{};
  while (control::pop_command(control::target_t::cs, command)) {
    internal::log_command(command);

    if ((command.verb == "exchange") || (command.verb == "ex")) {
      internal::handle_exchange_command(command);
      continue;
    }

    if (command.verb == "enable") {
      internal::handle_enable_command(command);
      continue;
    }

    if (command.verb == "disable") {
      internal::handle_disable_command(command);
      continue;
    }

    if (command.verb == "focus") {
      internal::handle_focus_command(command);
      continue;
    }

    if (command.verb == "nominal") {
      internal::handle_nominal_command();
      continue;
    }

    if (command.verb == "status") {
      internal::handle_status_command(command);
      continue;
    }

    if (command.verb == "cmd") {
      internal::handle_cmd_command(command);
      continue;
    }

    if (command.verb == "stagecmd") {
      internal::handle_stagecmd_command(command);
      continue;
    }

    if ((command.verb == "sendcmd") || (command.verb == "sendcmdbuf")) {
      internal::handle_sendcmd_command(command);
      continue;
    }

    if (command.verb == "help") {
      internal::log_info(
          "control: supported cs commands: exchange <target>, enable <target>, disable <target>, focus <target|clear>, nominal, status [target], cmd <target> <KEY> [value=N], stagecmd <target> <KEY> [value=N], sendcmd <target>");
      continue;
    }

    internal::log_warning("control: unsupported cs command: " + command.raw);
  }
}

}  // namespace cs::control_bridge
