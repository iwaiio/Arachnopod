#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>

namespace imsgheader {

constexpr std::size_t k_header_blocks = 1U;
constexpr std::size_t k_header_bytes = 2U;

struct bytes_t {
  std::uint8_t lo;
  std::uint8_t hi;
};

struct bits_t {
  std::uint16_t parity : 1;
  std::uint16_t addr : 5;
  std::uint16_t flag : 2;
  std::uint16_t marker : 3;
  std::uint16_t nblocks : 5;
};

union view_t {
  std::uint16_t raw16{0};
  bytes_t bytes;
  bits_t bits;
};

static_assert(std::endian::native == std::endian::little,
              "message header layout requires little-endian target");
static_assert(sizeof(view_t) == sizeof(std::uint16_t),
              "message header view must stay 16-bit wide");

struct value_t {
  std::uint8_t parity{0};
  std::uint8_t addr{0};
  std::uint8_t flag{0};
  std::uint8_t marker{0};
  std::uint8_t nblocks{0};
};

std::uint8_t calc_parity(std::uint16_t raw_header);
std::uint16_t make(std::uint8_t addr,
                   std::uint8_t flag,
                   std::uint8_t marker,
                   std::uint8_t nblocks);
bool parity_ok(std::uint16_t raw_header);
bool parse(std::uint16_t raw_header, value_t& out_header);

bool read_raw(const std::uint16_t* header_u16,
              const std::uint8_t* header_u8,
              std::uint16_t& out_raw);
void write_raw(std::uint16_t* header_u16,
               std::uint8_t* header_u8,
               std::uint16_t raw_header);

bool read_byte(const std::uint16_t* header_u16,
               const std::uint8_t* header_u8,
               std::size_t byte_index,
               std::uint8_t& out_value);
bool write_byte(std::uint16_t* header_u16,
                std::uint8_t* header_u8,
                std::size_t byte_index,
                std::uint8_t value);

bool parse_storage(const std::uint16_t* header_u16,
                   const std::uint8_t* header_u8,
                   value_t& out_header);
void write_storage(std::uint16_t* header_u16,
                   std::uint8_t* header_u8,
                   const value_t& header);

}  // namespace imsgheader
