#pragma once

#include <array>
#include <cstdint>
#include <mutex>

constexpr std::size_t MOCK_BUS_BUFFER_CAPACITY = 4096U;

// Private state of mock_bus.
// Bus stores a rolling stream of 8-bit bytes.
struct mock_bus_state_t {
  std::array<std::uint8_t, MOCK_BUS_BUFFER_CAPACITY> data{};
  std::uint64_t sequence{0U};
  std::mutex mutex{};
};
