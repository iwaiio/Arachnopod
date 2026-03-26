#include "i_control_console.hpp"

#include "control_internal.hpp"

namespace control {

void start(const config_t cfg) {
  if (!cfg.enabled || (!cfg.console_enabled && !cfg.file_enabled)) {
    return;
  }
  if (internal::G_RUNNING.exchange(true, std::memory_order_relaxed)) {
    internal::G_RUNNING.store(true, std::memory_order_relaxed);
    return;
  }

  internal::G_CFG = cfg;
  internal::G_STOP.store(false, std::memory_order_relaxed);
  internal::G_ACTIVE_WORKERS.store(0U, std::memory_order_relaxed);
  internal::G_RUNNING.store(false, std::memory_order_relaxed);

  if (internal::G_CFG.console_enabled) {
    internal::G_THREAD = std::thread(internal::console_loop);
    internal::G_THREAD.detach();
  }

  const auto control_file = internal::G_CFG.file_enabled ? internal::resolve_control_file() : std::filesystem::path{};
  if (!control_file.empty()) {
    internal::G_FILE_THREAD = std::thread(internal::file_loop, control_file);
    internal::G_FILE_THREAD.detach();
  }

  if (internal::G_ACTIVE_WORKERS.load(std::memory_order_relaxed) == 0U) {
    internal::G_RUNNING.store(false, std::memory_order_relaxed);
  }
}

void stop() {
  internal::G_STOP.store(true, std::memory_order_relaxed);
}

bool is_running() {
  return internal::G_RUNNING.load(std::memory_order_relaxed);
}

bool pop_command(const target_t target, command_t& out) {
  std::lock_guard<std::mutex> lock(internal::G_MUTEX);

  auto& queue = internal::G_QUEUES[internal::target_index(target)];
  if (!queue.empty()) {
    out = std::move(queue.front());
    queue.pop_front();
    return true;
  }

  if (target != target_t::any) {
    auto& any_queue = internal::G_QUEUES[internal::target_index(target_t::any)];
    if (!any_queue.empty()) {
      out = std::move(any_queue.front());
      any_queue.pop_front();
      return true;
    }
  }

  return false;
}

void clear(const target_t target) {
  std::lock_guard<std::mutex> lock(internal::G_MUTEX);
  if (target == target_t::any) {
    for (auto& queue : internal::G_QUEUES) {
      queue.clear();
    }
    return;
  }

  internal::G_QUEUES[internal::target_index(target)].clear();
}

bool try_parse_target(const std::string_view token, target_t& out) {
  const target_t parsed = internal::parse_target(std::string(token));
  if ((parsed == target_t::any) && (internal::to_lower_copy(std::string(token)) != "any")) {
    return false;
  }

  out = parsed;
  return true;
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
