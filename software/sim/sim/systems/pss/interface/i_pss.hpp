#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "arp_config.hpp"

namespace pss {

// Упаковка заголовка (16 бит):
// [0]       parity
// [1..5]    addr
// [6..7]    flag
// [8..10]   marker
// [11..15]  nblocks
union hdr_u {
  struct bits_t {
    uint16_t parity : 1;
    uint16_t addr : 5;
    uint16_t flag : 2;
    uint16_t marker : 3;
    uint16_t nblocks : 5;
  } bits;

  // Доступ к байтам (raw = hi<<8 | lo).
  // На little-endian: bytes.lo хранит младший байт raw, bytes.hi хранит старший байт raw.
  struct bytes_t {
    uint8_t lo;
    uint8_t hi;
  } bytes;

  uint16_t raw;
};
static_assert(sizeof(hdr_u) == 2, "hdr_u must be 2 bytes");

// Блок данных (16 бит). По умолчанию трактуется как битовая маска функций.
union data_u {
  struct bits_t {
    uint16_t b0 : 1;  uint16_t b1 : 1;
    uint16_t b2 : 1;  uint16_t b3 : 1;
    uint16_t b4 : 1;  uint16_t b5 : 1;
    uint16_t b6 : 1;  uint16_t b7 : 1;
    uint16_t b8 : 1;  uint16_t b9 : 1;
    uint16_t b10 : 1; uint16_t b11 : 1;
    uint16_t b12 : 1; uint16_t b13 : 1;
    uint16_t b14 : 1; uint16_t b15 : 1;
  } bits;

  struct bytes_t {
    uint8_t lo;
    uint8_t hi;
  } bytes;

  uint16_t raw;
};
static_assert(sizeof(data_u) == 2, "data_u must be 2 bytes");

struct frame_t {
  std::array<uint8_t, ARP_BUS_MAX_FRAME_BYTES> buf{};
  std::size_t len{0};
};

struct status_t {
  bool enabled{false};
  uint16_t voltage_mv{0};
  uint16_t current_ma{0};
  uint16_t fault_flags{0};
};

enum class rx_state_t : uint8_t {
  WAIT_SOF = 0,
  READ_HDR,
  READ_DATA,
  SKIP_DATA,
  WAIT_EOF,
  FRAME_READY,
  ERROR,
};

struct rx_parser_t {
  uint8_t self_addr{0};

  rx_state_t state{rx_state_t::WAIT_SOF};

  hdr_u hdr{};
  std::array<data_u, ARP_BUS_MAX_DATA_BLOCKS> data{};

  uint8_t expected_blocks{0};
  uint8_t blocks_read{0};

  // 0 -> ждём hdr.hi, 1 -> ждём hdr.lo
  uint8_t hdr_bytes_read{0};

  // 0 -> ждём data.hi, 1 -> ждём data.lo
  uint8_t data_bytes_read{0};
  data_u cur_data{};

  bool addressed{false};

  uint16_t error_flags{0};
};

static inline uint8_t parity_bit_for_even(uint16_t raw_with_parity0) {
  const uint16_t x = static_cast<uint16_t>(raw_with_parity0 >> 1);
  return static_cast<uint8_t>(__builtin_popcount(static_cast<unsigned>(x)) & 1u);
}

static inline bool parity_ok(uint16_t raw) {
  return ((__builtin_popcount(static_cast<unsigned>(raw)) & 1u) == 0u);
}

static inline hdr_u make_hdr(uint8_t addr, uint8_t flag, uint8_t marker, uint8_t nblocks) {
  hdr_u h{};
  h.raw = 0;
  h.bits.parity = 0;
  h.bits.addr = static_cast<uint16_t>(addr & 0x1Fu);
  h.bits.flag = static_cast<uint16_t>(flag & 0x03u);
  h.bits.marker = static_cast<uint16_t>(marker & 0x07u);
  h.bits.nblocks = static_cast<uint16_t>(nblocks & 0x1Fu);

  hdr_u tmp = h;
  tmp.bits.parity = 0;
  h.bits.parity = parity_bit_for_even(tmp.raw);
  return h;
}

static inline bool append_u8(frame_t& f, uint8_t v) {
  if (f.len >= f.buf.size()) return false;
  f.buf[f.len++] = v;
  return true;
}

static inline bool append_hdr(frame_t& f, const hdr_u& h) {
  return append_u8(f, h.bytes.hi) && append_u8(f, h.bytes.lo);
}

static inline bool append_data(frame_t& f, const data_u& d) {
  return append_u8(f, d.bytes.hi) && append_u8(f, d.bytes.lo);
}

static inline frame_t build_ping_cmd(uint8_t addr) {
  frame_t f{};
  const hdr_u h = make_hdr(addr, ARP_MSG_FLAG_CTRL, 0, 0);
  append_u8(f, ARP_BUS_SOF);
  append_hdr(f, h);
  append_u8(f, ARP_BUS_EOF);
  return f;
}

static inline frame_t build_get_status_req(uint8_t addr) {
  frame_t f{};
  const hdr_u h = make_hdr(addr, ARP_MSG_FLAG_REQ, 0, 0);
  append_u8(f, ARP_BUS_SOF);
  append_hdr(f, h);
  append_u8(f, ARP_BUS_EOF);
  return f;
}

static inline frame_t build_set_functions_cmd(uint8_t addr, uint16_t functions_mask) {
  frame_t f{};
  const hdr_u h = make_hdr(addr, ARP_MSG_FLAG_CTRL, 0, 1);
  data_u d{};
  d.raw = functions_mask;

  append_u8(f, ARP_BUS_SOF);
  append_hdr(f, h);
  append_data(f, d);
  append_u8(f, ARP_BUS_EOF);
  return f;
}

static inline frame_t build_ack(uint8_t addr) {
  frame_t f{};
  const hdr_u h = make_hdr(addr, ARP_MSG_FLAG_CTRL_ACK, 0, 0);
  append_u8(f, ARP_BUS_SOF);
  append_hdr(f, h);
  append_u8(f, ARP_BUS_EOF);
  return f;
}

static inline frame_t build_status_rsp(uint8_t addr, const status_t& st) {
  frame_t f{};
  const hdr_u h = make_hdr(addr, ARP_MSG_FLAG_REQ_RSP, 0, 4);

  data_u b0{};
  b0.raw = 0;
  b0.bits.b0 = static_cast<uint16_t>(st.enabled ? 1 : 0);

  data_u b1{};
  b1.raw = st.fault_flags;

  data_u b2{};
  b2.raw = st.voltage_mv;

  data_u b3{};
  b3.raw = st.current_ma;

  append_u8(f, ARP_BUS_SOF);
  append_hdr(f, h);
  append_data(f, b0);
  append_data(f, b1);
  append_data(f, b2);
  append_data(f, b3);
  append_u8(f, ARP_BUS_EOF);
  return f;
}

static inline void rx_reset(rx_parser_t& p, uint8_t self_addr) {
  p = rx_parser_t{};
  p.self_addr = self_addr;
  p.state = rx_state_t::WAIT_SOF;
}

static inline bool rx_has_frame(const rx_parser_t& p) {
  return p.state == rx_state_t::FRAME_READY;
}

static inline void rx_consume_frame(rx_parser_t& p) {
  p.state = rx_state_t::WAIT_SOF;
  p.hdr_bytes_read = 0;
  p.data_bytes_read = 0;
  p.blocks_read = 0;
  p.expected_blocks = 0;
  p.addressed = false;
}

static inline bool rx_feed(rx_parser_t& p, uint8_t byte) {
  switch (p.state) {
    case rx_state_t::WAIT_SOF: {
      if (byte == ARP_BUS_SOF) {
        p.hdr.raw = 0;
        p.hdr_bytes_read = 0;
        p.blocks_read = 0;
        p.expected_blocks = 0;
        p.data_bytes_read = 0;
        p.cur_data.raw = 0;
        p.addressed = false;
        p.state = rx_state_t::READ_HDR;
      }
      return true;
    }

    case rx_state_t::READ_HDR: {
      if (p.hdr_bytes_read == 0) {
        p.hdr.bytes.hi = byte;
        p.hdr_bytes_read = 1;
        return true;
      }

      p.hdr.bytes.lo = byte;
      p.hdr_bytes_read = 0;

      if (!parity_ok(p.hdr.raw)) {
        p.state = rx_state_t::ERROR;
        p.error_flags |= 0x0001;
        return false;
      }

      p.expected_blocks = static_cast<uint8_t>(p.hdr.bits.nblocks);
      if (p.expected_blocks > ARP_BUS_MAX_DATA_BLOCKS) {
        p.state = rx_state_t::ERROR;
        p.error_flags |= 0x0002;
        return false;
      }

      p.addressed = (static_cast<uint8_t>(p.hdr.bits.addr) == p.self_addr);
      p.blocks_read = 0;
      p.data_bytes_read = 0;
      p.cur_data.raw = 0;

      if (p.expected_blocks == 0) {
        p.state = rx_state_t::WAIT_EOF;
      } else {
        p.state = p.addressed ? rx_state_t::READ_DATA : rx_state_t::SKIP_DATA;
      }
      return true;
    }

    case rx_state_t::READ_DATA:
    case rx_state_t::SKIP_DATA: {
      if (p.data_bytes_read == 0) {
        p.cur_data.bytes.hi = byte;
        p.data_bytes_read = 1;
        return true;
      }

      p.cur_data.bytes.lo = byte;
      p.data_bytes_read = 0;

      if (p.state == rx_state_t::READ_DATA) {
        p.data[p.blocks_read] = p.cur_data;
      }

      p.blocks_read++;
      if (p.blocks_read >= p.expected_blocks) {
        p.state = rx_state_t::WAIT_EOF;
      }
      return true;
    }

    case rx_state_t::WAIT_EOF: {
      if (byte == ARP_BUS_EOF) {
        p.state = rx_state_t::FRAME_READY;
        return true;
      }
      return true;
    }

    case rx_state_t::FRAME_READY:
      return true;

    case rx_state_t::ERROR:
    default:
      return false;
  }
}

static inline bool is_ctrl_cmd(const rx_parser_t& p) {
  return static_cast<uint8_t>(p.hdr.bits.flag) == ARP_MSG_FLAG_CTRL;
}

static inline bool is_ctrl_ack(const rx_parser_t& p) {
  return static_cast<uint8_t>(p.hdr.bits.flag) == ARP_MSG_FLAG_CTRL_ACK;
}

static inline bool is_req(const rx_parser_t& p) {
  return static_cast<uint8_t>(p.hdr.bits.flag) == ARP_MSG_FLAG_REQ;
}

static inline bool is_rsp(const rx_parser_t& p) {
  return static_cast<uint8_t>(p.hdr.bits.flag) == ARP_MSG_FLAG_REQ_RSP;
}

static inline uint16_t get_functions_mask(const rx_parser_t& p) {
  if (p.expected_blocks < 1) return 0;
  return p.data[0].raw;
}

}  // namespace pss