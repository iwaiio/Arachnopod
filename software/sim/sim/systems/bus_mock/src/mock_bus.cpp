#include "mock_bus.hpp"
#include "i_mock_bus.hpp"

#include <cstdint>

namespace {

mock_bus_state_t G_BUS{};
thread_local std::uint64_t G_LAST_SEEN_SEQUENCE = 0U;

}  // namespace

void mock_bus_init() {
  std::lock_guard<std::mutex> lock(G_BUS.mutex);
  G_BUS.data.fill(0U);
  G_BUS.sequence = 0U;
  G_LAST_SEEN_SEQUENCE = 0U;
}

void mock_bus_write_u8(const std::uint8_t value) {
  std::lock_guard<std::mutex> lock(G_BUS.mutex);
  const std::uint64_t seq = G_BUS.sequence + 1U;
  G_BUS.data[static_cast<std::size_t>(seq % MOCK_BUS_BUFFER_CAPACITY)] = value;
  G_BUS.sequence = seq;
  G_LAST_SEEN_SEQUENCE = seq;
}

bool mock_bus_read_u8(std::uint8_t* out_value) {
  if (out_value == nullptr) {
    return false;
  }

  std::lock_guard<std::mutex> lock(G_BUS.mutex);
  const std::uint64_t seq = G_BUS.sequence;
  if ((seq == 0U) || (seq == G_LAST_SEEN_SEQUENCE)) {
    return false;
  }

  std::uint64_t next_sequence = G_LAST_SEEN_SEQUENCE + 1U;
  if ((seq - G_LAST_SEEN_SEQUENCE) > MOCK_BUS_BUFFER_CAPACITY) {
    next_sequence = seq - MOCK_BUS_BUFFER_CAPACITY + 1U;
  }

  *out_value = G_BUS.data[static_cast<std::size_t>(next_sequence % MOCK_BUS_BUFFER_CAPACITY)];
  G_LAST_SEEN_SEQUENCE = next_sequence;
  return true;
}
