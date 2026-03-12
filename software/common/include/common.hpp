#pragma once

#include <string_view>

namespace common::log {

enum class level_t : unsigned char {
  info = 0,
  warning = 1,
  error = 2,
};

struct init_config_t {
  level_t min_level{level_t::info};
  bool truncate_on_init{true};
};

// Initialize thread-local logger for a module.
// Creates log file: sim/log/<module>_log.log (or software/sim/log/... in repo root).
bool init_for_module(std::string_view module_name, init_config_t config = {});

bool is_initialized();
void write(level_t level, std::string_view message);

// Shared (cross-thread) module logger.
// Writes directly to <module_name>_log.log using a global synchronized backend.
void write_for_module(std::string_view module_name,
                      level_t level,
                      std::string_view message,
                      init_config_t config = {});

void info(std::string_view message);
void warning(std::string_view message);
void error(std::string_view message);

const char* to_string(level_t level);

}  // namespace common::log
