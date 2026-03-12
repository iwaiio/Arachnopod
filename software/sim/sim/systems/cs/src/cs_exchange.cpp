#include "cs_exchange.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

#include "common.hpp"
#include "cs.hpp"
#include "cs_bus_control.hpp"
#include "i_control_console.hpp"
#include "param_comm_list.hpp"
#include "sys_list.hpp"

namespace cs::exchange {
namespace {

constexpr std::uint32_t k_exchange_period_ticks = 50U;
constexpr std::uint32_t k_exchange_timeout_frames = ARP_BUS_RESPONSE_TIMEOUT_FRAMES;

struct pending_cmd_t {
  std::uint16_t id{0};
  std::int32_t value{0};
};

struct state_t {
  bool initialized{false};
  bool enabled{true};
  bool init_sent{false};
  bool force_exchange{false};
  std::uint32_t last_exchange_tick{0U};
  std::uint8_t target_addr{ARP_ADDR_PSS};
  std::deque<pending_cmd_t> pending_cmds{};
};

state_t G_STATE{};

struct target_info_t {
  std::string_view name;
  std::uint8_t addr;
  std::size_t comm_blocks;
  std::size_t param_blocks;
};

constexpr target_info_t k_targets[] = {
    {"pss", ARP_ADDR_PSS, PSS_COMM_MSG_BLOCKS, PSS_PARAM_MSG_BLOCKS},
    {"tcs", ARP_ADDR_TCS, TCS_COMM_MSG_BLOCKS, TCS_PARAM_MSG_BLOCKS},
    {"tms", ARP_ADDR_TMS, TMS_COMM_MSG_BLOCKS, TMS_PARAM_MSG_BLOCKS},
    {"mns", ARP_ADDR_MNS, MNS_COMM_MSG_BLOCKS, MNS_PARAM_MSG_BLOCKS},
    {"ls", ARP_ADDR_LS, LS_COMM_MSG_BLOCKS, LS_PARAM_MSG_BLOCKS},
};

std::string to_lower_copy(std::string s) {
  for (char& ch : s) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return s;
}

std::optional<target_info_t> find_target_by_name(const std::string& name) {
  const std::string key = to_lower_copy(name);
  for (const auto& info : k_targets) {
    if (key == info.name) {
      return info;
    }
  }
  return std::nullopt;
}

const target_info_t* find_target_by_addr(const std::uint8_t addr) {
  for (const auto& info : k_targets) {
    if (info.addr == addr) {
      return &info;
    }
  }
  return nullptr;
}

bool is_pss_target(const std::uint8_t addr) {
  return addr == ARP_ADDR_PSS;
}

void log_msg_snapshot(const std::string_view stage) {
  if (!common::log::is_initialized()) {
    return;
  }

  const auto& ctx = ipar_context();
  std::ostringstream oss;
  oss << "msg snapshot [" << stage << "]: "
      << "MSG_COM(" << ctx.msg_com_blocks << ")=";

  if (ctx.msg_com && ctx.msg_com_blocks) {
    oss << '[';
    for (std::size_t i = 0; i < ctx.msg_com_blocks; ++i) {
      if (i != 0U) {
        oss << ' ';
      }
      oss << std::setw(2) << std::setfill('0') << i << ":0x" << std::hex << std::uppercase << std::setw(4)
          << std::setfill('0') << ctx.msg_com[i] << std::dec;
    }
    oss << ']';
  } else {
    oss << "[]";
  }

  oss << " MSG_PAR(" << ctx.msg_par_blocks << ")=";
  if (ctx.msg_par && ctx.msg_par_blocks) {
    oss << '[';
    for (std::size_t i = 0; i < ctx.msg_par_blocks; ++i) {
      if (i != 0U) {
        oss << ' ';
      }
      oss << std::setw(2) << std::setfill('0') << i << ":0x" << std::hex << std::uppercase << std::setw(4)
          << std::setfill('0') << ctx.msg_par[i] << std::dec;
    }
    oss << ']';
  } else {
    oss << "[]";
  }

  common::log::info(oss.str());
}

void set_pss_oneshot_commands(const std::int32_t value) {
  const auto& ctx = ipar_context();
  (void)ipar::IGEN(ctx, CPSSINIT, value);
  (void)ipar::IGEN(ctx, CPSSPWRON, value);
  (void)ipar::IGEN(ctx, CPSSPWR12ON, value);
  (void)ipar::IGEN(ctx, CPSSPWR6ON, value);
  (void)ipar::IGEN(ctx, CPSSPWR5ON, value);
  (void)ipar::IGEN(ctx, CPSSPWR33ON, value);
}

void set_pss_config_commands() {
  const auto& ctx = ipar_context();
  (void)ipar::IGEN(ctx, CPSSMAXT1, 70);
  (void)ipar::IGEN(ctx, CPSSMAXT2, 70);
  (void)ipar::IGEN(ctx, CPSSMAXA, 120);
  (void)ipar::IGEN(ctx, CPSSMAXW, 220);
  (void)ipar::IGEN(ctx, CPSSMINC, 20);
  (void)ipar::IGEN(ctx, CPSSMINV, 90);
}

void apply_pending_commands(const std::deque<pending_cmd_t>& cmds) {
  const auto& ctx = ipar_context();
  for (const auto& cmd : cmds) {
    (void)ipar::IGEN(ctx, cmd.id, cmd.value);
  }
}

void clear_pending_commands(const std::deque<pending_cmd_t>& cmds) {
  const auto& ctx = ipar_context();
  for (const auto& cmd : cmds) {
    (void)ipar::IGEN(ctx, cmd.id, 0);
  }
}

void fill_tx_blocks(const target_info_t& target,
                    std::array<std::uint16_t, ARP_BUS_MAX_DATA_BLOCKS>& tx_blocks,
                    std::uint8_t& tx_block_count) {
  const auto& ctx = ipar_context();
  tx_block_count = static_cast<std::uint8_t>(
      std::min<std::size_t>({ctx.msg_com_blocks, target.comm_blocks, tx_blocks.size()}));

  for (std::size_t i = 0; i < tx_block_count; ++i) {
    tx_blocks[i] = ctx.msg_com[i];
  }
}

void copy_rsp_to_cs_msg_par(const target_info_t& target, const bus_control::exchange_response_t& rsp) {
  const auto& ctx = ipar_context();
  const std::size_t n = std::min<std::size_t>({static_cast<std::size_t>(rsp.rx_block_count), ctx.msg_par_blocks,
                                               target.param_blocks, ARP_BUS_MAX_DATA_BLOCKS});

  for (std::size_t i = 0; i < n; ++i) {
    ctx.msg_par[i] = rsp.rx_blocks[i];
  }
}

void parse_pss_telemetry_for_validation() {
  const auto& ctx = ipar_context();
  (void)ipar::IPAR(ctx, PSSPWR);
  (void)ipar::IPAR(ctx, PSSCLOCK);
  (void)ipar::IPAR(ctx, PSST1);
  (void)ipar::IPAR(ctx, PSSA);
  (void)ipar::IPAR(ctx, PSSV);
  (void)ipar::IPAR(ctx, PSSC);
}

void run_exchange_cycle(const std::uint32_t tick) {
  const auto* target = find_target_by_addr(G_STATE.target_addr);
  if (target == nullptr) {
    if (common::log::is_initialized()) {
      common::log::warning("exchange: target address not supported");
    }
    return;
  }

  if (!is_pss_target(target->addr)) {
    if (common::log::is_initialized()) {
      std::ostringstream oss;
      oss << "exchange: target " << target->name << " not implemented yet";
      common::log::warning(oss.str());
    }
    return;
  }

  bus_control::exchange_request_t req_uv{};
  req_uv.dst_addr = target->addr;
  req_uv.flag = ARP_FLAG_UV;
  req_uv.marker = static_cast<std::uint8_t>(tick & 0x07U);
  fill_tx_blocks(*target, req_uv.tx_blocks, req_uv.tx_block_count);
  log_msg_snapshot("before UV tx");

  const auto uv_rsp = bus_control::exchange_once(req_uv, tick, k_exchange_timeout_frames);
  if (uv_rsp.status != bus_control::exchange_status_t::ok) {
    log_msg_snapshot("UV tx failed");
    return;
  }

  bus_control::exchange_request_t req_data{};
  req_data.dst_addr = target->addr;
  req_data.flag = ARP_FLAG_REQ_DATA;
  req_data.marker = static_cast<std::uint8_t>((tick + 1U) & 0x07U);
  req_data.tx_block_count = 0;

  const auto data_rsp = bus_control::exchange_once(req_data, tick, k_exchange_timeout_frames);
  if (data_rsp.status != bus_control::exchange_status_t::ok) {
    log_msg_snapshot("REQ_DATA tx failed");
    return;
  }

  copy_rsp_to_cs_msg_par(*target, data_rsp);
  parse_pss_telemetry_for_validation();
  log_msg_snapshot("after REQ_DATA rx");
}

bool parse_int(const std::string& token, std::int32_t& out) {
  if (token.empty()) {
    return false;
  }
  char* end = nullptr;
  const long value = std::strtol(token.c_str(), &end, 0);
  if (end == token.c_str() || (end != nullptr && *end != '\0')) {
    return false;
  }
  out = static_cast<std::int32_t>(value);
  return true;
}

std::optional<std::uint16_t> resolve_pss_command_id(const std::string& name) {
  if (name.empty()) {
    return std::nullopt;
  }

  std::int32_t numeric = 0;
  if (parse_int(name, numeric)) {
    if (numeric < 0 || numeric > 0xFFFF) {
      return std::nullopt;
    }
    return static_cast<std::uint16_t>(numeric);
  }

  if (name == "CPSSINIT") return CPSSINIT;
  if (name == "CPSSPWRON") return CPSSPWRON;
  if (name == "CPSSPWROFF") return CPSSPWROFF;
  if (name == "CPSSPWR12ON") return CPSSPWR12ON;
  if (name == "CPSSPWR12OFF") return CPSSPWR12OFF;
  if (name == "CPSSPWR6ON") return CPSSPWR6ON;
  if (name == "CPSSPWR6OFF") return CPSSPWR6OFF;
  if (name == "CPSSPWR5ON") return CPSSPWR5ON;
  if (name == "CPSSPWR5OFF") return CPSSPWR5OFF;
  if (name == "CPSSPWR33ON") return CPSSPWR33ON;
  if (name == "CPSSPWR33OFF") return CPSSPWR33OFF;
  if (name == "CPSSECOON") return CPSSECOON;
  if (name == "CPSSECOOFF") return CPSSECOOFF;
  if (name == "CPSSMAXT1") return CPSSMAXT1;
  if (name == "CPSSMAXT2") return CPSSMAXT2;
  if (name == "CPSSMAXA") return CPSSMAXA;
  if (name == "CPSSMAXW") return CPSSMAXW;
  if (name == "CPSSMINC") return CPSSMINC;
  if (name == "CPSSMINV") return CPSSMINV;
  if (name == "CPSSMAXA12") return CPSSMAXA12;
  if (name == "CPSSMAXA6") return CPSSMAXA6;
  if (name == "CPSSMAXA5") return CPSSMAXA5;
  if (name == "CPSSMAXA33") return CPSSMAXA33;
  if (name == "CPSSCLOCK") return CPSSCLOCK;

  return std::nullopt;
}

void handle_control_command(const control::command_t& cmd) {
  if (cmd.verb == "help") {
    common::log::info("control: commands: exchange <pss>, cmd <pss> <CPSS...> [value], enable, disable");
    return;
  }

  if (cmd.verb == "enable") {
    set_enabled(true);
    return;
  }

  if (cmd.verb == "disable") {
    set_enabled(false);
    return;
  }

  if (cmd.verb == "exchange" || cmd.verb == "ex") {
    if (!cmd.arg0.empty()) {
      const auto target = find_target_by_name(cmd.arg0);
      if (target.has_value()) {
        G_STATE.target_addr = target->addr;
      } else if (common::log::is_initialized()) {
        common::log::warning("exchange: unknown target " + cmd.arg0);
      }
    }
    request_exchange();
    return;
  }

  if (cmd.verb == "cmd") {
    if (cmd.arg0.empty() || cmd.arg1.empty()) {
      if (common::log::is_initialized()) {
        common::log::warning("cmd: expected format cmd <pss> <CPSS...> [value]");
      }
      return;
    }

    const auto target = find_target_by_name(cmd.arg0);
    if (!target.has_value()) {
      if (common::log::is_initialized()) {
        common::log::warning("cmd: unknown target " + cmd.arg0);
      }
      return;
    }

    G_STATE.target_addr = target->addr;
    if (!is_pss_target(G_STATE.target_addr)) {
      if (common::log::is_initialized()) {
        common::log::warning("cmd: target not implemented yet");
      }
      return;
    }

    const auto id = resolve_pss_command_id(cmd.arg1);
    if (!id.has_value()) {
      if (common::log::is_initialized()) {
        common::log::warning("cmd: unknown command " + cmd.arg1);
      }
      return;
    }

    const std::int32_t value = cmd.has_value ? cmd.value : 1;
    queue_command(*id, value);
    request_exchange();
    return;
  }
}

void process_control_commands() {
  if (!control::is_running()) {
    return;
  }

  control::command_t cmd{};
  while (control::pop_command(control::target_t::cs, cmd)) {
    handle_control_command(cmd);
  }
}

}  // namespace

void init() {
  if (G_STATE.initialized) {
    return;
  }

  G_STATE = state_t{};
  G_STATE.initialized = true;
  G_STATE.enabled = true;
  G_STATE.target_addr = ARP_ADDR_PSS;

  set_pss_oneshot_commands(0);
  set_pss_config_commands();
  log_msg_snapshot("exchange init");
}

void step(const std::uint32_t tick) {
  if (!G_STATE.initialized) {
    init();
  }

  process_control_commands();

  if (!G_STATE.enabled && !G_STATE.force_exchange && G_STATE.pending_cmds.empty()) {
    return;
  }

  if (!G_STATE.force_exchange && G_STATE.pending_cmds.empty()) {
    if ((tick - G_STATE.last_exchange_tick) < k_exchange_period_ticks) {
      return;
    }
  }

  if (!G_STATE.init_sent) {
    set_pss_oneshot_commands(1);
  }

  set_pss_config_commands();
  apply_pending_commands(G_STATE.pending_cmds);

  run_exchange_cycle(tick);

  if (!G_STATE.init_sent) {
    set_pss_oneshot_commands(0);
    G_STATE.init_sent = true;
  }

  if (!G_STATE.pending_cmds.empty()) {
    clear_pending_commands(G_STATE.pending_cmds);
    G_STATE.pending_cmds.clear();
  }

  G_STATE.force_exchange = false;
  G_STATE.last_exchange_tick = tick;
}

void set_enabled(const bool enabled) {
  G_STATE.enabled = enabled;
}

bool is_enabled() {
  return G_STATE.enabled;
}

void request_exchange() {
  G_STATE.force_exchange = true;
}

void queue_command(const std::uint16_t cmd_id, const std::int32_t value) {
  G_STATE.pending_cmds.push_back(pending_cmd_t{cmd_id, value});
}

}  // namespace cs::exchange
