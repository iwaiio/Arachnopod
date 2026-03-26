#include "mock_bus.hpp"
#include "i_mock_bus.hpp"

#include <cstdint>

namespace {

mock_bus_state_t G_BUS{};

}  // namespace

void mock_bus_init() {
  std::lock_guard<std::mutex> lock(G_BUS.mutex);
  G_BUS.visible_data = 0U;
  G_BUS.visible_tick = 0U;
  G_BUS.visible_valid = false;
  G_BUS.pending_data = 0U;
  G_BUS.pending_tick = 0U;
  G_BUS.pending_valid = false;
}

void mock_bus_tick(const std::uint32_t tick) {
  std::lock_guard<std::mutex> lock(G_BUS.mutex);
  if (G_BUS.pending_valid && (G_BUS.pending_tick + 1U == tick)) {
    G_BUS.visible_data = G_BUS.pending_data;
    G_BUS.visible_tick = tick;
    G_BUS.visible_valid = true;
  } else {
    G_BUS.visible_data = 0U;
    G_BUS.visible_tick = tick;
    G_BUS.visible_valid = false;
  }

  G_BUS.pending_valid = false;
}

void mock_bus_write_u8(const std::uint32_t tick, const std::uint8_t value) {
  std::lock_guard<std::mutex> lock(G_BUS.mutex);
  G_BUS.pending_data = value;
  G_BUS.pending_tick = tick;
  G_BUS.pending_valid = true;
}

bool mock_bus_read_u8(const std::uint32_t tick, std::uint8_t* out_value) {
  if (out_value == nullptr) {
    return false;
  }

  std::lock_guard<std::mutex> lock(G_BUS.mutex);
  if (!G_BUS.visible_valid || (G_BUS.visible_tick != tick)) {
    return false;
  }

  *out_value = G_BUS.visible_data;
  return true;
}
