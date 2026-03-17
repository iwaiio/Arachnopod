#include "mock_bus.hpp"
#include "i_mock_bus.hpp"

#include <cstdint>

namespace {

mock_bus_state_t G_BUS{};
thread_local std::uint32_t G_LAST_SEEN_SEQUENCE = 0U;

}  // namespace

void mock_bus_init() {
  G_BUS.data.store(0, std::memory_order_relaxed);
  G_BUS.sequence.store(0, std::memory_order_relaxed);
  G_LAST_SEEN_SEQUENCE = 0U;
}

void mock_bus_write_u8(const std::uint8_t value) {
  G_BUS.data.store(value, std::memory_order_relaxed);
  const std::uint32_t seq = G_BUS.sequence.fetch_add(1U, std::memory_order_acq_rel) + 1U;
  G_LAST_SEEN_SEQUENCE = seq;
}

bool mock_bus_read_u8(std::uint8_t* out_value) {
  if (out_value == nullptr) {
    return false;
  }

  const std::uint32_t seq = G_BUS.sequence.load(std::memory_order_acquire);
  if ((seq == 0U) || (seq == G_LAST_SEEN_SEQUENCE)) {
    return false;
  }

  *out_value = G_BUS.data.load(std::memory_order_relaxed);
  G_LAST_SEEN_SEQUENCE = seq;
  return true;
}
