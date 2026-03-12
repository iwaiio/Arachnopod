#include "i_cs.hpp"

#include <cstdint>
#include <sstream>

#include "cs.hpp"
#include "cs_exchange.hpp"
#include "common.hpp"

namespace cs {
namespace {

struct runtime_state_t {
  bool initialized{false};
  std::uint64_t count{0U};
  std::uint8_t clock{0U};
};

runtime_state_t G_RUNTIME{};

void step_runtime_counters() {
  ++G_RUNTIME.count;
  G_RUNTIME.clock = (G_RUNTIME.clock == 0xFFU) ? 1U : static_cast<std::uint8_t>(G_RUNTIME.clock + 1U);

  if (!common::log::is_initialized()) {
    return;
  }

  std::ostringstream oss;
  oss << "tick: count=" << G_RUNTIME.count << " clock=" << static_cast<unsigned>(G_RUNTIME.clock);
  common::log::info(oss.str());
}

}  // namespace

void runtime_init() {
  init_msg_buffers();
  bind_ipar_context();
  exchange::init();

  G_RUNTIME = runtime_state_t{};
  G_RUNTIME.initialized = true;
}

void runtime_step(const std::uint32_t tick) {
  if (!G_RUNTIME.initialized) {
    runtime_init();
  }

  step_runtime_counters();
  exchange::step(tick);
}

}  // namespace cs
