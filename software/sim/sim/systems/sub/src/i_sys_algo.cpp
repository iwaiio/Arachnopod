#include "i_sys_algo.hpp"

#include <algorithm>
#include <cstring>

#include "command_tab.hpp"
#include "i_msg_block.hpp"
#include "i_msg_header.hpp"
#include "i_mock_bus.hpp"
#include "i_sim.hpp"

namespace isysalgo {
namespace {

void call_hook(void (*fn)(void*), void* user) {
  if (fn) {
    fn(user);
  }
}

void call_hook_state(void (*fn)(void*, bus_state_t&), void* user, bus_state_t& state) {
  if (fn) {
    fn(user, state);
  }
}

void call_hook_log(void (*fn)(void*, const bus_state_t&), void* user, const bus_state_t& state) {
  if (fn) {
    fn(user, state);
  }
}

bool read_bus(const bus_state_t& state, std::uint8_t* out_value) {
  return mock_bus_read_u8(state.writer_tick, out_value);
}

void write_bus(const bus_state_t& state, const std::uint8_t value) {
  mock_bus_write_u8(state.writer_tick, value);
}

std::uint8_t type_width_bits(const std::uint8_t type) {
  switch (type) {
    case TYPE_D:
      return 1U;
    case TYPE_A:
      return 8U;
    case TYPE_AP:
      return 16U;
    default:
      return 0U;
  }
}

std::size_t clamp_frame_blocks(const std::size_t requested_blocks, const std::size_t buffer_blocks) {
  if (buffer_blocks == 0U) {
    return requested_blocks;
  }
  return std::min(requested_blocks, buffer_blocks);
}

std::size_t cmd_frame_blocks(const bus_state_t& state) {
  const auto requested = (state.msg_cmd_frame_blocks == 0U) ? state.msg_cmd_blocks : state.msg_cmd_frame_blocks;
  return clamp_frame_blocks(requested, state.msg_cmd_buf_blocks);
}

std::size_t prm_frame_blocks(const bus_state_t& state) {
  const auto requested = (state.msg_prm_frame_blocks == 0U) ? state.msg_prm_blocks : state.msg_prm_frame_blocks;
  return clamp_frame_blocks(requested, state.msg_prm_buf_blocks);
}

std::size_t cmd_frame_byte_offset(const bus_state_t& state) {
  return state.msg_cmd_frame_offset_blocks * imsgblock::k_block_bytes;
}

std::size_t prm_frame_byte_offset(const bus_state_t& state) {
  return state.msg_prm_frame_offset_blocks * imsgblock::k_block_bytes;
}

bool read_buffer_byte(const std::uint16_t* blocks_u16,
                      const std::size_t block_count,
                      const std::uint8_t* buf_u8,
                      const std::size_t byte_index,
                      std::uint8_t& out_value) {
  if (blocks_u16) {
    return imsgblock::read_byte(blocks_u16, block_count, byte_index, out_value);
  }

  if (!buf_u8) {
    return false;
  }

  out_value = buf_u8[byte_index];
  return true;
}

bool write_buffer_byte(std::uint16_t* blocks_u16,
                       const std::size_t block_count,
                       std::uint8_t* buf_u8,
                       const std::size_t byte_index,
                       const std::uint8_t value) {
  if (blocks_u16) {
    return imsgblock::write_byte(blocks_u16, block_count, byte_index, value);
  }

  if (!buf_u8) {
    return false;
  }

  buf_u8[byte_index] = value;
  return true;
}

void copy_blocks(std::uint16_t* dst_u16,
                 const std::size_t dst_blocks,
                 const std::size_t dst_offset_blocks,
                 const std::uint16_t* src_u16,
                 const std::size_t src_blocks,
                 const std::size_t src_offset_blocks,
                 const std::size_t blocks_to_copy) {
  if (dst_u16 && src_u16) {
    imsgblock::copy_blocks(dst_u16,
                           dst_blocks,
                           dst_offset_blocks,
                           src_u16,
                           src_blocks,
                           src_offset_blocks,
                           blocks_to_copy);
    return;
  }

  if ((dst_u16 == nullptr) || (src_u16 == nullptr) || (blocks_to_copy == 0U)) {
    return;
  }

  for (std::size_t i = 0; i < blocks_to_copy; ++i) {
    dst_u16[dst_offset_blocks + i] = src_u16[src_offset_blocks + i];
  }
}

void merge_nonzero_cmd_fields(bus_state_t& state) {
  if ((state.system_id == SYS_NONE) || (state.msg_cmd_u16 == nullptr) || (state.msg_cmd_buf_u16 == nullptr)) {
    return;
  }

  for (std::size_t i = 0; i < Comm_max; ++i) {
    const auto& item = S_basecommtab[i];
    if (static_cast<std::uint8_t>(item.sys_id) != state.system_id) {
      continue;
    }

    const auto width_bits = type_width_bits(static_cast<std::uint8_t>(item.type));
    if (width_bits == 0U) {
      continue;
    }

    std::uint32_t raw_value = 0U;
    if (!imsgblock::read_value(state.msg_cmd_buf_u16,
                               state.msg_cmd_buf_blocks,
                               static_cast<std::size_t>(static_cast<unsigned char>(item.msg_block_n)),
                               static_cast<std::uint8_t>(item.msg_block_offset),
                               width_bits,
                               raw_value)) {
      continue;
    }

    if (raw_value == 0U) {
      continue;
    }

    (void)imsgblock::write_value(state.msg_cmd_u16,
                                 state.msg_cmd_blocks,
                                 static_cast<std::size_t>(static_cast<unsigned char>(item.msg_block_n)),
                                 static_cast<std::uint8_t>(item.msg_block_offset),
                                 width_bits,
                                 raw_value);
  }
}

std::size_t expected_bytes(const bus_state_t& state) {
  return static_cast<std::size_t>(state.msg_n_blocks) * 2U;
}

std::size_t com_capacity_bytes(const bus_state_t& state) {
  return cmd_frame_blocks(state) * 2U;
}

std::size_t par_capacity_bytes(const bus_state_t& state) {
  return prm_frame_blocks(state) * 2U;
}

std::size_t min_size(const std::size_t a, const std::size_t b) {
  return (a < b) ? a : b;
}

std::uint8_t calc_nblocks(const bus_state_t& state) {
  if (state.msg_flag == ARP_FLAG_CMD_REQ) {
    return static_cast<std::uint8_t>(cmd_frame_blocks(state));
  }
  if (state.msg_flag == ARP_FLAG_DATA_RESP) {
    return static_cast<std::uint8_t>(prm_frame_blocks(state));
  }
  return 0;
}

void mark_result(bus_state_t& state, const exchange_result_t result) {
  state.last_result = result;
  state.result_ready = true;
}

void reset_to_idle(bus_state_t& state) {
  state.exchange_flag = exchange_flag_t::rx;
  state.rx_flag = rx_flag_t::rx_sof;
  state.tx_flag = tx_flag_t::tx_sof;
  state.msg_adr_flag = false;
  state.msg_n_blocks = 0;
  state.msg_flag = ARP_FLAG_CMD_REQ;
  state.exchange_sub_clock = 0;
  state.exchange_wait_clock = 0;
  state.exchange_busy = false;
  state.msg_cmd_frame_offset_blocks = 0;
  state.msg_cmd_frame_blocks = clamp_frame_blocks(state.msg_cmd_blocks, state.msg_cmd_buf_blocks);
  state.msg_prm_frame_offset_blocks = 0;
  state.msg_prm_frame_blocks = clamp_frame_blocks(state.msg_prm_blocks, state.msg_prm_buf_blocks);
}

}  // namespace

void IBUS_BIND_STD(bus_state_t& state,
                   const bus_role_t role,
                   const std::uint8_t system_id,
                   const std::uint8_t self_addr,
                   const std::uint8_t target_addr,
                   std::uint8_t* msg_header_u8,
                   std::uint16_t* msg_header_u16,
                   std::uint8_t* msg_cmd_buf_u8,
                   std::uint16_t* msg_cmd_buf_u16,
                   const std::size_t msg_cmd_buf_blocks,
                   std::uint8_t* msg_prm_buf_u8,
                   std::uint16_t* msg_prm_buf_u16,
                   const std::size_t msg_prm_buf_blocks,
                   std::uint8_t* msg_cmd_u8,
                   std::uint16_t* msg_cmd_u16,
                   std::uint8_t* msg_prm_u8,
                   std::uint16_t* msg_prm_u16,
                   const std::size_t msg_cmd_blocks,
                   const std::size_t msg_prm_blocks) {
  state = bus_state_t{};
  state.role = role;
  state.system_id = system_id;
  state.self_addr = self_addr;
  state.target_addr = target_addr;
  state.msg_header_u8 = msg_header_u8;
  state.msg_header_u16 = msg_header_u16;
  state.msg_cmd_buf_u8 = msg_cmd_buf_u8;
  state.msg_cmd_buf_u16 = msg_cmd_buf_u16;
  state.msg_cmd_buf_blocks = msg_cmd_buf_blocks;
  state.msg_prm_buf_u8 = msg_prm_buf_u8;
  state.msg_prm_buf_u16 = msg_prm_buf_u16;
  state.msg_prm_buf_blocks = msg_prm_buf_blocks;
  state.msg_cmd_u8 = msg_cmd_u8;
  state.msg_cmd_u16 = msg_cmd_u16;
  state.msg_prm_u8 = msg_prm_u8;
  state.msg_prm_u16 = msg_prm_u16;
  state.msg_cmd_blocks = msg_cmd_blocks;
  state.msg_prm_blocks = msg_prm_blocks;
  state.msg_cmd_frame_offset_blocks = 0;
  state.msg_cmd_frame_blocks = clamp_frame_blocks(msg_cmd_blocks, msg_cmd_buf_blocks);
  state.msg_prm_frame_offset_blocks = 0;
  state.msg_prm_frame_blocks = clamp_frame_blocks(msg_prm_blocks, msg_prm_buf_blocks);
}

void IBUS_PARSE_STD_HEADER(bus_state_t& state) {
  imsgheader::value_t header{};
  if (!imsgheader::parse_storage(state.msg_header_u16, state.msg_header_u8, header)) {
    state.msg_adr_flag = false;
    state.msg_n_blocks = 0;
    return;
  }

  state.msg_adr_flag = (header.addr == state.self_addr);
  state.msg_flag = header.flag;
  state.msg_n_blocks = header.nblocks;
}

void IBUS_GEN_STD_HEADER(bus_state_t& state) {
  if (!state.msg_header_u16 && !state.msg_header_u8) {
    return;
  }

  state.msg_n_blocks = calc_nblocks(state);
  imsgheader::value_t header{};
  header.addr = state.target_addr;
  header.flag = state.msg_flag;
  header.marker = static_cast<std::uint8_t>(state.writer_tick);
  header.nblocks = state.msg_n_blocks;
  imsgheader::write_storage(state.msg_header_u16, state.msg_header_u8, header);
}

void IBUS_RESET_FRAME_LAYOUT(bus_state_t& state) {
  state.msg_cmd_frame_offset_blocks = 0;
  state.msg_cmd_frame_blocks = clamp_frame_blocks(state.msg_cmd_blocks, state.msg_cmd_buf_blocks);
  state.msg_prm_frame_offset_blocks = 0;
  state.msg_prm_frame_blocks = clamp_frame_blocks(state.msg_prm_blocks, state.msg_prm_buf_blocks);
}

bool IBUS_SET_CMD_FRAME(bus_state_t& state, const std::size_t offset_blocks, const std::size_t block_count) {
  if ((offset_blocks > state.msg_cmd_blocks) || (block_count > state.msg_cmd_blocks) ||
      ((offset_blocks + block_count) > state.msg_cmd_blocks) || (block_count > state.msg_cmd_buf_blocks)) {
    return false;
  }

  state.msg_cmd_frame_offset_blocks = offset_blocks;
  state.msg_cmd_frame_blocks = block_count;
  return true;
}

bool IBUS_SET_PRM_FRAME(bus_state_t& state, const std::size_t offset_blocks, const std::size_t block_count) {
  if ((offset_blocks > state.msg_prm_blocks) || (block_count > state.msg_prm_blocks) ||
      ((offset_blocks + block_count) > state.msg_prm_blocks) || (block_count > state.msg_prm_buf_blocks)) {
    return false;
  }

  state.msg_prm_frame_offset_blocks = offset_blocks;
  state.msg_prm_frame_blocks = block_count;
  return true;
}

void IBUS_CLEAR_RESULT(bus_state_t& state) {
  state.result_ready = false;
  state.last_result = exchange_result_t::none;
}

bool IBUS_TAKE_RESULT(bus_state_t& state, exchange_result_t& out_result) {
  if (!state.result_ready) {
    return false;
  }

  out_result = state.last_result;
  state.result_ready = false;
  state.last_result = exchange_result_t::none;
  return true;
}

const char* IBUS_RESULT_TO_STRING(const exchange_result_t result) {
  switch (result) {
    case exchange_result_t::success:
      return "success";
    case exchange_result_t::timeout:
      return "timeout";
    case exchange_result_t::invalid_eof:
      return "invalid_eof";
    case exchange_result_t::none:
    default:
      return "none";
  }
}

void ISYS_SYNC_MSGCMD_TO_BUF(bus_state_t& state) {
  if (!state.msg_cmd_u16 || !state.msg_cmd_buf_u16) {
    return;
  }

  const auto blocks_to_copy = min_size(state.msg_cmd_buf_blocks, state.msg_cmd_blocks);
  imsgblock::copy_blocks(state.msg_cmd_buf_u16,
                         state.msg_cmd_buf_blocks,
                         0,
                         state.msg_cmd_u16,
                         state.msg_cmd_blocks,
                         0,
                         blocks_to_copy);
}

void ISYS_SYNC_MSGPRM_TO_BUF(bus_state_t& state) {
  if (!state.msg_prm_u16 || !state.msg_prm_buf_u16) {
    return;
  }

  const auto blocks_to_copy = min_size(state.msg_prm_buf_blocks, state.msg_prm_blocks);
  imsgblock::copy_blocks(state.msg_prm_buf_u16,
                         state.msg_prm_buf_blocks,
                         0,
                         state.msg_prm_u16,
                         state.msg_prm_blocks,
                         0,
                         blocks_to_copy);
}

void ISYS_EXPORT_MODEL_PARAMS(const ipar::context_t& ctx,
                              const std::uint16_t base_id,
                              const std::uint16_t count,
                              model_export_override_fn override_fn,
                              void* user) {
  for (std::uint16_t i = 0; i < count; ++i) {
    const std::uint16_t id = static_cast<std::uint16_t>(base_id + i);

    float value = 0.0F;
    if (override_fn && override_fn(user, id, value)) {
      (void)ipar::IMSGSET(ctx, id, value);
      continue;
    }

    isim::value_t model_value{};
    if (isim::ISIMGET(id, &model_value) != isim::status_t::ok) {
      continue;
    }

    (void)ipar::IMSGSET(ctx, id, isim::ISIMF32(model_value));
  }
}

void ISYSALG(const sysalg_hooks_t& hooks, void* user) {
  call_hook(hooks.ICMDUPD, user);
  call_hook(hooks.LOCALSYSALG, user);
  call_hook(hooks.ISYSNODES, user);
  call_hook(hooks.IPRMUPD, user);
}

void ISYSBASEALG(isys::tick_state_t& tick,
                 const bool power_on,
                 bus_state_t& bus,
                 const bus_hooks_t& bus_hooks,
                 const base_hooks_t& base_hooks,
                 void* user) {
  isys::ISYSTEP(tick);

  if (power_on) {
    ISYSALG(base_hooks.ISYSALG, user);
  }

  if (!bus.exchange_busy) {
    call_hook_state(base_hooks.IEXCHANGECTRL, user, bus);
  }

  if (base_hooks.IBUSEXCHANGE) {
    base_hooks.IBUSEXCHANGE(bus, bus_hooks, user);
  } else {
    IBUS_EXCHANGE(bus, bus_hooks, user);
  }

  call_hook(base_hooks.ICMDPWRUPD, user);
  call_hook(base_hooks.ILOG, user);
}

void IBUS_EXCHANGE(bus_state_t& state, const bus_hooks_t& hooks, void* user) {
  switch (state.exchange_flag) {
    case exchange_flag_t::rx:
      IBUS_RX_EXCHANGE(state, hooks, user);
      break;
    case exchange_flag_t::tx:
      IBUS_TX_EXCHANGE(state, hooks, user);
      break;
    case exchange_flag_t::bypass:
      IBUS_BYPASS(state, hooks, user);
      break;
    default:
      break;
  }

  call_hook_log(hooks.log_exchange, user, state);
}

void IBUS_RX_EXCHANGE(bus_state_t& state, const bus_hooks_t& hooks, void* user) {
  if ((state.role == bus_role_t::master) && state.exchange_busy) {
    ++state.exchange_wait_clock;
    if (state.exchange_wait_clock > ARP_BUS_RESPONSE_TIMEOUT_FRAMES) {
      mark_result(state, exchange_result_t::timeout);
      call_hook_state(hooks.on_error, user, state);
      reset_to_idle(state);
      return;
    }
  }

  switch (state.rx_flag) {
    case rx_flag_t::rx_sof:
      IRX_SOF(state, hooks, user);
      break;
    case rx_flag_t::rx_header:
      IRX_HEADER(state, hooks, user);
      break;
    case rx_flag_t::rx_data:
      IRX_DATA(state, hooks, user);
      break;
    case rx_flag_t::rx_eof:
    default:
      IRX_EOF(state, hooks, user);
      break;
  }
}

void IBUS_TX_EXCHANGE(bus_state_t& state, const bus_hooks_t& hooks, void* user) {
  switch (state.tx_flag) {
    case tx_flag_t::tx_sof:
      ITX_SOF(state, hooks, user);
      break;
    case tx_flag_t::tx_header:
      ITX_HEADER(state, hooks, user);
      break;
    case tx_flag_t::tx_data:
      ITX_DATA(state, hooks, user);
      break;
    case tx_flag_t::tx_eof:
    default:
      ITX_EOF(state, hooks, user);
      break;
  }
}

void IBUS_BYPASS(bus_state_t& state, const bus_hooks_t& hooks, void* user) {
  call_hook_state(hooks.bypass, user, state);
}

void IRX_SOF(bus_state_t& state, const bus_hooks_t&, void*) {
  std::uint8_t value = 0;
  if (read_bus(state, &value) && value == ARP_BUS_SOF) {
    state.rx_flag = rx_flag_t::rx_header;
    state.exchange_sub_clock = 0;
    state.exchange_wait_clock = 0;
    if (state.role == bus_role_t::device) {
      state.exchange_busy = true;
    }
  }
}

void IRX_HEADER(bus_state_t& state, const bus_hooks_t& hooks, void* user) {
  std::uint8_t value = 0;
  if (state.exchange_sub_clock == 0) {
    if (read_bus(state, &value)) {
      (void)imsgheader::write_byte(state.msg_header_u16, state.msg_header_u8, 0U, value);
      state.exchange_sub_clock = 1;
      state.exchange_wait_clock = 0;
    }
    return;
  }

  if (state.exchange_sub_clock == 1) {
    if (!read_bus(state, &value)) {
      return;
    }

    (void)imsgheader::write_byte(state.msg_header_u16, state.msg_header_u8, 1U, value);
    state.exchange_wait_clock = 0;

    if (hooks.parse_header) {
      hooks.parse_header(user, state);
    } else {
      IBUS_PARSE_STD_HEADER(state);
    }

    state.rx_flag = (state.msg_n_blocks > 0) ? rx_flag_t::rx_data : rx_flag_t::rx_eof;
    state.exchange_sub_clock = 0;
  }
}

void IRX_DATA(bus_state_t& state, const bus_hooks_t&, void*) {
  const bool read_allowed =
      state.msg_adr_flag && (state.msg_flag == ARP_FLAG_CMD_REQ || state.msg_flag == ARP_FLAG_DATA_RESP);

  std::uint8_t value = 0;
  if (!read_bus(state, &value)) {
    return;
  }
  state.exchange_wait_clock = 0;

  if (read_allowed) {
    const std::size_t index = static_cast<std::size_t>(state.exchange_sub_clock);
    if (state.msg_flag == ARP_FLAG_CMD_REQ) {
      if (index < com_capacity_bytes(state)) {
        (void)write_buffer_byte(state.msg_cmd_buf_u16,
                                state.msg_cmd_buf_blocks,
                                state.msg_cmd_buf_u8,
                                index,
                                value);
      }
    } else if (state.msg_flag == ARP_FLAG_DATA_RESP) {
      if (index < par_capacity_bytes(state)) {
        (void)write_buffer_byte(state.msg_prm_buf_u16,
                                state.msg_prm_buf_blocks,
                                state.msg_prm_buf_u8,
                                index,
                                value);
      }
    }
  }

  ++state.exchange_sub_clock;
  if (state.exchange_sub_clock >= expected_bytes(state)) {
    state.rx_flag = rx_flag_t::rx_eof;
  }
}

void IRX_EOF(bus_state_t& state, const bus_hooks_t& hooks, void* user) {
  std::uint8_t value = 0;
  if (!read_bus(state, &value)) {
    return;
  }

  const bool eof_ok = (value == ARP_BUS_EOF);

  if (!eof_ok) {
    mark_result(state, exchange_result_t::invalid_eof);
    call_hook_state(hooks.on_error, user, state);
    reset_to_idle(state);
    return;
  }

  if (!state.msg_adr_flag) {
    reset_to_idle(state);
    return;
  }

  if (state.msg_flag == ARP_FLAG_CMD_REQ) {
    merge_nonzero_cmd_fields(state);
    state.msg_flag = ARP_FLAG_CMD_RESP;
    state.tx_flag = tx_flag_t::tx_sof;
    state.exchange_flag = exchange_flag_t::tx;
    state.exchange_sub_clock = 0;
    return;
  }

  if (state.msg_flag == ARP_FLAG_DATA_REQ) {
    state.msg_flag = ARP_FLAG_DATA_RESP;
    state.tx_flag = tx_flag_t::tx_sof;
    state.exchange_flag = exchange_flag_t::tx;
    state.exchange_sub_clock = 0;
    return;
  }

  if (state.msg_flag == ARP_FLAG_DATA_RESP) {
    copy_blocks(state.msg_prm_u16,
                state.msg_prm_blocks,
                state.msg_prm_frame_offset_blocks,
                state.msg_prm_buf_u16,
                state.msg_prm_buf_blocks,
                0,
                prm_frame_blocks(state));
  }

  if (state.role == bus_role_t::master) {
    mark_result(state, exchange_result_t::success);
  }

  reset_to_idle(state);
}

void ITX_SOF(bus_state_t& state, const bus_hooks_t&, void*) {
  write_bus(state, ARP_BUS_SOF);
  state.tx_flag = tx_flag_t::tx_header;
  state.exchange_sub_clock = 0;
  state.exchange_wait_clock = 0;
  if (state.role == bus_role_t::master) {
    state.exchange_busy = true;
  }
}

void ITX_HEADER(bus_state_t& state, const bus_hooks_t& hooks, void* user) {
  if (state.exchange_sub_clock == 0) {
    if (hooks.gen_header) {
      hooks.gen_header(user, state);
    } else {
      IBUS_GEN_STD_HEADER(state);
    }

    std::uint8_t value = 0;
    (void)imsgheader::read_byte(state.msg_header_u16, state.msg_header_u8, 0U, value);
    write_bus(state, value);
    state.exchange_sub_clock = 1;
    state.exchange_wait_clock = 0;
    return;
  }

  if (state.exchange_sub_clock == 1) {
    std::uint8_t value = 0;
    (void)imsgheader::read_byte(state.msg_header_u16, state.msg_header_u8, 1U, value);
    write_bus(state, value);
    state.tx_flag = (state.msg_n_blocks > 0) ? tx_flag_t::tx_data : tx_flag_t::tx_eof;
    state.exchange_sub_clock = 0;
    state.exchange_wait_clock = 0;
  }
}

void ITX_DATA(bus_state_t& state, const bus_hooks_t&, void*) {
  const std::size_t index = static_cast<std::size_t>(state.exchange_sub_clock);
  std::uint8_t value = 0;
  if (state.msg_flag == ARP_FLAG_CMD_REQ) {
    if (index < com_capacity_bytes(state)) {
      (void)read_buffer_byte(state.msg_cmd_buf_u16,
                             state.msg_cmd_buf_blocks,
                             state.msg_cmd_buf_u8,
                             index,
                             value);
    }
  } else if (state.msg_flag == ARP_FLAG_DATA_RESP) {
    if (index < par_capacity_bytes(state)) {
      (void)read_buffer_byte(state.msg_prm_buf_u16,
                             state.msg_prm_buf_blocks,
                             state.msg_prm_buf_u8,
                             index,
                             value);
    }
  }

  write_bus(state, value);

  ++state.exchange_sub_clock;
  state.exchange_wait_clock = 0;
  if (state.exchange_sub_clock >= expected_bytes(state)) {
    state.tx_flag = tx_flag_t::tx_eof;
  }
}

void ITX_EOF(bus_state_t& state, const bus_hooks_t&, void*) {
  write_bus(state, ARP_BUS_EOF);
  state.rx_flag = rx_flag_t::rx_sof;
  state.exchange_flag = exchange_flag_t::rx;
  state.exchange_sub_clock = 0;
  state.exchange_wait_clock = 0;

  if (state.role == bus_role_t::device) {
    state.exchange_busy = false;
    state.msg_flag = ARP_FLAG_CMD_REQ;
    state.tx_flag = tx_flag_t::tx_sof;
    return;
  }

  if (state.msg_flag == ARP_FLAG_CMD_REQ) {
    state.msg_flag = ARP_FLAG_CMD_RESP;
  } else if (state.msg_flag == ARP_FLAG_DATA_REQ) {
    state.msg_flag = ARP_FLAG_DATA_RESP;
  } else {
    state.exchange_busy = false;
    state.msg_flag = ARP_FLAG_CMD_REQ;
  }

  state.tx_flag = tx_flag_t::tx_sof;
}

}  // namespace isysalgo
