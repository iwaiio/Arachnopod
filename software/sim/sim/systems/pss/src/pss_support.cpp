#include "pss_internal.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>

#include "../../base/param_comm_list.hpp"
#include "common.hpp"
#include "i_cmd.hpp"
#include "i_isim_registry.hpp"
#include "sim_pss_base.hpp"

namespace pss::internal {
namespace {

ipar::context_t make_context() {
  return isystools::make_imsg_context(ipar::role_t::device, SYS_PSS, G_WIRE);
}

void log_isim_status(const common::log::level_t level,
                     const std::string_view prefix,
                     const isim::status_t status,
                     const std::uint16_t id) {
  if (!G_LOGGER_READY) {
    return;
  }

  std::ostringstream oss;
  oss << prefix << "id=" << id << " status=" << isim::to_string(status);
  common::log::write(level, oss.str());
}

std::uint8_t normalize_system_clock(const std::int32_t raw) {
  if (raw <= 0) {
    return 1U;
  }
  if (raw > 255) {
    return 255U;
  }
  return static_cast<std::uint8_t>(raw);
}

bool export_override(void*, const std::uint16_t id, float& out_value) {
  if (id != PSSCLOCK) {
    return false;
  }

  out_value = static_cast<float>(G_SOFTWARE.tick.clock);
  return true;
}

void log_command_status(const std::uint16_t param_id, const isim::status_t status) {
  log_isim_status(common::log::level_t::warning, "ISIMSET failed: ", status, param_id);
}

bool apply_i32_command(const std::uint16_t command_id, const std::uint16_t param_id, const std::int32_t value) {
  const auto& ctx = G_IMSG_CTX;
  if (!icmd::ICMDACT(ctx, command_id)) {
    return false;
  }

  const auto status = isim::ISIMSETI32(param_id, value);
  if (status != isim::status_t::ok) {
    log_command_status(param_id, status);
    return false;
  }

  icmd::ICMDCLEAR(ctx, command_id);
  return true;
}

bool apply_f32_command(const std::uint16_t command_id, const std::uint16_t param_id) {
  const auto& ctx = G_IMSG_CTX;
  if (!icmd::ICMDACT(ctx, command_id)) {
    return false;
  }

  const float value = icmd::ICMDF32(ctx, command_id, 0.0F);
  const auto status = isim::ISIMSETF32(param_id, value);
  if (status != isim::status_t::ok) {
    log_command_status(param_id, status);
    return false;
  }

  icmd::ICMDCLEAR(ctx, command_id);
  return true;
}

void execute_system_command(const std::uint16_t command_id) {
  const auto& ctx = G_IMSG_CTX;
  if (!icmd::ICMDACT(ctx, command_id)) {
    return;
  }

  switch (command_id) {
    case CPSSINIT:
      sim_pss_base::init_defaults();
      icmd::ICMDCLEAR(ctx, command_id);
      return;
    case CPSSPWR12ON:
      (void)apply_i32_command(command_id, PSSPWR12, 1);
      return;
    case CPSSPWR12OFF:
      (void)apply_i32_command(command_id, PSSPWR12, 0);
      return;
    case CPSSPWR6ON:
      (void)apply_i32_command(command_id, PSSPWR6, 1);
      return;
    case CPSSPWR6OFF:
      (void)apply_i32_command(command_id, PSSPWR6, 0);
      return;
    case CPSSPWR5ON:
      (void)apply_i32_command(command_id, PSSPWR5, 1);
      return;
    case CPSSPWR5OFF:
      (void)apply_i32_command(command_id, PSSPWR5, 0);
      return;
    case CPSSPWR33ON:
      (void)apply_i32_command(command_id, PSSPWR33, 1);
      return;
    case CPSSPWR33OFF:
      (void)apply_i32_command(command_id, PSSPWR33, 0);
      return;
    case CPSSECOON:
      (void)apply_i32_command(command_id, PSSECO, 1);
      return;
    case CPSSECOOFF:
      (void)apply_i32_command(command_id, PSSECO, 0);
      return;
    case CPSSMAXT1: {
      const auto status = isim::ISIMSETI32(PSSMAXT1, icmd::ICMDVAL(ctx, command_id, 0));
      if (status == isim::status_t::ok) {
        icmd::ICMDCLEAR(ctx, command_id);
      } else {
        log_command_status(PSSMAXT1, status);
      }
      return;
    }
    case CPSSMAXT2: {
      const auto status = isim::ISIMSETI32(PSSMAXT2, icmd::ICMDVAL(ctx, command_id, 0));
      if (status == isim::status_t::ok) {
        icmd::ICMDCLEAR(ctx, command_id);
      } else {
        log_command_status(PSSMAXT2, status);
      }
      return;
    }
    case CPSSMAXA:
      (void)apply_f32_command(command_id, PSSMAXA);
      return;
    case CPSSMAXW:
      (void)apply_f32_command(command_id, PSSMAXW);
      return;
    case CPSSMINC: {
      const auto status = isim::ISIMSETI32(PSSMINC, icmd::ICMDVAL(ctx, command_id, 0));
      if (status == isim::status_t::ok) {
        icmd::ICMDCLEAR(ctx, command_id);
      } else {
        log_command_status(PSSMINC, status);
      }
      return;
    }
    case CPSSMINV:
      (void)apply_f32_command(command_id, PSSMINV);
      return;
    case CPSSMAXA12:
      (void)apply_f32_command(command_id, PSSMAXA12);
      return;
    case CPSSMAXA6:
      (void)apply_f32_command(command_id, PSSMAXA6);
      return;
    case CPSSMAXA5:
      (void)apply_f32_command(command_id, PSSMAXA5);
      return;
    case CPSSMAXA33:
      (void)apply_f32_command(command_id, PSSMAXA33);
      return;
    case CPSSCLOCK: {
      G_SOFTWARE.tick.clock = normalize_system_clock(icmd::ICMDVAL(ctx, command_id, 0));
      icmd::ICMDCLEAR(ctx, command_id);
      return;
    }
    case CPSSPWRON:
    case CPSSPWROFF:
      return;
    default:
      return;
  }
}

void apply_system_commands() {
  for (std::uint16_t command_id = CPSS_BASE; command_id < static_cast<std::uint16_t>(CPSS_BASE + CPSS_COUNT); ++command_id) {
    if ((command_id == CPSSPWRON) || (command_id == CPSSPWROFF)) {
      continue;
    }
    execute_system_command(command_id);
  }
}

void execute_power_command(const std::uint16_t command_id) {
  switch (command_id) {
    case CPSSPWRON:
      (void)apply_i32_command(command_id, PSSPWR, 1);
      return;
    case CPSSPWROFF:
      (void)apply_i32_command(command_id, PSSPWR, 0);
      return;
    default:
      return;
  }
}

void apply_power_commands() {
  for (const std::uint16_t command_id : {CPSSPWRON, CPSSPWROFF}) {
    execute_power_command(command_id);
  }

  std::int32_t main_pwr = 0;
  if (isim::ISIMGETI32(PSSPWR, &main_pwr) != isim::status_t::ok) {
    main_pwr = 0;
  }
  if (main_pwr == 0) {
    (void)isim::ISIMSETI32(PSSPWR12, 0);
    (void)isim::ISIMSETI32(PSSPWR6, 0);
    (void)isim::ISIMSETI32(PSSPWR5, 0);
    (void)isim::ISIMSETI32(PSSPWR33, 0);
  }
}

}  // namespace

bool G_LOGGER_READY = false;
wire_storage_t G_WIRE{};
ipar::context_t G_IMSG_CTX = make_context();
software_state_t G_SOFTWARE{};
isysalgo::bus_state_t G_BUS{};
bool G_BUS_READY = false;
std::uint64_t G_LAST_TICK_LOG_COUNT = 0U;
std::array<isim::binding_t, static_cast<std::size_t>(PSS_COUNT)> G_ISIM_BINDINGS{};
isim::registry_t G_ISIM_REGISTRY{};

void init_bus_state() {
  if (G_BUS_READY) {
    return;
  }

  isystools::bind_standard_bus(
      G_BUS, isysalgo::bus_role_t::device, SYS_PSS, ARP_ADDR_PSS, ARP_ADDR_CS, G_WIRE);

  G_BUS_READY = true;
}

void init_isim_registry() {
  isystools::build_isim_registry(
      sim_pss_base::ISIM_PARAMS.data(), sim_pss_base::ISIM_PARAMS.size(), G_ISIM_BINDINGS, G_ISIM_REGISTRY, SYS_PSS);
}

void ensure_logger_initialized() {
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

void ILOG(void*) {
  if (!G_LOGGER_READY) {
    return;
  }

  std::ostringstream oss;
  oss << "tick: count=" << G_SOFTWARE.tick.count << " clock=" << static_cast<unsigned>(G_SOFTWARE.tick.clock)
      << " power=" << (is_power_on() ? 1 : 0) << ' '
      << "MSG_CMD=" << isystools::format_msg_words(G_IMSG_CTX.msg_cmd, G_IMSG_CTX.msg_cmd_blocks) << ' '
      << "MSG_PRM=" << isystools::format_msg_words(G_IMSG_CTX.msg_prm, G_IMSG_CTX.msg_prm_blocks);
  common::log::info(oss.str());
}

bool is_power_on() {
  std::int32_t main_pwr = 0;
  if (isim::ISIMGETI32(PSSPWR, &main_pwr) != isim::status_t::ok) {
    return false;
  }
  return main_pwr != 0;
}

void IPRMUPD(void*) {
  isysalgo::ISYS_EXPORT_MODEL_PARAMS(G_IMSG_CTX, PSS_BASE, PSS_COUNT, export_override, nullptr);
  isysalgo::ISYS_SYNC_MSGPRM_TO_BUF(G_BUS);
}

void ICMDUPD(void*) {
  apply_system_commands();
}

void LOCALSYSALG(void*) {}

void ISYSNODES(void*) {}

void ICMDPWRUPD(void*) {
  apply_power_commands();
}

const isysalgo::bus_hooks_t k_bus_hooks{
    .parse_header = nullptr,
    .gen_header = nullptr,
    .bypass = nullptr,
    .log_exchange = nullptr,
    .on_error = nullptr,
};

const isysalgo::base_hooks_t k_base_hooks{
    .ISYSALG =
        {
            .ICMDUPD = ICMDUPD,
            .LOCALSYSALG = LOCALSYSALG,
            .ISYSNODES = ISYSNODES,
            .IPRMUPD = IPRMUPD,
        },
    .IBUSEXCHANGE = nullptr,
    .IEXCHANGECTRL = nullptr,
    .ICMDPWRUPD = ICMDPWRUPD,
    .ILOG = ILOG,
};

}  // namespace pss::internal
