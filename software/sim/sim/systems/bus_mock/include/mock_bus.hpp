#pragma once

#include <atomic>
#include <cstdint>

// Private state of mock_bus.
// Bus stores one 8-bit byte at a time.
struct mock_bus_state_t {
  std::atomic<std::uint8_t> data{0};
  std::atomic<std::uint8_t> is_new{0};
  std::atomic<std::uint8_t> rx_addr{0};

  // Debug metadata of the latest byte.
  std::atomic<std::uint32_t> seq{0};
  std::atomic<std::uint8_t> writer_addr{0};
  std::atomic<std::uint32_t> writer_tick{0};
  std::atomic<std::uint64_t> write_time_ns{0};
};
