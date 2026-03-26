#pragma once

#include <cstdint>
#include <mutex>

// Private state of mock_bus.
// Bus stores one visible byte for the current tick and one pending byte written during this tick.
struct mock_bus_state_t {
  std::uint8_t visible_data{0U};
  std::uint32_t visible_tick{0U};
  bool visible_valid{false};
  std::uint8_t pending_data{0U};
  std::uint32_t pending_tick{0U};
  bool pending_valid{false};
  std::mutex mutex{};
};
