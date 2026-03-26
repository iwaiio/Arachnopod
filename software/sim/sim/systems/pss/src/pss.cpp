#include "pss.hpp"

#include "common.hpp"
#include "pss_internal.hpp"

namespace pss {

void init_msg_buffers() { // Clear wire data storage
  internal::init_bus_state();
  isystools::clear_wire_storage(internal::G_WIRE);
}

void bind_imsg_context() { // Bind IMSG context for this thread
  ipar::bind_context(&internal::G_IMSG_CTX);
}

void bind_ipar_context() {
  bind_imsg_context();
}

const ipar::context_t& imsg_context() { // Return current IMSG context
  return internal::G_IMSG_CTX;
}

const ipar::context_t& ipar_context() {
  return imsg_context();
}

void runtime_init() { // Initialize PSS system software state
  internal::ensure_logger_initialized();
  init_msg_buffers();
  bind_imsg_context();
  internal::init_isim_registry();

  isystools::reset_software_state(internal::G_SOFTWARE, internal::G_LAST_TICK_LOG_COUNT);

  if (internal::G_LOGGER_READY) {
    common::log::info("system software initialized");
    internal::log_msg_snapshot("software init");
  }
}

void runtime_step(const std::uint32_t tick) { // Execute one PSS system software tick
  if (!internal::G_SOFTWARE.initialized) {
    runtime_init();
  }

  internal::G_SOFTWARE.current_tick = tick;
  internal::G_BUS.writer_tick = tick;

  const bool power_on = internal::is_power_on();
  isysalgo::ISYSBASEALG(
      internal::G_SOFTWARE.tick, power_on, internal::G_BUS, internal::k_bus_hooks, internal::k_base_hooks, nullptr);
}

}  // namespace pss
