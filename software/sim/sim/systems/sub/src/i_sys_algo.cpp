#include "i_sys_algo.hpp"

#include <algorithm>
#include <cstring>

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

bool read_bus(std::uint8_t* out_value) {
  return mock_bus_read_u8(out_value);
}

void write_bus(const std::uint8_t value) {
  mock_bus_write_u8(value);
}

std::size_t com_frame_blocks(const bus_state_t& state) {
  return (state.msg_com_frame_blocks == 0U) ? state.msg_com_blocks : state.msg_com_frame_blocks;
}

std::size_t par_frame_blocks(const bus_state_t& state) {
  return (state.msg_par_frame_blocks == 0U) ? state.msg_par_blocks : state.msg_par_frame_blocks;
}

std::size_t com_frame_byte_offset(const bus_state_t& state) {
  return state.msg_com_frame_offset_blocks * imsgblock::k_block_bytes;
}

std::size_t par_frame_byte_offset(const bus_state_t& state) {
  return state.msg_par_frame_offset_blocks * imsgblock::k_block_bytes;
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

std::size_t expected_bytes(const bus_state_t& state) {
  return static_cast<std::size_t>(state.msg_n_blocks) * 2U;
}

std::size_t com_capacity_bytes(const bus_state_t& state) {
  return com_frame_blocks(state) * 2U;
}

std::size_t par_capacity_bytes(const bus_state_t& state) {
  return par_frame_blocks(state) * 2U;
}

std::size_t min_size(const std::size_t a, const std::size_t b) {
  return (a < b) ? a : b;
}

std::uint8_t calc_nblocks(const bus_state_t& state) {
  if (state.msg_flag == ARP_FLAG_CMD_REQ) {
    return static_cast<std::uint8_t>(com_frame_blocks(state));
  }
  if (state.msg_flag == ARP_FLAG_DATA_RESP) {
    return static_cast<std::uint8_t>(par_frame_blocks(state));
  }
  return 0;
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
  state.msg_com_frame_offset_blocks = 0;
  state.msg_com_frame_blocks = state.msg_com_blocks;
  state.msg_par_frame_offset_blocks = 0;
  state.msg_par_frame_blocks = state.msg_par_blocks;
}

}  // namespace

void IBUS_BIND_STD(bus_state_t& state,
                   const bus_role_t role,
                   const std::uint8_t self_addr,
                   const std::uint8_t target_addr,
                   std::uint8_t* msg_header_u8,
                   std::uint16_t* msg_header_u16,
                   std::uint8_t* msg_com_buf_u8,
                   std::uint16_t* msg_com_buf_u16,
                   std::uint8_t* msg_par_buf_u8,
                   std::uint16_t* msg_par_buf_u16,
                   std::uint8_t* msg_com_u8,
                   std::uint16_t* msg_com_u16,
                   std::uint8_t* msg_par_u8,
                   std::uint16_t* msg_par_u16,
                   const std::size_t msg_com_blocks,
                   const std::size_t msg_par_blocks) {
  state = bus_state_t{};
  state.role = role;
  state.self_addr = self_addr;
  state.target_addr = target_addr;
  state.msg_header_u8 = msg_header_u8;
  state.msg_header_u16 = msg_header_u16;
  state.msg_com_buf_u8 = msg_com_buf_u8;
  state.msg_com_buf_u16 = msg_com_buf_u16;
  state.msg_par_buf_u8 = msg_par_buf_u8;
  state.msg_par_buf_u16 = msg_par_buf_u16;
  state.msg_com_u8 = msg_com_u8;
  state.msg_com_u16 = msg_com_u16;
  state.msg_par_u8 = msg_par_u8;
  state.msg_par_u16 = msg_par_u16;
  state.msg_com_blocks = msg_com_blocks;
  state.msg_par_blocks = msg_par_blocks;
  state.msg_com_frame_offset_blocks = 0;
  state.msg_com_frame_blocks = msg_com_blocks;
  state.msg_par_frame_offset_blocks = 0;
  state.msg_par_frame_blocks = msg_par_blocks;
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
  state.msg_com_frame_offset_blocks = 0;
  state.msg_com_frame_blocks = state.msg_com_blocks;
  state.msg_par_frame_offset_blocks = 0;
  state.msg_par_frame_blocks = state.msg_par_blocks;
}

bool IBUS_SET_COM_FRAME(bus_state_t& state, const std::size_t offset_blocks, const std::size_t block_count) {
  if ((offset_blocks > state.msg_com_blocks) || (block_count > state.msg_com_blocks) ||
      ((offset_blocks + block_count) > state.msg_com_blocks)) {
    return false;
  }

  state.msg_com_frame_offset_blocks = offset_blocks;
  state.msg_com_frame_blocks = block_count;
  return true;
}

bool IBUS_SET_PAR_FRAME(bus_state_t& state, const std::size_t offset_blocks, const std::size_t block_count) {
  if ((offset_blocks > state.msg_par_blocks) || (block_count > state.msg_par_blocks) ||
      ((offset_blocks + block_count) > state.msg_par_blocks)) {
    return false;
  }

  state.msg_par_frame_offset_blocks = offset_blocks;
  state.msg_par_frame_blocks = block_count;
  return true;
}

void ISYS_SYNC_MSGCOM_TO_BUF(bus_state_t& state) {
  if (!state.msg_com_u16 || !state.msg_com_buf_u16) {
    return;
  }

  imsgblock::copy_blocks(state.msg_com_buf_u16,
                         state.msg_com_blocks,
                         0,
                         state.msg_com_u16,
                         state.msg_com_blocks,
                         0,
                         state.msg_com_blocks);
}

void ISYS_SYNC_MSGPAR_TO_BUF(bus_state_t& state) {
  if (!state.msg_par_u16 || !state.msg_par_buf_u16) {
    return;
  }

  imsgblock::copy_blocks(state.msg_par_buf_u16,
                         state.msg_par_blocks,
                         0,
                         state.msg_par_u16,
                         state.msg_par_blocks,
                         0,
                         state.msg_par_blocks);
}

void ISYS_EXPORT_MODEL_PARAMS(const ipar::context_t& ctx,
                              const std::uint16_t base_id,
                              const std::uint16_t count,
                              model_export_override_fn override_fn,
                              void* user) {
  for (std::uint16_t i = 0; i < count; ++i) {
    const std::uint16_t id = static_cast<std::uint16_t>(base_id + i);

    std::int32_t value = 0;
    if (override_fn && override_fn(user, id, value)) {
      (void)ipar::IGEN(ctx, id, value);
      continue;
    }

    isim::value_t model_value{};
    if (isim::ISIMPAR(id, &model_value) != isim::status_t::ok) {
      continue;
    }

    (void)ipar::IGEN(ctx, id, isim::ISIMI32(model_value));
  }
}

void ISYSBASE_STEP(isys::tick_state_t& tick,
                   const bool power_on,
                   bus_state_t& bus,
                   const bus_hooks_t& bus_hooks,
                   const base_hooks_t& base_hooks,
                   void* user) {
  isys::ISYSTEP(tick);

  if (power_on) {
    call_hook(base_hooks.sim_param_update, user);
    call_hook(base_hooks.cmd_update, user);
    call_hook(base_hooks.local_algorithm, user);
  }

  if (!bus.exchange_busy) {
    call_hook_state(base_hooks.exchange_control, user, bus);
  }

  if (base_hooks.bus_exchange) {
    base_hooks.bus_exchange(bus, bus_hooks, user);
  } else {
    IBUS_EXCHANGE(bus, bus_hooks, user);
  }

  call_hook(base_hooks.cmd_pwr_update, user);
  call_hook(base_hooks.logging, user);
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
    default:
      IBUS_BYPASS(state, hooks, user);
      break;
  }

  call_hook_log(hooks.log_exchange, user, state);
}

void IBUS_RX_EXCHANGE(bus_state_t& state, const bus_hooks_t& hooks, void* user) {
  if ((state.role == bus_role_t::master) && state.exchange_busy) {
    ++state.exchange_wait_clock;
    if (state.exchange_wait_clock > ARP_BUS_RESPONSE_TIMEOUT_FRAMES) {
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
  if (read_bus(&value) && value == ARP_BUS_SOF) {
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
    if (read_bus(&value)) {
      (void)imsgheader::write_byte(state.msg_header_u16, state.msg_header_u8, 0U, value);
      state.exchange_sub_clock = 1;
      state.exchange_wait_clock = 0;
    }
    return;
  }

  if (state.exchange_sub_clock == 1) {
    if (!read_bus(&value)) {
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
  if (read_allowed) {
    if (!read_bus(&value)) {
      return;
    }
    state.exchange_wait_clock = 0;

    const std::size_t index = static_cast<std::size_t>(state.exchange_sub_clock);
    if (state.msg_flag == ARP_FLAG_CMD_REQ) {
      if (index < com_capacity_bytes(state)) {
        (void)write_buffer_byte(state.msg_com_buf_u16,
                                state.msg_com_blocks,
                                state.msg_com_buf_u8,
                                com_frame_byte_offset(state) + index,
                                value);
      }
    } else if (state.msg_flag == ARP_FLAG_DATA_RESP) {
      if (index < par_capacity_bytes(state)) {
        (void)write_buffer_byte(state.msg_par_buf_u16,
                                state.msg_par_blocks,
                                state.msg_par_buf_u8,
                                par_frame_byte_offset(state) + index,
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
  const bool eof_ok = read_bus(&value) && (value == ARP_BUS_EOF);

  if (!eof_ok) {
    call_hook_state(hooks.on_error, user, state);
    reset_to_idle(state);
    return;
  }

  if (state.msg_flag == ARP_FLAG_CMD_REQ) {
    copy_blocks(state.msg_com_u16,
                state.msg_com_blocks,
                state.msg_com_frame_offset_blocks,
                state.msg_com_buf_u16,
                state.msg_com_blocks,
                state.msg_com_frame_offset_blocks,
                com_frame_blocks(state));
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
    copy_blocks(state.msg_par_u16,
                state.msg_par_blocks,
                state.msg_par_frame_offset_blocks,
                state.msg_par_buf_u16,
                state.msg_par_blocks,
                state.msg_par_frame_offset_blocks,
                par_frame_blocks(state));
  }

  reset_to_idle(state);
}

void ITX_SOF(bus_state_t& state, const bus_hooks_t&, void*) {
  write_bus(ARP_BUS_SOF);
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
    write_bus(value);
    state.exchange_sub_clock = 1;
    state.exchange_wait_clock = 0;
    return;
  }

  if (state.exchange_sub_clock == 1) {
    std::uint8_t value = 0;
    (void)imsgheader::read_byte(state.msg_header_u16, state.msg_header_u8, 1U, value);
    write_bus(value);
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
      (void)read_buffer_byte(state.msg_com_buf_u16,
                             state.msg_com_blocks,
                             state.msg_com_buf_u8,
                             com_frame_byte_offset(state) + index,
                             value);
    }
  } else if (state.msg_flag == ARP_FLAG_DATA_RESP) {
    if (index < par_capacity_bytes(state)) {
      (void)read_buffer_byte(state.msg_par_buf_u16,
                             state.msg_par_blocks,
                             state.msg_par_buf_u8,
                             par_frame_byte_offset(state) + index,
                             value);
    }
  }

  write_bus(value);

  ++state.exchange_sub_clock;
  state.exchange_wait_clock = 0;
  if (state.exchange_sub_clock >= expected_bytes(state)) {
    state.tx_flag = tx_flag_t::tx_eof;
  }
}

void ITX_EOF(bus_state_t& state, const bus_hooks_t&, void*) {
  write_bus(ARP_BUS_EOF);
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
