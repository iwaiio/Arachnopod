#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace control {

enum class target_t : std::uint8_t {
  any = 0,
  cs,
  pss,
  tcs,
  tms,
  mns,
  ls,
  count,
};

struct command_t {
  target_t target{target_t::any};
  std::string verb;
  std::string arg0;
  std::string arg1;
  std::int32_t value{0};
  bool has_value{false};
  std::string raw;
};

struct config_t {
  bool enabled{true};
  bool echo{false};
  bool console_enabled{true};
  bool file_enabled{true};
};

void start(config_t cfg = {});
void stop();
bool is_running();

bool pop_command(target_t target, command_t& out);
void clear(target_t target = target_t::any);

bool try_parse_target(std::string_view token, target_t& out);
const char* target_to_string(target_t target);

}  // namespace control
