#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "i_control_console.hpp"

namespace control::internal {

extern std::atomic<bool> G_RUNNING;
extern std::atomic<bool> G_STOP;
extern std::atomic<unsigned> G_ACTIVE_WORKERS;
extern std::mutex G_MUTEX;
extern std::array<std::deque<command_t>, static_cast<std::size_t>(target_t::count)> G_QUEUES;
extern std::thread G_THREAD;
extern std::thread G_FILE_THREAD;
extern config_t G_CFG;

void worker_started();
void worker_stopped();
void ensure_logger();

std::string to_lower_copy(std::string s);
std::vector<std::string> split_ws(const std::string& line);
std::size_t target_index(target_t target);
bool parse_number(const std::string& token, float& out);
target_t parse_target(const std::string& token);
void enqueue_command(command_t&& cmd);
bool parse_line(const std::string& line, command_t& out);
void console_loop();
std::filesystem::path resolve_log_dir();
std::filesystem::path resolve_control_file();
void file_loop(std::filesystem::path path);

}  // namespace control::internal
