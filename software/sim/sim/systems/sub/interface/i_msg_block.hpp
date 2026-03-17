#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>

namespace imsgblock {

constexpr std::size_t k_block_bits = 16U;
constexpr std::size_t k_block_bytes = 2U;

struct bytes_t {
  std::uint8_t lo;
  std::uint8_t hi;
};

struct discrete_t {
  std::uint16_t b0 : 1;
  std::uint16_t b1 : 1;
  std::uint16_t b2 : 1;
  std::uint16_t b3 : 1;
  std::uint16_t b4 : 1;
  std::uint16_t b5 : 1;
  std::uint16_t b6 : 1;
  std::uint16_t b7 : 1;
  std::uint16_t b8 : 1;
  std::uint16_t b9 : 1;
  std::uint16_t b10 : 1;
  std::uint16_t b11 : 1;
  std::uint16_t b12 : 1;
  std::uint16_t b13 : 1;
  std::uint16_t b14 : 1;
  std::uint16_t b15 : 1;
};

union view_t {
  std::uint16_t raw16{0};
  bytes_t bytes;
  discrete_t discrete;
};

static_assert(std::endian::native == std::endian::little,
              "message block layout requires little-endian target");
static_assert(sizeof(view_t) == sizeof(std::uint16_t),
              "message block view must stay 16-bit wide");

bool layout_supported(std::uint8_t width_bits, int block_offset);

bool read_value(const std::uint16_t* blocks,
                std::size_t block_count,
                std::size_t block_n,
                std::uint8_t block_offset,
                std::uint8_t width_bits,
                std::uint32_t& out_raw);

bool write_value(std::uint16_t* blocks,
                 std::size_t block_count,
                 std::size_t block_n,
                 std::uint8_t block_offset,
                 std::uint8_t width_bits,
                 std::uint32_t raw_value);

bool read_byte(const std::uint16_t* blocks,
               std::size_t block_count,
               std::size_t byte_index,
               std::uint8_t& out_value);

bool write_byte(std::uint16_t* blocks,
                std::size_t block_count,
                std::size_t byte_index,
                std::uint8_t value);

void copy_blocks(std::uint16_t* dst,
                 std::size_t dst_block_count,
                 std::size_t dst_block_offset,
                 const std::uint16_t* src,
                 std::size_t src_block_count,
                 std::size_t src_block_offset,
                 std::size_t blocks_to_copy);

}  // namespace imsgblock
