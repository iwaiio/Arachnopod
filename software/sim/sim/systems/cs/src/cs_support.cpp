#include "cs_internal.hpp"

#include <array>
#include <cstdint>
#include <sstream>

#include "common.hpp"
#include "cs_control_bridge.hpp"
#include "cs_operating_mode.hpp"
#include "cs_scheduler.hpp"
#include "cs_systems.hpp"
#include "i_isim_registry.hpp"
#include "i_msg_block.hpp"
#include "sim_cs_base.hpp"

namespace cs::internal {
namespace {

ipar::context_t make_context() {
  return isystools::make_imsg_context(ipar::role_t::cs, SYS_CS, G_WIRE);
}

}  // namespace

wire_storage_t G_WIRE{};
software_state_t G_SOFTWARE{};
ipar::context_t G_IMSG_CTX = make_context();
isysalgo::bus_state_t G_BUS{};
bool G_BUS_READY = false;
bool G_LOGGER_READY = false;
std::uint64_t G_LAST_TICK_LOG_COUNT = 0U;
std::array<isim::binding_t, sim_cs_base::k_isim_param_count> G_ISIM_BINDINGS{};
isim::registry_t G_ISIM_REGISTRY{};

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

void init_bus_state() {
  if (G_BUS_READY) {
    return;
  }

  isystools::bind_standard_bus(
      G_BUS, isysalgo::bus_role_t::master, SYS_CS, ARP_ADDR_CS, ARP_ADDR_PSS, G_WIRE);
  G_BUS_READY = true;
}

void init_isim_registry() {
  isystools::build_isim_registry(
      sim_cs_base::ISIM_PARAMS.data(), sim_cs_base::ISIM_PARAMS.size(), G_ISIM_BINDINGS, G_ISIM_REGISTRY, SYS_CS);
}

void log_msg_snapshot(const std::string_view stage) {
  if (!G_LOGGER_READY) {
    return;
  }

  const auto& ctx = G_IMSG_CTX;
  std::ostringstream oss;
  oss << "MSG snapshot [" << stage << "]: "
      << "MSG_CMD(" << ctx.msg_cmd_blocks << ")=" << isystools::format_msg_words(ctx.msg_cmd, ctx.msg_cmd_blocks) << ' '
      << "MSG_PRM(" << ctx.msg_prm_blocks << ")=" << isystools::format_msg_words(ctx.msg_prm, ctx.msg_prm_blocks);
  common::log::info(oss.str());
}

bool is_power_on() {
  std::int32_t power = 1;
  if (isim::ISIMGETI32(LCSPWR, &power) != isim::status_t::ok) {
    return true;
  }
  return power != 0;
}

void ILOG(void*) {
  if (!G_LOGGER_READY) {
    return;
  }

  std::ostringstream oss;
  oss << "tick: count=" << G_SOFTWARE.tick.count << " clock=" << static_cast<unsigned>(G_SOFTWARE.tick.clock)
      << " power=" << (is_power_on() ? 1 : 0) << " exchange_busy=" << (G_BUS.exchange_busy ? 1 : 0) << ' '
      << "MSG_CMD=" << isystools::format_msg_words(G_IMSG_CTX.msg_cmd, G_IMSG_CTX.msg_cmd_blocks) << ' '
      << "MSG_PRM=" << isystools::format_msg_words(G_IMSG_CTX.msg_prm, G_IMSG_CTX.msg_prm_blocks);
  common::log::info(oss.str());
}

void ICSCONTROL(void*) {
  control_bridge::step();
}

void ICSHIALG(void*) {
  operating_mode::step();
}

void LOCALSYSALG(void*) {}

void ICSSYSOPERATOR(void*) {
  systems::step();
}

void ICSMODULEOPERATOR(void*) {}

void ICSNODES(void* user) {
  ICSSYSOPERATOR(user);
  ICSMODULEOPERATOR(user);
}

void ICSSCHEDULER(void*, isysalgo::bus_state_t& bus) {
  if (bus.exchange_busy) {
    return;
  }
  scheduler::exchange_control(bus);
}

void prepare_command_exchange(const scheduler::internal::task_t& task, isysalgo::bus_state_t& bus) {
  if ((task.node == nullptr) || (scheduler::internal::G_CTX.imsg_ctx == nullptr)) {
    return;
  }

  scheduler::internal::clear_command_buffer_window(*task.node);
  scheduler::internal::load_pending_command_window(*task.node);
  (void)isysalgo::IBUS_SET_CMD_FRAME(bus, task.node->com_cs_base_block, task.node->com_blocks);
  (void)isysalgo::IBUS_SET_PRM_FRAME(bus, task.node->par_cs_base_block, task.node->par_blocks);
  if (!task.use_staged_window && (task.command_id != 0U)) {
    (void)ipar::IMSGSET(*scheduler::internal::G_CTX.imsg_ctx,
                        task.command_id,
                        task.has_value ? task.command_value : 1.0F);
  }
  scheduler::internal::merge_command_window_to_buffer(*task.node);
  scheduler::internal::clear_command_msg_window(*task.node);
}

void prepare_data_exchange(const scheduler::internal::task_t& task, isysalgo::bus_state_t& bus) {
  if (task.node == nullptr) {
    return;
  }

  (void)isysalgo::IBUS_SET_CMD_FRAME(bus, task.node->com_cs_base_block, task.node->com_blocks);
  (void)isysalgo::IBUS_SET_PRM_FRAME(bus, task.node->par_cs_base_block, task.node->par_blocks);
}

void arm_active_exchange(isysalgo::bus_state_t& bus) {
  const auto slot = scheduler::internal::G_ACTIVE_TASK_SLOT;
  if (slot == scheduler::internal::k_invalid_slot) {
    return;
  }
  if (slot >= scheduler::internal::G_TASKS.size()) {
    return;
  }

  auto& task = scheduler::internal::G_TASKS[slot];
  if (!task.used || !task.active || task.exchange_armed || (task.node == nullptr)) {
    return;
  }

  isysalgo::IBUS_RESET_FRAME_LAYOUT(bus);
  if (task.kind == scheduler::internal::task_kind_t::command) {
    prepare_command_exchange(task, bus);
    bus.msg_flag = ARP_FLAG_CMD_REQ;
  } else {
    prepare_data_exchange(task, bus);
    bus.msg_flag = ARP_FLAG_DATA_REQ;
  }

  bus.target_addr = task.node->bus_addr;
  bus.exchange_flag = isysalgo::exchange_flag_t::tx;
  bus.tx_flag = isysalgo::tx_flag_t::tx_sof;
  bus.rx_flag = isysalgo::rx_flag_t::rx_sof;
  bus.msg_adr_flag = false;
  bus.msg_n_blocks = 0;
  bus.exchange_sub_clock = 0;
  bus.exchange_wait_clock = 0;
  bus.exchange_busy = true;
  isysalgo::IBUS_CLEAR_RESULT(bus);
  task.exchange_armed = true;
}

void IBUSEXCHANGE(isysalgo::bus_state_t& bus, const isysalgo::bus_hooks_t& hooks, void* user) {
  if (!bus.exchange_busy) {
    arm_active_exchange(bus);
  }
  isysalgo::IBUS_EXCHANGE(bus, hooks, user);
}

void ICSALG(void* user, isysalgo::bus_state_t& bus) {
  ICSCONTROL(user);
  ICSHIALG(user);
  LOCALSYSALG(user);
  ICSNODES(user);
  ICSSCHEDULER(user, bus);
}

void ICSBASEALG(isys::tick_state_t& tick,
                isysalgo::bus_state_t& bus,
                const isysalgo::bus_hooks_t& bus_hooks,
                void* user) {
  isys::ISYSTEP(tick);
  ICSALG(user, bus);
  IBUSEXCHANGE(bus, bus_hooks, user);
  ILOG(user);
}

const isysalgo::bus_hooks_t k_bus_hooks{
    .parse_header = nullptr,
    .gen_header = nullptr,
    .bypass = nullptr,
    .log_exchange = nullptr,
    .on_error = nullptr,
};

}  // namespace cs::internal
