#pragma once

#include <cstdint>
#include <string>

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
};

void start(config_t cfg = {});
void stop();
bool is_running();

bool pop_command(target_t target, command_t& out);
void clear(target_t target = target_t::any);

const char* target_to_string(target_t target);

}  // namespace control
