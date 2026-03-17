#pragma once

#include <atomic>
#include <cstdint>

// Private state of mock_bus.
// Bus stores one 8-bit byte at a time.
struct mock_bus_state_t {
  std::atomic<std::uint8_t> data{0};
  std::atomic<std::uint32_t> sequence{0};
};
