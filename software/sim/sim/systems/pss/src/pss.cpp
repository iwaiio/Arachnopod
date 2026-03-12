#include "pss.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>

#include "../../base/param_comm_list.hpp"
#include "../../base/sys_id_range.hpp"
#include "../../bus_mock/interface/i_mock_bus.hpp"
#include "../../../simulation/pss/interface/i_pss_sim.hpp"
#include "common.hpp"
#include "i_sim.hpp"
#include "sim_pss_base.hpp"

namespace pss {
namespace {

constexpr std::uint8_t k_pss_addr = ARP_ADDR_PSS;
constexpr std::uint8_t k_cs_addr = ARP_ADDR_CS;
constexpr std::uint32_t k_rx_poll_timeout_ms = 1U;
constexpr std::uint16_t k_param_id_base = static_cast<std::uint16_t>(PSS_BASE);
constexpr std::size_t k_param_id_count = static_cast<std::size_t>(PSS_COUNT);

bool G_LOGGER_READY = false;

std::array<std::uint16_t, PSS_PARAM_MSG_BLOCKS> MSG_PAR{};
std::array<std::uint16_t, PSS_COMM_MSG_BLOCKS> MSG_COM{};

ipar::context_t make_context() {
  ipar::context_t ctx{};
  ctx.role = ipar::role_t::device;
  ctx.system_id = SYS_PSS;
  ctx.msg_par = MSG_PAR.data();
  ctx.msg_par_blocks = MSG_PAR.size();
  ctx.msg_com = MSG_COM.data();
  ctx.msg_com_blocks = MSG_COM.size();
  return ctx;
}

ipar::context_t G_IPAR_CTX = make_context();

struct runtime_state_t {
  bool initialized{false};
  bool tx_pending{false};
  mock_bus_frame_t tx_frame{};
  std::uint64_t count{0U};
  std::uint8_t clock{0U};
};

runtime_state_t G_RUNTIME{};

std::array<isim::binding_t, k_param_id_count> G_ISIM_BINDINGS{};
isim::registry_t G_ISIM_REGISTRY{};

isim::value_kind_t kind_from_param_entry(const sim_base::param_entry_t& entry) {
  if (entry.bits >= 16U) {
    return entry.is_signed ? isim::value_kind_t::s16 : isim::value_kind_t::u16;
  }
  return entry.is_signed ? isim::value_kind_t::s8 : isim::value_kind_t::u8;
}

std::int32_t value_to_i32(const isim::value_t& value) {
  switch (value.kind) {
    case isim::value_kind_t::u8:
      return static_cast<std::int32_t>(value.data.u8);
    case isim::value_kind_t::s8:
      return static_cast<std::int32_t>(value.data.s8);
    case isim::value_kind_t::u16:
      return static_cast<std::int32_t>(value.data.u16);
    case isim::value_kind_t::s16:
      return static_cast<std::int32_t>(value.data.s16);
    case isim::value_kind_t::u32:
      return static_cast<std::int32_t>(value.data.u32);
    case isim::value_kind_t::s32:
      return value.data.s32;
    case isim::value_kind_t::f32:
      return static_cast<std::int32_t>(std::lround(value.data.f32));
    default:
      return 0;
  }
}

const isim::binding_t* find_binding_by_id(const std::uint16_t id) {
  for (const auto& binding : G_ISIM_BINDINGS) {
    if (binding.id == id) {
      return &binding;
    }
  }
  return nullptr;
}

std::int32_t clamp_to_range(const std::int32_t value, const std::int32_t lo, const std::int32_t hi) {
  return std::clamp(value, lo, hi);
}

isim::status_t isim_set_i32(const std::uint16_t id, const std::int32_t value) {
  const auto* binding = find_binding_by_id(id);
  if (binding == nullptr) {
    return isim::status_t::id_not_found;
  }

  isim::value_t payload{};
  payload.kind = binding->kind;
  switch (binding->kind) {
    case isim::value_kind_t::u8:
      payload.data.u8 = static_cast<std::uint8_t>(clamp_to_range(value, 0, 255));
      break;
    case isim::value_kind_t::s8:
      payload.data.s8 = static_cast<std::int8_t>(clamp_to_range(value, -128, 127));
      break;
    case isim::value_kind_t::u16:
      payload.data.u16 = static_cast<std::uint16_t>(clamp_to_range(value, 0, 65535));
      break;
    case isim::value_kind_t::s16:
      payload.data.s16 = static_cast<std::int16_t>(clamp_to_range(value, -32768, 32767));
      break;
    case isim::value_kind_t::u32:
      if (value < 0) {
        return isim::status_t::value_out_of_range;
      }
      payload.data.u32 = static_cast<std::uint32_t>(value);
      break;
    case isim::value_kind_t::s32:
      payload.data.s32 = value;
      break;
    case isim::value_kind_t::f32:
      payload.data.f32 = static_cast<float>(value);
      break;
    default:
      return isim::status_t::type_mismatch;
  }

  return isim::ISIMSET(id, &payload);
}

isim::status_t isim_get_i32(const std::uint16_t id, std::int32_t* out_value) {
  if (out_value == nullptr) {
    return isim::status_t::bad_argument;
  }

  isim::value_t value{};
  const auto status = isim::ISIMPAR(id, &value);
  if (status != isim::status_t::ok) {
    return status;
  }

  *out_value = value_to_i32(value);
  return isim::status_t::ok;
}

void init_isim_registry() {
  for (std::size_t i = 0; i < k_param_id_count; ++i) {
    const auto& entry = sim_pss_base::PARAMS[i];
    G_ISIM_BINDINGS[i] = isim::binding_t{
        .id = entry.id,
        .kind = kind_from_param_entry(entry),
        .ptr = entry.ptr,
    };
  }

  G_ISIM_REGISTRY = isim::registry_t{
      .bindings = G_ISIM_BINDINGS.data(),
      .count = G_ISIM_BINDINGS.size(),
      .system_id = SYS_PSS,
  };
  isim::bind_registry(&G_ISIM_REGISTRY);
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

void log_bus_status(const common::log::level_t level,
                    const std::string_view prefix,
                    const mock_bus_status_t status) {
  if (!G_LOGGER_READY) {
    return;
  }

  std::string msg(prefix);
  msg += mock_bus_status_to_string(status);
  common::log::write(level, msg);
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

const char* flag_to_string(const std::uint8_t flag) {
  switch (flag) {
    case ARP_FLAG_UV:
      return "UV";
    case ARP_FLAG_UV_RESP:
      return "UV_RESP";
    case ARP_FLAG_REQ_DATA:
      return "REQ_DATA";
    case ARP_FLAG_REQ_DATA_RESP:
      return "REQ_DATA_RESP";
    default:
      return "UNKNOWN";
  }
}

std::string format_blocks(const std::array<std::uint16_t, ARP_BUS_MAX_DATA_BLOCKS>& blocks,
                          const std::uint8_t count) {
  std::ostringstream oss;
  oss << '[';
  const std::size_t n = std::min<std::size_t>(count, 8U);
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0U) {
      oss << ' ';
    }
    oss << "0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << blocks[i] << std::dec;
  }
  if (count > n) {
    oss << " ...";
  }
  oss << ']';
  return oss.str();
}

std::string format_msg_words(const std::uint16_t* data, const std::size_t count) {
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

void log_msg_snapshot(const std::string_view stage) {
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

void log_bus_frame(const std::string_view direction, const mock_bus_frame_t& frame) {
  if (!G_LOGGER_READY) {
    return;
  }

  std::ostringstream oss;
  oss << direction << ": dst=" << static_cast<unsigned>(frame.dst_addr) << " flag=" << flag_to_string(frame.flag)
      << " marker=" << static_cast<unsigned>(frame.marker) << " blocks=" << static_cast<unsigned>(frame.block_count)
      << " data=" << format_blocks(frame.blocks, frame.block_count);
  common::log::info(oss.str());
}

void log_tick_state() {
  if (!G_LOGGER_READY) {
    return;
  }

  std::ostringstream oss;
  oss << "tick: count=" << G_RUNTIME.count << " clock=" << static_cast<unsigned>(G_RUNTIME.clock);
  common::log::info(oss.str());
}

void clear_msg_com() {
  MSG_COM.fill(0U);
}

void copy_frame_to_msg_com(const mock_bus_frame_t& frame) {
  clear_msg_com();
  const std::size_t n = std::min<std::size_t>(MSG_COM.size(), frame.block_count);
  for (std::size_t i = 0; i < n; ++i) {
    MSG_COM[i] = frame.blocks[i];
  }
}

std::uint8_t normalize_runtime_clock(const std::int32_t raw) {
  if (raw <= 0) {
    return 1U;
  }
  if (raw > 255) {
    return 255U;
  }
  return static_cast<std::uint8_t>(raw);
}

void step_runtime_counters() {
  ++G_RUNTIME.count;
  G_RUNTIME.clock = (G_RUNTIME.clock == 0xFFU) ? 1U : static_cast<std::uint8_t>(G_RUNTIME.clock + 1U);
  log_tick_state();
}

void write_signals_to_msg_par() {
  const auto& ctx = G_IPAR_CTX;
  for (std::size_t i = 0; i < k_param_id_count; ++i) {
    const std::uint16_t id = static_cast<std::uint16_t>(k_param_id_base + static_cast<std::uint16_t>(i));

    // Runtime software clock belongs to PSS software thread, not SIM model.
    if (id == PSSCLOCK) {
      const auto gen_status = ipar::IGEN(ctx, id, static_cast<std::int32_t>(G_RUNTIME.clock));
      if ((gen_status != ipar::status_t::ok) && G_LOGGER_READY) {
        std::ostringstream oss;
        oss << "IGEN failed for runtime clock: id=" << id << " status=" << ipar::to_string(gen_status);
        common::log::warning(oss.str());
      }
      continue;
    }

    isim::value_t value{};
    const auto read_status = isim::ISIMPAR(id, &value);
    if (read_status != isim::status_t::ok) {
      log_isim_status(common::log::level_t::warning, "ISIMPAR failed: ", read_status, id);
      continue;
    }

    const auto gen_status = ipar::IGEN(ctx, id, value_to_i32(value));
    if (gen_status != ipar::status_t::ok) {
      if (G_LOGGER_READY) {
        std::ostringstream oss;
        oss << "IGEN failed: id=" << id << " status=" << ipar::to_string(gen_status);
        common::log::warning(oss.str());
      }
    }
  }
}

std::int32_t parse_command_value(const ipar::context_t& ctx, const std::uint16_t id) {
  const auto parsed = ipar::IPAR(ctx, id);
  if (parsed.status != ipar::status_t::ok) {
    return 0;
  }
  return static_cast<std::int32_t>(std::lround(parsed.value));
}

bool command_active(const ipar::context_t& ctx, const std::uint16_t id) {
  return parse_command_value(ctx, id) != 0;
}

void apply_bool_pair(const ipar::context_t& ctx,
                     const std::uint16_t on_cmd_id,
                     const std::uint16_t off_cmd_id,
                     const std::uint16_t param_id) {
  if (command_active(ctx, on_cmd_id)) {
    const auto status = isim_set_i32(param_id, 1);
    if (status != isim::status_t::ok) {
      log_isim_status(common::log::level_t::warning, "ISIMSET failed: ", status, param_id);
    }
  }
  if (command_active(ctx, off_cmd_id)) {
    const auto status = isim_set_i32(param_id, 0);
    if (status != isim::status_t::ok) {
      log_isim_status(common::log::level_t::warning, "ISIMSET failed: ", status, param_id);
    }
  }
}

void apply_nonzero_value(const ipar::context_t& ctx, const std::uint16_t cmd_id, const std::uint16_t param_id) {
  const auto value = parse_command_value(ctx, cmd_id);
  if (value != 0) {
    const auto status = isim_set_i32(param_id, value);
    if (status != isim::status_t::ok) {
      log_isim_status(common::log::level_t::warning, "ISIMSET failed: ", status, param_id);
    }
  }
}

void apply_msg_com_to_sim() {
  const auto& ctx = G_IPAR_CTX;
  if (command_active(ctx, CPSSINIT)) {
    pss_sim::init();
  }

  apply_bool_pair(ctx, CPSSPWRON, CPSSPWROFF, PSSPWR);
  apply_bool_pair(ctx, CPSSPWR12ON, CPSSPWR12OFF, PSSPWR12);
  apply_bool_pair(ctx, CPSSPWR6ON, CPSSPWR6OFF, PSSPWR6);
  apply_bool_pair(ctx, CPSSPWR5ON, CPSSPWR5OFF, PSSPWR5);
  apply_bool_pair(ctx, CPSSPWR33ON, CPSSPWR33OFF, PSSPWR33);
  apply_bool_pair(ctx, CPSSECOON, CPSSECOOFF, PSSECO);

  apply_nonzero_value(ctx, CPSSMAXT1, PSSMAXT1);
  apply_nonzero_value(ctx, CPSSMAXT2, PSSMAXT2);
  apply_nonzero_value(ctx, CPSSMAXA, PSSMAXA);
  apply_nonzero_value(ctx, CPSSMAXW, PSSMAXW);
  apply_nonzero_value(ctx, CPSSMINC, PSSMINC);
  apply_nonzero_value(ctx, CPSSMINV, PSSMINV);
  apply_nonzero_value(ctx, CPSSMAXA12, PSSMAXA12);
  apply_nonzero_value(ctx, CPSSMAXA6, PSSMAXA6);
  apply_nonzero_value(ctx, CPSSMAXA5, PSSMAXA5);
  apply_nonzero_value(ctx, CPSSMAXA33, PSSMAXA33);

  const auto clock_value = parse_command_value(ctx, CPSSCLOCK);
  if (clock_value != 0) {
    G_RUNTIME.clock = normalize_runtime_clock(clock_value);
  }

  std::int32_t main_pwr = 0;
  if (isim_get_i32(PSSPWR, &main_pwr) != isim::status_t::ok) {
    main_pwr = 0;
  }
  if (main_pwr == 0) {
    (void)isim_set_i32(PSSPWR12, 0);
    (void)isim_set_i32(PSSPWR6, 0);
    (void)isim_set_i32(PSSPWR5, 0);
    (void)isim_set_i32(PSSPWR33, 0);
  }
}

void queue_uv_response(const std::uint8_t marker) {
  G_RUNTIME.tx_frame = mock_bus_frame_t{};
  G_RUNTIME.tx_frame.dst_addr = k_cs_addr;
  G_RUNTIME.tx_frame.flag = ARP_FLAG_UV_RESP;
  G_RUNTIME.tx_frame.marker = marker;
  G_RUNTIME.tx_frame.block_count = 0;
  G_RUNTIME.tx_pending = true;
}

void queue_data_response(const std::uint8_t marker) {
  G_RUNTIME.tx_frame = mock_bus_frame_t{};
  G_RUNTIME.tx_frame.dst_addr = k_cs_addr;
  G_RUNTIME.tx_frame.flag = ARP_FLAG_REQ_DATA_RESP;
  G_RUNTIME.tx_frame.marker = marker;
  G_RUNTIME.tx_frame.block_count = static_cast<std::uint8_t>(MSG_PAR.size());
  for (std::size_t i = 0; i < MSG_PAR.size(); ++i) {
    G_RUNTIME.tx_frame.blocks[i] = MSG_PAR[i];
  }
  G_RUNTIME.tx_pending = true;
}

}  // namespace

void init_msg_buffers() {
  MSG_PAR.fill(0U);
  MSG_COM.fill(0U);
}

void bind_ipar_context() {
  ipar::bind_context(&G_IPAR_CTX);
}

const ipar::context_t& ipar_context() {
  return G_IPAR_CTX;
}

void runtime_init() {
  ensure_logger_initialized();
  init_msg_buffers();
  bind_ipar_context();
  init_isim_registry();
  pss_sim::init();

  G_RUNTIME = runtime_state_t{};
  G_RUNTIME.initialized = true;

  if (G_LOGGER_READY) {
    common::log::info("runtime initialized");
    log_msg_snapshot("runtime init");
  }
}

void runtime_step(const std::uint32_t tick) {
  if (!G_RUNTIME.initialized) {
    runtime_init();
  }

  step_runtime_counters();
  write_signals_to_msg_par();

  if (G_RUNTIME.tx_pending) {
    const auto tx_status = mock_bus_broadcast_frame(k_pss_addr, tick, G_RUNTIME.tx_frame);
    if (tx_status == mock_bus_status_t::ok) {
      log_bus_frame("bus tx", G_RUNTIME.tx_frame);
      G_RUNTIME.tx_pending = false;
    } else if (tx_status != mock_bus_status_t::timeout) {
      log_bus_status(common::log::level_t::warning, "tx failed: ", tx_status);
    }
    return;
  }

  mock_bus_frame_t rx{};
  const auto rx_status = mock_bus_listen_frame(k_pss_addr, k_rx_poll_timeout_ms, &rx);
  if (rx_status != mock_bus_status_t::ok) {
    if (rx_status != mock_bus_status_t::timeout) {
      log_bus_status(common::log::level_t::warning, "rx failed: ", rx_status);
    }
    return;
  }
  log_bus_frame("bus rx", rx);

  if (rx.flag == ARP_FLAG_UV) {
    copy_frame_to_msg_com(rx);
    log_msg_snapshot("after UV rx");
    apply_msg_com_to_sim();
    queue_uv_response(rx.marker);
    return;
  }

  if (rx.flag == ARP_FLAG_REQ_DATA) {
    log_msg_snapshot("before REQ_DATA_RESP tx");
    queue_data_response(rx.marker);
    return;
  }

  if (G_LOGGER_READY) {
    common::log::warning("received frame with unknown flag");
  }
}

}  // namespace pss
