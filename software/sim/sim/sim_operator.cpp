/*
    Threaded start-up (17 threads):
      Systems software: CS, PSS, TCS, MNS, TMS, LS.
      Simulation models: CS, PSS, TCS, TMS, MNS, INM, DCM, NM, CVM, AM, LS.
*/

#include <atomic>
#include <chrono>
#include <csignal>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "arp_config.hpp"
#include "common.hpp"
#include "simulation/pss/interface/i_pss_sim.hpp"
#include "systems/bus_mock/interface/i_mock_bus.hpp"
#include "systems/control/interface/i_control_console.hpp"
#include "systems/cs/interface/i_cs.hpp"
#include "systems/pss/interface/i_pss.hpp"

namespace {

std::atomic<bool> g_stop_requested{false};

void on_signal(int) {
  g_stop_requested.store(true, std::memory_order_relaxed);
}

std::uint32_t read_duration_from_env_seconds() {
  const char* raw = std::getenv("ARP_SIM_DURATION_SEC");
  if (raw == nullptr || *raw == '\0') {
    return 0U;
  }

  char* end = nullptr;
  const unsigned long value = std::strtoul(raw, &end, 10);
  if (end == raw || (end != nullptr && *end != '\0')) {
    return 0U;
  }

  return static_cast<std::uint32_t>(value);
}

std::chrono::microseconds tick_period() {
  constexpr std::uint32_t k_default_hz = 1000U;
  const std::uint32_t hz = (ARP_TICK_HZ == 0U) ? k_default_hz : ARP_TICK_HZ;
  const std::uint32_t speed_percent = (ARP_SIM_SPEED_PERCENT == 0U) ? 100U : ARP_SIM_SPEED_PERCENT;

  const double base_period_sec = 1.0 / static_cast<double>(hz);
  const double scaled_period_sec = base_period_sec * (100.0 / static_cast<double>(speed_percent));

  const auto period =
      std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double>(scaled_period_sec));
  if (period.count() <= 0) {
    return std::chrono::microseconds(1);
  }
  return period;
}

std::string normalize_module_name(const std::string_view name) {
  std::string module;
  module.reserve(name.size());

  for (const char ch : name) {
    const unsigned char uch = static_cast<unsigned char>(ch);
    if (std::isalnum(uch) != 0) {
      module.push_back(static_cast<char>(std::tolower(uch)));
    } else {
      module.push_back('_');
    }
  }

  if (module.empty()) {
    module = "thread";
  }
  return module;
}

void init_thread_logger(const std::string_view module_name) {
  const common::log::init_config_t cfg{
      .min_level = common::log::level_t::info,
      .truncate_on_init = true,
  };
  if (common::log::init_for_module(module_name, cfg)) {
    common::log::info("logger initialized");
  }
}

void sleep_to_next_tick(const std::chrono::microseconds period,
                        std::chrono::steady_clock::time_point& next_wakeup) {
  next_wakeup += period;

  const auto now = std::chrono::steady_clock::now();
  if (next_wakeup <= now) {
    next_wakeup = now + period;
  }

  std::this_thread::sleep_until(next_wakeup);
}

void idle_worker_loop(const std::string_view name, const std::chrono::microseconds period) {
  init_thread_logger(normalize_module_name(name));

  std::uint64_t tick = 0U;
  auto next_wakeup = std::chrono::steady_clock::now();

  while (!g_stop_requested.load(std::memory_order_relaxed)) {
    ++tick;
    sleep_to_next_tick(period, next_wakeup);
  }
}

void cs_worker_loop(const std::chrono::microseconds period) {
  cs::runtime_init();

  std::uint32_t tick = 0U;
  auto next_wakeup = std::chrono::steady_clock::now();

  while (!g_stop_requested.load(std::memory_order_relaxed)) {
    cs::runtime_step(tick);
    ++tick;

    sleep_to_next_tick(period, next_wakeup);
  }
}

void pss_worker_loop(const std::chrono::microseconds period) {
  pss::runtime_init();

  std::uint32_t tick = 0U;
  auto next_wakeup = std::chrono::steady_clock::now();

  while (!g_stop_requested.load(std::memory_order_relaxed)) {
    pss::runtime_step(tick);
    ++tick;

    sleep_to_next_tick(period, next_wakeup);
  }
}

void pss_sim_worker_loop(const std::chrono::microseconds period) {
  init_thread_logger("sim_pss");
  pss_sim::init();

  std::uint32_t tick = 0U;
  auto next_wakeup = std::chrono::steady_clock::now();

  while (!g_stop_requested.load(std::memory_order_relaxed)) {
    pss_sim::step(tick);
    ++tick;
    sleep_to_next_tick(period, next_wakeup);
  }
}

}  // namespace

int main() {
  std::signal(SIGINT, on_signal);
  std::signal(SIGTERM, on_signal);

  mock_bus_init();

  const char* control_env = std::getenv("ARP_CONTROL_CONSOLE");
  const bool console_enabled = (control_env == nullptr) || (std::string(control_env) != "0");
  control::config_t control_cfg{};
  control_cfg.enabled = true;
  control_cfg.console_enabled = console_enabled;
  control_cfg.file_enabled = true;
  control::start(control_cfg);

  const auto period_sys = tick_period();
  const auto period_sim = period_sys * 5;
  const std::uint32_t duration_sec = read_duration_from_env_seconds();

  std::vector<std::thread> threads;
  threads.reserve(17);

  // Systems software threads.
  threads.emplace_back(cs_worker_loop, period_sys);
  threads.emplace_back(pss_worker_loop, period_sys);
  threads.emplace_back(idle_worker_loop, "SYS_TCS", period_sys);
  threads.emplace_back(idle_worker_loop, "SYS_MNS", period_sys);
  threads.emplace_back(idle_worker_loop, "SYS_TMS", period_sys);
  threads.emplace_back(idle_worker_loop, "SYS_LS", period_sys);

  // Simulation model threads.
  threads.emplace_back(idle_worker_loop, "SIM_CS", period_sim);
  threads.emplace_back(pss_sim_worker_loop, period_sim);
  threads.emplace_back(idle_worker_loop, "SIM_TCS", period_sim);
  threads.emplace_back(idle_worker_loop, "SIM_TMS", period_sim);
  threads.emplace_back(idle_worker_loop, "SIM_MNS", period_sim);
  threads.emplace_back(idle_worker_loop, "SIM_INM", period_sim);
  threads.emplace_back(idle_worker_loop, "SIM_DCM", period_sim);
  threads.emplace_back(idle_worker_loop, "SIM_NM", period_sim);
  threads.emplace_back(idle_worker_loop, "SIM_CVM", period_sim);
  threads.emplace_back(idle_worker_loop, "SIM_AM", period_sim);
  threads.emplace_back(idle_worker_loop, "SIM_LS", period_sim);

  std::thread duration_guard;
  if (duration_sec > 0U) {
    duration_guard = std::thread([duration_sec]() {
      std::this_thread::sleep_for(std::chrono::seconds(duration_sec));
      g_stop_requested.store(true, std::memory_order_relaxed);
    });
  }

  std::cout << "sim_operator started with " << threads.size() << " worker threads." << std::endl;
  if (duration_sec > 0U) {
    std::cout << "auto-stop after " << duration_sec << " sec (ARP_SIM_DURATION_SEC)." << std::endl;
  } else {
    std::cout << "press Ctrl+C to stop." << std::endl;
  }

  while (!g_stop_requested.load(std::memory_order_relaxed)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  for (auto& t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }

  if (duration_guard.joinable()) {
    duration_guard.join();
  }

  std::cout << "sim_operator stopped." << std::endl;
  return 0;
}
