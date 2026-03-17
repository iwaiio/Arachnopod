#include "pss.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

#include "../../base/param_comm_list.hpp"
#include "../../base/sys_id_range.hpp"
#include "common.hpp"
#include "i_cmd.hpp"
#include "i_cmd_exec.hpp"
#include "i_sys_algo.hpp"
#include "i_sim.hpp"
#include "i_isim_registry.hpp"
#include "i_sys.hpp"
#include "sim_pss_base.hpp"

namespace pss {
namespace {

bool G_LOGGER_READY = false;

isys::msg_storage_t<1> MSG_HEADER{};
isys::msg_storage_t<PSS_COMM_MSG_BLOCKS> MSG_COM_BUF{};
isys::msg_storage_t<PSS_PARAM_MSG_BLOCKS> MSG_PAR_BUF{};

std::array<std::uint16_t, PSS_PARAM_MSG_BLOCKS> MSG_PAR{};
std::array<std::uint16_t, PSS_COMM_MSG_BLOCKS> MSG_COM{};

ipar::context_t make_context() { // Build IPAR context for PSS
  return isys::ISYSCTX(ipar::role_t::device,
                       SYS_PSS,
                       MSG_PAR.data(),
                       MSG_PAR.size(),
                       MSG_COM.data(),
                       MSG_COM.size());
}

ipar::context_t G_IPAR_CTX = make_context();

struct runtime_state_t {
  bool initialized{false};
  isys::tick_state_t tick{};
  std::uint32_t current_tick{0U};
};

runtime_state_t G_RUNTIME{};
isysalgo::bus_state_t G_BUS{};
bool G_BUS_READY = false;

std::array<isim::binding_t, static_cast<std::size_t>(PSS_COUNT)> G_ISIM_BINDINGS{};
isim::registry_t G_ISIM_REGISTRY{};

constexpr std::array<icmdexec::binding_t, 5> k_runtime_bool_commands{{
    {icmdexec::mode_t::bool_pair, CPSSPWR12ON, CPSSPWR12OFF, PSSPWR12},
    {icmdexec::mode_t::bool_pair, CPSSPWR6ON, CPSSPWR6OFF, PSSPWR6},
    {icmdexec::mode_t::bool_pair, CPSSPWR5ON, CPSSPWR5OFF, PSSPWR5},
    {icmdexec::mode_t::bool_pair, CPSSPWR33ON, CPSSPWR33OFF, PSSPWR33},
    {icmdexec::mode_t::bool_pair, CPSSECOON, CPSSECOOFF, PSSECO},
}};

constexpr std::array<icmdexec::binding_t, 10> k_runtime_value_commands{{
    {icmdexec::mode_t::nonzero_value, CPSSMAXT1, 0, PSSMAXT1},
    {icmdexec::mode_t::nonzero_value, CPSSMAXT2, 0, PSSMAXT2},
    {icmdexec::mode_t::nonzero_value, CPSSMAXA, 0, PSSMAXA},
    {icmdexec::mode_t::nonzero_value, CPSSMAXW, 0, PSSMAXW},
    {icmdexec::mode_t::nonzero_value, CPSSMINC, 0, PSSMINC},
    {icmdexec::mode_t::nonzero_value, CPSSMINV, 0, PSSMINV},
    {icmdexec::mode_t::nonzero_value, CPSSMAXA12, 0, PSSMAXA12},
    {icmdexec::mode_t::nonzero_value, CPSSMAXA6, 0, PSSMAXA6},
    {icmdexec::mode_t::nonzero_value, CPSSMAXA5, 0, PSSMAXA5},
    {icmdexec::mode_t::nonzero_value, CPSSMAXA33, 0, PSSMAXA33},
}};

constexpr std::array<icmdexec::binding_t, 1> k_power_commands{{
    {icmdexec::mode_t::bool_pair, CPSSPWRON, CPSSPWROFF, PSSPWR},
}};

void init_bus_state() { // Initialize bus state pointers and flags
  if (G_BUS_READY) {
    return;
  }

  isysalgo::IBUS_BIND_STD(G_BUS,
                          isysalgo::bus_role_t::device,
                          ARP_ADDR_PSS,
                          ARP_ADDR_CS,
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

void init_isim_registry() { // Build and bind ISIM registry
  isimreg::ISIMREG_BUILD(sim_pss_base::PARAMS.data(),
                         sim_pss_base::PARAMS.size(),
                         G_ISIM_BINDINGS.data(),
                         G_ISIM_REGISTRY,
                         SYS_PSS);
}

void ensure_logger_initialized() { // Init PSS logger
  if (G_LOGGER_READY) {
    return;
  }

  const common::log::init_config_t cfg{
      .min_level = common::log::level_t::info,
      .truncate_on_init = true,
  };
  G_LOGGER_READY = common::log::init_for_module("pss", cfg);
  if (G_LOGGER_READY) {
    common::log::info("logger initialized");
  }
}

void log_isim_status(const common::log::level_t level,
                     const std::string_view prefix,
                     const isim::status_t status,
                     const std::uint16_t id) { // Log ISIM status
  if (!G_LOGGER_READY) {
    return;
  }

  std::ostringstream oss;
  oss << prefix << "id=" << id << " status=" << isim::to_string(status);
  common::log::write(level, oss.str());
}

std::string format_msg_words(const std::uint16_t* data, const std::size_t count) { // Format MSG words for logging
  if ((data == nullptr) || (count == 0U)) {
    return "[]";
  }

  std::ostringstream oss;
  oss << '[';
  for (std::size_t i = 0; i < count; ++i) {
    if (i != 0U) {
      oss << ' ';
    }
    oss << std::setw(2) << std::setfill('0') << i << ":0x" << std::hex << std::uppercase << std::setw(4)
        << std::setfill('0') << data[i] << std::dec;
  }
  oss << ']';
  return oss.str();
}

void log_msg_snapshot(const std::string_view stage) { // Log MSG_COM/MSG_PAR snapshot
  if (!G_LOGGER_READY) {
    return;
  }

  const auto& ctx = G_IPAR_CTX;
  std::ostringstream oss;
  oss << "msg snapshot [" << stage << "]: "
      << "MSG_COM(" << ctx.msg_com_blocks << ")=" << format_msg_words(ctx.msg_com, ctx.msg_com_blocks) << ' '
      << "MSG_PAR(" << ctx.msg_par_blocks << ")=" << format_msg_words(ctx.msg_par, ctx.msg_par_blocks);
  common::log::info(oss.str());
}

void log_tick_state() { // Log tick counters
  if (!G_LOGGER_READY) {
    return;
  }

  std::ostringstream oss;
  oss << "tick: count=" << G_RUNTIME.tick.count << " clock=" << static_cast<unsigned>(G_RUNTIME.tick.clock);
  common::log::info(oss.str());
}

std::uint8_t normalize_runtime_clock(const std::int32_t raw) { // Normalize clock to 1..255
  if (raw <= 0) {
    return 1U;
  }
  if (raw > 255) {
    return 255U;
  }
  return static_cast<std::uint8_t>(raw);
}

bool export_override(void*, const std::uint16_t id, std::int32_t& out_value) {
  if (id != PSSCLOCK) {
    return false;
  }

  out_value = static_cast<std::int32_t>(G_RUNTIME.tick.clock);
  return true;
}

void log_command_status(void*, const icmdexec::binding_t& binding, const isim::status_t status) {
  log_isim_status(common::log::level_t::warning, "ISIMSET failed: ", status, binding.param_id);
}

void apply_runtime_commands() { // Apply non-power commands from MSG_COM
  const auto& ctx = G_IPAR_CTX;
  (void)icmdexec::apply_all(ctx,
                            k_runtime_bool_commands.data(),
                            k_runtime_bool_commands.size(),
                            log_command_status,
                            nullptr);
  (void)icmdexec::apply_all(ctx,
                            k_runtime_value_commands.data(),
                            k_runtime_value_commands.size(),
                            log_command_status,
                            nullptr);

  const auto clock_value = icmd::ICMDVAL(ctx, CPSSCLOCK, 0);
  if (clock_value != 0) {
    G_RUNTIME.tick.clock = normalize_runtime_clock(clock_value);
  }
}

void apply_power_commands() { // Apply power commands from MSG_COM
  const auto& ctx = G_IPAR_CTX;
  (void)icmdexec::apply_all(ctx,
                            k_power_commands.data(),
                            k_power_commands.size(),
                            log_command_status,
                            nullptr);

  std::int32_t main_pwr = 0;
  if (isim::ISIMPARI32(PSSPWR, &main_pwr) != isim::status_t::ok) {
    main_pwr = 0;
  }
  if (main_pwr == 0) {
    (void)isim::ISIMSETI32(PSSPWR12, 0);
    (void)isim::ISIMSETI32(PSSPWR6, 0);
    (void)isim::ISIMSETI32(PSSPWR5, 0);
    (void)isim::ISIMSETI32(PSSPWR33, 0);
  }
}

bool is_power_on() { // Read main power state
  std::int32_t main_pwr = 0;
  if (isim::ISIMPARI32(PSSPWR, &main_pwr) != isim::status_t::ok) {
    return false;
  }
  return main_pwr != 0;
}

void local_algorithm(void*) { // Step model and refresh MSG_PAR/MSG_PAR_BUF
  isysalgo::ISYS_EXPORT_MODEL_PARAMS(G_IPAR_CTX, PSS_BASE, PSS_COUNT, export_override, nullptr);
  isysalgo::ISYS_SYNC_MSGPAR_TO_BUF(G_BUS);
}

void cmd_update(void*) { // Handle non-power commands
  apply_runtime_commands();
}

void cmd_pwr_update(void*) { // Handle power commands
  apply_power_commands();
}

void logging_hook(void*) { // Emit runtime logs
  log_tick_state();
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
    .cmd_update = cmd_update,
    .local_algorithm = local_algorithm,
    .exchange_control = nullptr,
    .cmd_pwr_update = cmd_pwr_update,
    .logging = logging_hook,
};

}

void init_msg_buffers() { // Clear MSG buffers
  init_bus_state();
  isys::ISYSCLEAR(MSG_PAR.data(), MSG_PAR.size());
  isys::ISYSCLEAR(MSG_COM.data(), MSG_COM.size());
  isys::ISYSCLEAR(MSG_COM_BUF.u16.data(), MSG_COM_BUF.u16.size());
  isys::ISYSCLEAR(MSG_PAR_BUF.u16.data(), MSG_PAR_BUF.u16.size());
  MSG_HEADER.u16[0] = 0;
}

void bind_ipar_context() { // Bind IPAR context for this thread
  ipar::bind_context(&G_IPAR_CTX);
}

const ipar::context_t& ipar_context() { // Return current IPAR context
  return G_IPAR_CTX;
}

void runtime_init() { // Initialize PSS runtime state
  ensure_logger_initialized();
  init_msg_buffers();
  bind_ipar_context();
  init_isim_registry();

  G_RUNTIME = runtime_state_t{};
  G_RUNTIME.initialized = true;

  if (G_LOGGER_READY) {
    common::log::info("runtime initialized");
    log_msg_snapshot("runtime init");
  }
}

void runtime_step(const std::uint32_t tick) { // Execute one PSS runtime tick
  if (!G_RUNTIME.initialized) {
    runtime_init();
  }

  G_RUNTIME.current_tick = tick;
  G_BUS.writer_tick = tick;

  const bool power_on = is_power_on();
  isysalgo::ISYSBASE_STEP(G_RUNTIME.tick,
                          power_on,
                          G_BUS,
                          k_bus_hooks,
                          k_base_hooks,
                          nullptr);
}

}
