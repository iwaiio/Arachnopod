#include "i_cs.hpp"

#include "common.hpp"
#include "cs.hpp"
#include "cs_internal.hpp"
#include "cs_operating_mode.hpp"
#include "cs_scheduler.hpp"
#include "cs_systems.hpp"

namespace cs {

void init_msg_buffers() {
  internal::ensure_logger_initialized();
  internal::init_bus_state();
  isystools::clear_wire_storage(internal::G_WIRE);
  isysalgo::IBUS_RESET_FRAME_LAYOUT(internal::G_BUS);
  if (internal::G_LOGGER_READY) {
    common::log::info("wire data storage initialized");
  }
}

void bind_imsg_context() {
  internal::ensure_logger_initialized();
  ipar::bind_context(&internal::G_IMSG_CTX);
  if (internal::G_LOGGER_READY) {
    common::log::info("IMSG context bound");
  }
}

void bind_ipar_context() {
  bind_imsg_context();
}

const ipar::context_t& imsg_context() {
  return internal::G_IMSG_CTX;
}

const ipar::context_t& ipar_context() {
  return imsg_context();
}

isysalgo::bus_state_t& bus_state() {
  internal::init_bus_state();
  return internal::G_BUS;
}

void runtime_init() {
  init_msg_buffers();
  bind_imsg_context();
  internal::init_isim_registry();
  scheduler::init({
      .imsg_ctx = &internal::G_IMSG_CTX,
      .msg_cmd = internal::G_WIRE.msg_cmd.data(),
      .msg_cmd_blocks = internal::G_WIRE.msg_cmd.size(),
      .msg_cmd_buf = internal::G_WIRE.msg_cmd_buf.u16.data(),
      .msg_cmd_buf_blocks = internal::G_WIRE.msg_cmd_buf.u16.size(),
  });
  scheduler::reset();
  operating_mode::reset();
  systems::reset();

  isystools::reset_software_state(internal::G_SOFTWARE, internal::G_LAST_TICK_LOG_COUNT);
  if (internal::G_LOGGER_READY) {
    internal::log_msg_snapshot("software init");
  }
}

void runtime_step(const std::uint32_t tick) {
  if (!internal::G_SOFTWARE.initialized) {
    runtime_init();
  }

  auto& bus = bus_state();
  bus.writer_tick = tick;

  internal::G_SOFTWARE.current_tick = tick;
  internal::ICSBASEALG(internal::G_SOFTWARE.tick, bus, internal::k_bus_hooks, &internal::G_SOFTWARE);
}

}  // namespace cs
