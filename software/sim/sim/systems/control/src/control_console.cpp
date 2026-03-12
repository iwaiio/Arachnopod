#include "i_control_console.hpp"

#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "common.hpp"

namespace control {
namespace {

std::atomic<bool> G_RUNNING{false};
std::atomic<bool> G_STOP{false};
std::mutex G_MUTEX{};
std::array<std::deque<command_t>, static_cast<std::size_t>(target_t::count)> G_QUEUES{};

std::thread G_THREAD{};
std::thread G_FILE_THREAD{};
config_t G_CFG{};

void ensure_logger() {
  if (common::log::is_initialized()) {
    return;
  }
  const common::log::init_config_t cfg{
      .min_level = common::log::level_t::info,
      .truncate_on_init = true,
  };
  (void)common::log::init_for_module("control", cfg);
}

std::string to_lower_copy(std::string s) {
  for (char& ch : s) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return s;
}

std::vector<std::string> split_ws(const std::string& line) {
  std::istringstream iss(line);
  std::vector<std::string> out;
  std::string token;
  while (iss >> token) {
    out.push_back(token);
  }
  return out;
}

std::size_t target_index(const target_t target) {
  return static_cast<std::size_t>(target);
}

bool parse_int(const std::string& token, std::int32_t& out) {
  if (token.empty()) {
    return false;
  }
  char* end = nullptr;
  const long value = std::strtol(token.c_str(), &end, 0);
  if (end == token.c_str() || (end != nullptr && *end != '\0')) {
    return false;
  }
  out = static_cast<std::int32_t>(value);
  return true;
}

target_t parse_target(const std::string& token) {
  const std::string t = to_lower_copy(token);
  if (t == "cs") {
    return target_t::cs;
  }
  if (t == "pss") {
    return target_t::pss;
  }
  if (t == "tcs") {
    return target_t::tcs;
  }
  if (t == "tms") {
    return target_t::tms;
  }
  if (t == "mns") {
    return target_t::mns;
  }
  if (t == "ls") {
    return target_t::ls;
  }
  if (t == "any") {
    return target_t::any;
  }
  return target_t::any;
}

void enqueue_command(command_t&& cmd) {
  std::lock_guard<std::mutex> lock(G_MUTEX);
  G_QUEUES[target_index(cmd.target)].push_back(std::move(cmd));
}

bool parse_line(const std::string& line, command_t& out) {
  const auto tokens = split_ws(line);
  if (tokens.empty()) {
    return false;
  }

  std::size_t idx = 0;
  target_t target = target_t::cs;
  target_t first_token_target = parse_target(tokens[0]);
  if (first_token_target != target_t::any) {
    target = first_token_target;
    idx = 1;
  }

  if (idx >= tokens.size()) {
    return false;
  }

  const std::string verb = to_lower_copy(tokens[idx]);
  command_t cmd{};
  cmd.target = target;
  cmd.verb = verb;
  cmd.raw = line;

  if (verb == "help") {
    out = std::move(cmd);
    return true;
  }

  if (verb == "quit" || verb == "exit") {
    out = std::move(cmd);
    return true;
  }

  if (verb == "exchange" || verb == "ex") {
    if ((idx + 1) < tokens.size()) {
      cmd.arg0 = tokens[idx + 1];
    }
    out = std::move(cmd);
    return true;
  }

  if (verb == "enable" || verb == "disable") {
    if ((idx + 1) < tokens.size()) {
      cmd.arg0 = tokens[idx + 1];
    }
    out = std::move(cmd);
    return true;
  }

  if (verb == "cmd") {
    if ((idx + 1) < tokens.size()) {
      cmd.arg0 = tokens[idx + 1];
    }
    if ((idx + 2) < tokens.size()) {
      cmd.arg1 = tokens[idx + 2];
      if (cmd.arg1.rfind("id=", 0) == 0) {
        cmd.arg1 = cmd.arg1.substr(3);
      }
    }

    for (std::size_t i = idx + 3; i < tokens.size(); ++i) {
      const std::string& token = tokens[i];
      const auto eq_pos = token.find('=');
      if (eq_pos != std::string::npos) {
        const std::string key = to_lower_copy(token.substr(0, eq_pos));
        const std::string value = token.substr(eq_pos + 1);
        if (key == "value") {
          std::int32_t parsed = 0;
          if (parse_int(value, parsed)) {
            cmd.value = parsed;
            cmd.has_value = true;
          }
        } else if (key == "id") {
          cmd.arg1 = value;
        }
      } else {
        std::int32_t parsed = 0;
        if (parse_int(token, parsed)) {
          cmd.value = parsed;
          cmd.has_value = true;
        }
      }
    }

    out = std::move(cmd);
    return true;
  }

  return false;
}

void console_loop() {
  ensure_logger();

  common::log::info("control console started");

  std::string line;
  while (!G_STOP.load(std::memory_order_relaxed)) {
    if (!std::getline(std::cin, line)) {
      break;
    }

    if (line.empty()) {
      continue;
    }

    command_t cmd{};
    if (!parse_line(line, cmd)) {
      common::log::warning("control: ignored invalid command: " + line);
      continue;
    }

    if (cmd.verb == "quit" || cmd.verb == "exit") {
      G_STOP.store(true, std::memory_order_relaxed);
      break;
    }

    enqueue_command(std::move(cmd));

    if (G_CFG.echo) {
      common::log::info("control: " + line);
    }
  }

  common::log::info("control console stopped");
  G_RUNNING.store(false, std::memory_order_relaxed);
}

std::filesystem::path resolve_log_dir() {
  const auto cwd = std::filesystem::current_path();

  for (auto p = cwd; !p.empty(); p = p.parent_path()) {
    const auto candidate = p / "software" / "sim";
    std::error_code ec{};
    if (std::filesystem::exists(candidate, ec) && std::filesystem::is_directory(candidate, ec)) {
      return candidate / "log";
    }
    if (p == p.parent_path()) {
      break;
    }
  }

  const std::filesystem::path root_variant = std::filesystem::path("software") / "sim";
  std::error_code ec{};
  if (std::filesystem::exists(root_variant, ec) && std::filesystem::is_directory(root_variant, ec)) {
    return root_variant / "log";
  }

  const std::filesystem::path local_variant = std::filesystem::path("sim");
  if (std::filesystem::exists(local_variant, ec) && std::filesystem::is_directory(local_variant, ec)) {
    return local_variant / "log";
  }

  return local_variant / "log";
}

std::filesystem::path resolve_control_file() {
  const char* env = std::getenv("ARP_CONTROL_FILE");
  if (env != nullptr && *env != '\0') {
    const std::string raw(env);
    if (raw == "0") {
      return {};
    }
    return std::filesystem::path(raw);
  }

  return resolve_log_dir() / "control_in.txt";
}

void file_loop(const std::filesystem::path path) {
  ensure_logger();

  if (path.empty()) {
    return;
  }

  common::log::info("control file listener started: " + path.string());

  std::uintmax_t offset = 0;
  while (!G_STOP.load(std::memory_order_relaxed)) {
    std::error_code ec{};
    if (!std::filesystem::exists(path, ec)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    const std::uintmax_t size = std::filesystem::file_size(path, ec);
    if (ec) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    if (size < offset) {
      offset = 0;
    }

    if (size > offset) {
      std::ifstream stream(path, std::ios::in);
      if (stream.is_open()) {
        stream.seekg(static_cast<std::streamoff>(offset));
        std::string line;
        while (std::getline(stream, line)) {
          if (line.empty()) {
            continue;
          }

          command_t cmd{};
          if (!parse_line(line, cmd)) {
            common::log::warning("control: ignored invalid command (file): " + line);
            continue;
          }

          if (cmd.verb == "quit" || cmd.verb == "exit") {
            common::log::warning("control: quit ignored for file input");
            continue;
          }

          enqueue_command(std::move(cmd));
          if (G_CFG.echo) {
            common::log::info("control(file): " + line);
          }
        }

        const auto pos = stream.tellg();
        if (pos != std::streampos(-1)) {
          offset = static_cast<std::uintmax_t>(pos);
        } else {
          offset = size;
        }
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  common::log::info("control file listener stopped");
}

}  // namespace

void start(const config_t cfg) {
  if (!cfg.enabled) {
    return;
  }
  if (G_RUNNING.exchange(true, std::memory_order_relaxed)) {
    return;
  }

  G_CFG = cfg;
  G_STOP.store(false, std::memory_order_relaxed);
  G_THREAD = std::thread(console_loop);
  G_THREAD.detach();

  const auto control_file = resolve_control_file();
  if (!control_file.empty()) {
    G_FILE_THREAD = std::thread(file_loop, control_file);
    G_FILE_THREAD.detach();
  }
}

void stop() {
  G_STOP.store(true, std::memory_order_relaxed);
}

bool is_running() {
  return G_RUNNING.load(std::memory_order_relaxed);
}

bool pop_command(const target_t target, command_t& out) {
  std::lock_guard<std::mutex> lock(G_MUTEX);

  auto& queue = G_QUEUES[target_index(target)];
  if (!queue.empty()) {
    out = std::move(queue.front());
    queue.pop_front();
    return true;
  }

  if (target != target_t::any) {
    auto& any_queue = G_QUEUES[target_index(target_t::any)];
    if (!any_queue.empty()) {
      out = std::move(any_queue.front());
      any_queue.pop_front();
      return true;
    }
  }

  return false;
}

void clear(const target_t target) {
  std::lock_guard<std::mutex> lock(G_MUTEX);
  if (target == target_t::any) {
    for (auto& queue : G_QUEUES) {
      queue.clear();
    }
    return;
  }

  G_QUEUES[target_index(target)].clear();
}

const char* target_to_string(const target_t target) {
  switch (target) {
    case target_t::any:
      return "any";
    case target_t::cs:
      return "cs";
    case target_t::pss:
      return "pss";
    case target_t::tcs:
      return "tcs";
    case target_t::tms:
      return "tms";
    case target_t::mns:
      return "mns";
    case target_t::ls:
      return "ls";
    default:
      return "unknown";
  }
}

}  // namespace control
