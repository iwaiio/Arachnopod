#include "i_msg_header.hpp"

#include <bit>

namespace imsgheader {
namespace {

view_t load_view(const std::uint16_t* header_u16, const std::uint8_t* header_u8) {
  view_t header{};
  if (header_u16 != nullptr) {
    header.raw16 = header_u16[0];
    return header;
  }

  if (header_u8 != nullptr) {
    header.bytes.lo = header_u8[0];
    header.bytes.hi = header_u8[1];
  }

  return header;
}

}  // namespace

std::uint8_t calc_parity(const std::uint16_t raw_header) {
  view_t header{};
  header.raw16 = raw_header;
  header.bits.parity = 0U;
  return static_cast<std::uint8_t>(std::popcount(static_cast<unsigned>(header.raw16)) & 0x1U);
}

std::uint16_t make(const std::uint8_t addr,
                   const std::uint8_t flag,
                   const std::uint8_t marker,
                   const std::uint8_t nblocks) {
  view_t header{};
  header.bits.addr = addr;
  header.bits.flag = flag;
  header.bits.marker = marker;
  header.bits.nblocks = nblocks;
  header.bits.parity = calc_parity(header.raw16);
  return header.raw16;
}

bool parity_ok(const std::uint16_t raw_header) {
  view_t header{};
  header.raw16 = raw_header;
  return static_cast<std::uint8_t>(header.bits.parity) == calc_parity(raw_header);
}

bool parse(const std::uint16_t raw_header, value_t& out_header) {
  if (!parity_ok(raw_header)) {
    return false;
  }

  view_t header{};
  header.raw16 = raw_header;

  out_header.parity = static_cast<std::uint8_t>(header.bits.parity);
  out_header.addr = static_cast<std::uint8_t>(header.bits.addr);
  out_header.flag = static_cast<std::uint8_t>(header.bits.flag);
  out_header.marker = static_cast<std::uint8_t>(header.bits.marker);
  out_header.nblocks = static_cast<std::uint8_t>(header.bits.nblocks);
  return true;
}

bool read_raw(const std::uint16_t* header_u16,
              const std::uint8_t* header_u8,
              std::uint16_t& out_raw) {
  if ((header_u16 == nullptr) && (header_u8 == nullptr)) {
    return false;
  }

  out_raw = load_view(header_u16, header_u8).raw16;
  return true;
}

void write_raw(std::uint16_t* header_u16,
               std::uint8_t* header_u8,
               const std::uint16_t raw_header) {
  view_t header{};
  header.raw16 = raw_header;

  if (header_u16 != nullptr) {
    header_u16[0] = header.raw16;
  }

  if (header_u8 != nullptr) {
    header_u8[0] = header.bytes.lo;
    header_u8[1] = header.bytes.hi;
  }
}

bool read_byte(const std::uint16_t* header_u16,
               const std::uint8_t* header_u8,
               const std::size_t byte_index,
               std::uint8_t& out_value) {
  if (byte_index >= k_header_bytes) {
    return false;
  }

  const view_t header = load_view(header_u16, header_u8);
  out_value = (byte_index == 0U) ? header.bytes.lo : header.bytes.hi;
  return true;
}

bool write_byte(std::uint16_t* header_u16,
                std::uint8_t* header_u8,
                const std::size_t byte_index,
                const std::uint8_t value) {
  if (byte_index >= k_header_bytes) {
    return false;
  }

  view_t header = load_view(header_u16, header_u8);
  if (byte_index == 0U) {
    header.bytes.lo = value;
  } else {
    header.bytes.hi = value;
  }

  write_raw(header_u16, header_u8, header.raw16);
  return true;
}

bool parse_storage(const std::uint16_t* header_u16,
                   const std::uint8_t* header_u8,
                   value_t& out_header) {
  std::uint16_t raw = 0;
  if (!read_raw(header_u16, header_u8, raw)) {
    return false;
  }

  return parse(raw, out_header);
}

void write_storage(std::uint16_t* header_u16,
                   std::uint8_t* header_u8,
                   const value_t& header) {
  write_raw(header_u16,
            header_u8,
            make(header.addr, header.flag, header.marker, header.nblocks));
}

}  // namespace imsgheader
