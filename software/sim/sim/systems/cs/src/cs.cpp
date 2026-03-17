#include "i_cs.hpp"

#include "cs.hpp"
#include "cs_operating_mode.hpp"
#include "cs_scheduler.hpp"
#include "cs_systems.hpp"

#include <array>
#include <cstdint>
#include <sstream>

#include "common.hpp"
#include "i_sys_algo.hpp"
#include "i_sys.hpp"

namespace cs {
namespace {

isys::msg_storage_t<1> MSG_HEADER{};
isys::msg_storage_t<CS_COMM_MSG_BLOCKS> MSG_COM_BUF{};
isys::msg_storage_t<CS_PARAM_MSG_BLOCKS> MSG_PAR_BUF{};

std::array<std::uint16_t, CS_PARAM_MSG_BLOCKS> MSG_PAR{};
std::array<std::uint16_t, CS_COMM_MSG_BLOCKS> MSG_COM{};

struct runtime_state_t {
  bool initialized{false};
  isys::tick_state_t tick{};
  std::uint32_t current_tick{0U};
};

runtime_state_t G_RUNTIME{};
isysalgo::bus_state_t G_BUS{};
bool G_BUS_READY = false;
bool G_LOGGER_READY = false;

void ensure_logger_initialized() {
  if (G_LOGGER_READY) {
    return;
  }

  const common::log::init_config_t cfg{
      .min_level = common::log::level_t::info,
      .truncate_on_init = true,
  };
  G_LOGGER_READY = common::log::init_for_module("cs", cfg);
  if (G_LOGGER_READY) {
    common::log::info("logger initialized");
  }
}

ipar::context_t make_context() {
  return isys::ISYSCTX(ipar::role_t::cs,
                       SYS_CS,
                       MSG_PAR.data(),
                       MSG_PAR.size(),
                       MSG_COM.data(),
                       MSG_COM.size());
}

ipar::context_t G_IPAR_CTX = make_context();

void init_bus_state() {
  if (G_BUS_READY) {
    return;
  }

  isysalgo::IBUS_BIND_STD(G_BUS,
                          isysalgo::bus_role_t::master,
                          ARP_ADDR_CS,
                          ARP_ADDR_PSS,
                          MSG_HEADER.u8.data(),
                          MSG_HEADER.u16.data(),
                          MSG_COM_BUF.u8.data(),
                          MSG_COM_BUF.u16.data(),
                          MSG_PAR_BUF.u8.data(),
                          MSG_PAR_BUF.u16.data(),
                          reinterpret_cast<std::uint8_t*>(MSG_COM.data()),
                          MSG_COM.data(),
                          reinterpret_cast<std::uint8_t*>(MSG_PAR.data()),
                          MSG_PAR.data(),
                          MSG_COM.size(),
                          MSG_PAR.size());
  G_BUS_READY = true;
}

void log_tick_state(void*) {
  if (!G_LOGGER_READY) {
    return;
  }

  std::ostringstream oss;
  oss << "tick: count=" << G_RUNTIME.tick.count << " clock=" << static_cast<unsigned>(G_RUNTIME.tick.clock);
  common::log::info(oss.str());
}

void local_algorithm(void*) {
  scheduler::local_algorithm();
  operating_mode::step();
  systems::step();
}

void exchange_control(void*, isysalgo::bus_state_t& bus) {
  scheduler::exchange_control(bus);
}

const isysalgo::bus_hooks_t k_bus_hooks{
    .parse_header = nullptr,
    .gen_header = nullptr,
    .bypass = nullptr,
    .log_exchange = nullptr,
    .on_error = nullptr,
};

const isysalgo::base_hooks_t k_base_hooks{
    .bus_exchange = nullptr,
    .sim_param_update = nullptr,
    .cmd_update = nullptr,
    .local_algorithm = local_algorithm,
    .exchange_control = exchange_control,
    .cmd_pwr_update = nullptr,
    .logging = log_tick_state,
};

}  // namespace

void init_msg_buffers() {
  ensure_logger_initialized();
  init_bus_state();
  isys::ISYSCLEAR(MSG_PAR.data(), MSG_PAR.size());
  isys::ISYSCLEAR(MSG_COM.data(), MSG_COM.size());
  isys::ISYSCLEAR(MSG_COM_BUF.u16.data(), MSG_COM_BUF.u16.size());
  isys::ISYSCLEAR(MSG_PAR_BUF.u16.data(), MSG_PAR_BUF.u16.size());
  MSG_HEADER.u16[0] = 0;
  isysalgo::IBUS_RESET_FRAME_LAYOUT(G_BUS);
  if (G_LOGGER_READY) {
    common::log::info("message buffers initialized");
  }
}

void bind_ipar_context() {
  ensure_logger_initialized();
  ipar::bind_context(&G_IPAR_CTX);
  if (G_LOGGER_READY) {
    common::log::info("IPAR context bound");
  }
}

const ipar::context_t& ipar_context() {
  return G_IPAR_CTX;
}

isysalgo::bus_state_t& bus_state() {
  init_bus_state();
  return G_BUS;
}

void runtime_init() {
  init_msg_buffers();
  bind_ipar_context();
  scheduler::init({
      .ipar_ctx = &G_IPAR_CTX,
      .msg_com = MSG_COM.data(),
      .msg_com_blocks = MSG_COM.size(),
      .msg_com_buf = MSG_COM_BUF.u16.data(),
      .msg_com_buf_blocks = MSG_COM_BUF.u16.size(),
  });
  scheduler::reset();
  operating_mode::reset();
  systems::reset();

  G_RUNTIME = runtime_state_t{};
  G_RUNTIME.initialized = true;
}

void runtime_step(const std::uint32_t tick) {
  if (!G_RUNTIME.initialized) {
    runtime_init();
  }

  auto& bus = bus_state();
  bus.writer_tick = tick;

  G_RUNTIME.current_tick = tick;
  isysalgo::ISYSBASE_STEP(G_RUNTIME.tick,
                          true,
                          bus,
                          k_bus_hooks,
                          k_base_hooks,
                          &G_RUNTIME);
}

}  // namespace cs
