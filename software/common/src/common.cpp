#include "common.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>

namespace common::log {
namespace {

struct logger_state_t {
  bool initialized{false};
  level_t min_level{level_t::info};
  std::ofstream stream{};
};

thread_local logger_state_t g_logger{};

struct shared_logger_state_t {
  bool initialized{false};
  level_t min_level{level_t::info};
  std::ofstream stream{};
};

std::mutex g_shared_logger_mutex{};
std::unordered_map<std::string, shared_logger_state_t> g_shared_loggers{};

std::filesystem::path resolve_log_dir() {
  const auto cwd = std::filesystem::current_path();

  // Find repository root candidate that contains software/sim.
  for (auto p = cwd; !p.empty(); p = p.parent_path()) {
    const auto candidate = p / "software" / "sim";
    if (std::filesystem::exists(candidate) && std::filesystem::is_directory(candidate)) {
      return candidate / "log";
    }
    if (p == p.parent_path()) {
      break;
    }
  }

  // Fallback for cases where current dir is already software/sim.
  const std::filesystem::path root_variant = std::filesystem::path("software") / "sim";
  if (std::filesystem::exists(root_variant) && std::filesystem::is_directory(root_variant)) {
    return root_variant / "log";
  }

  // Last-resort local path.
  const std::filesystem::path local_variant = std::filesystem::path("sim");
  if (std::filesystem::exists(local_variant) && std::filesystem::is_directory(local_variant)) {
    return local_variant / "log";
  }

  return local_variant / "log";
}

std::string now_timestamp() {
  const auto now = std::chrono::system_clock::now();
  const auto sec_point = std::chrono::time_point_cast<std::chrono::seconds>(now);
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - sec_point).count();

  const std::time_t now_tt = std::chrono::system_clock::to_time_t(now);
  std::tm tm_local{};
#if defined(_WIN32)
  localtime_s(&tm_local, &now_tt);
#else
  localtime_r(&now_tt, &tm_local);
#endif

  std::ostringstream oss;
  oss << std::put_time(&tm_local, "%Y-%m-%d %H:%M:%S") << '.';
  oss << std::setfill('0') << std::setw(3) << ms;
  return oss.str();
}

bool level_allowed(const level_t level) {
  return static_cast<unsigned char>(level) >= static_cast<unsigned char>(g_logger.min_level);
}

bool level_allowed(const level_t level, const level_t min_level) {
  return static_cast<unsigned char>(level) >= static_cast<unsigned char>(min_level);
}

}  // namespace

bool init_for_module(const std::string_view module_name, const init_config_t config) {
  if (module_name.empty()) {
    return false;
  }

  const auto log_dir = resolve_log_dir();
  std::error_code ec{};
  std::filesystem::create_directories(log_dir, ec);
  if (ec) {
    return false;
  }

  std::string file_name(module_name);
  file_name += "_log.log";

  const auto path = log_dir / file_name;
  const auto mode = std::ios::out | (config.truncate_on_init ? std::ios::trunc : std::ios::app);
  g_logger.stream = std::ofstream(path, mode);
  if (!g_logger.stream.is_open()) {
    g_logger.initialized = false;
    return false;
  }

  g_logger.min_level = config.min_level;
  g_logger.initialized = true;
  return true;
}

bool is_initialized() {
  return g_logger.initialized;
}

void write(const level_t level, const std::string_view message) {
  if (!g_logger.initialized || !level_allowed(level)) {
    return;
  }

  g_logger.stream << now_timestamp() << ": " << to_string(level) << ": " << message << '\n';
  g_logger.stream.flush();
}

void write_for_module(const std::string_view module_name,
                      const level_t level,
                      const std::string_view message,
                      const init_config_t config) {
  if (module_name.empty()) {
    return;
  }

  std::lock_guard<std::mutex> lock(g_shared_logger_mutex);

  auto& logger = g_shared_loggers[std::string(module_name)];
  if (!logger.initialized) {
    const auto log_dir = resolve_log_dir();
    std::error_code ec{};
    std::filesystem::create_directories(log_dir, ec);
    if (ec) {
      return;
    }

    std::string file_name(module_name);
    file_name += "_log.log";

    const auto path = log_dir / file_name;
    const auto mode = std::ios::out | (config.truncate_on_init ? std::ios::trunc : std::ios::app);
    logger.stream = std::ofstream(path, mode);
    if (!logger.stream.is_open()) {
      logger.initialized = false;
      return;
    }

    logger.min_level = config.min_level;
    logger.initialized = true;
  }

  if (!logger.initialized || !level_allowed(level, logger.min_level)) {
    return;
  }

  logger.stream << now_timestamp() << ": " << to_string(level) << ": " << message << '\n';
  logger.stream.flush();
}

void info(const std::string_view message) {
  write(level_t::info, message);
}

void warning(const std::string_view message) {
  write(level_t::warning, message);
}

void error(const std::string_view message) {
  write(level_t::error, message);
}

const char* to_string(const level_t level) {
  switch (level) {
    case level_t::info:
      return "info";
    case level_t::warning:
      return "warning";
    case level_t::error:
      return "error";
    default:
      return "unknown";
  }
}

}  // namespace common::log
