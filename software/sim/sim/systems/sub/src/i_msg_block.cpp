#include "i_msg_block.hpp"

#include <algorithm>

namespace imsgblock {
namespace {

bool read_discrete(const view_t& block, const std::uint8_t block_offset, std::uint32_t& out_raw) {
  switch (block_offset) {
    case 0:
      out_raw = block.discrete.b0;
      return true;
    case 1:
      out_raw = block.discrete.b1;
      return true;
    case 2:
      out_raw = block.discrete.b2;
      return true;
    case 3:
      out_raw = block.discrete.b3;
      return true;
    case 4:
      out_raw = block.discrete.b4;
      return true;
    case 5:
      out_raw = block.discrete.b5;
      return true;
    case 6:
      out_raw = block.discrete.b6;
      return true;
    case 7:
      out_raw = block.discrete.b7;
      return true;
    case 8:
      out_raw = block.discrete.b8;
      return true;
    case 9:
      out_raw = block.discrete.b9;
      return true;
    case 10:
      out_raw = block.discrete.b10;
      return true;
    case 11:
      out_raw = block.discrete.b11;
      return true;
    case 12:
      out_raw = block.discrete.b12;
      return true;
    case 13:
      out_raw = block.discrete.b13;
      return true;
    case 14:
      out_raw = block.discrete.b14;
      return true;
    case 15:
      out_raw = block.discrete.b15;
      return true;
    default:
      return false;
  }
}

bool write_discrete(view_t& block, const std::uint8_t block_offset, const bool value) {
  const std::uint16_t discrete = value ? 1U : 0U;
  switch (block_offset) {
    case 0:
      block.discrete.b0 = discrete;
      return true;
    case 1:
      block.discrete.b1 = discrete;
      return true;
    case 2:
      block.discrete.b2 = discrete;
      return true;
    case 3:
      block.discrete.b3 = discrete;
      return true;
    case 4:
      block.discrete.b4 = discrete;
      return true;
    case 5:
      block.discrete.b5 = discrete;
      return true;
    case 6:
      block.discrete.b6 = discrete;
      return true;
    case 7:
      block.discrete.b7 = discrete;
      return true;
    case 8:
      block.discrete.b8 = discrete;
      return true;
    case 9:
      block.discrete.b9 = discrete;
      return true;
    case 10:
      block.discrete.b10 = discrete;
      return true;
    case 11:
      block.discrete.b11 = discrete;
      return true;
    case 12:
      block.discrete.b12 = discrete;
      return true;
    case 13:
      block.discrete.b13 = discrete;
      return true;
    case 14:
      block.discrete.b14 = discrete;
      return true;
    case 15:
      block.discrete.b15 = discrete;
      return true;
    default:
      return false;
  }
}

}  // namespace

bool layout_supported(const std::uint8_t width_bits, const int block_offset) {
  if (block_offset < 0 || block_offset >= static_cast<int>(k_block_bits)) {
    return false;
  }

  switch (width_bits) {
    case 1U:
      return true;
    case 8U:
      return (block_offset == 0) || (block_offset == 8);
    case 16U:
      return block_offset == 0;
    default:
      return false;
  }
}

bool read_value(const std::uint16_t* blocks,
                const std::size_t block_count,
                const std::size_t block_n,
                const std::uint8_t block_offset,
                const std::uint8_t width_bits,
                std::uint32_t& out_raw) {
  if ((blocks == nullptr) || (width_bits == 0U) || (block_n >= block_count)) {
    return false;
  }

  view_t block{};
  block.raw16 = blocks[block_n];

  switch (width_bits) {
    case 1U:
      return read_discrete(block, block_offset, out_raw);
    case 8U:
      if (block_offset == 0U) {
        out_raw = block.bytes.lo;
        return true;
      }
      if (block_offset == 8U) {
        out_raw = block.bytes.hi;
        return true;
      }
      return false;
    case 16U:
      if (block_offset != 0U) {
        return false;
      }
      out_raw = block.raw16;
      return true;
    default:
      return false;
  }
}

bool write_value(std::uint16_t* blocks,
                 const std::size_t block_count,
                 const std::size_t block_n,
                 const std::uint8_t block_offset,
                 const std::uint8_t width_bits,
                 const std::uint32_t raw_value) {
  if ((blocks == nullptr) || (width_bits == 0U) || (block_n >= block_count)) {
    return false;
  }

  view_t block{};
  block.raw16 = blocks[block_n];

  switch (width_bits) {
    case 1U:
      if (!write_discrete(block, block_offset, raw_value != 0U)) {
        return false;
      }
      break;
    case 8U:
      if (block_offset == 0U) {
        block.bytes.lo = static_cast<std::uint8_t>(raw_value);
      } else if (block_offset == 8U) {
        block.bytes.hi = static_cast<std::uint8_t>(raw_value);
      } else {
        return false;
      }
      break;
    case 16U:
      if (block_offset != 0U) {
        return false;
      }
      block.raw16 = static_cast<std::uint16_t>(raw_value);
      break;
    default:
      return false;
  }

  blocks[block_n] = block.raw16;
  return true;
}

bool read_byte(const std::uint16_t* blocks,
               const std::size_t block_count,
               const std::size_t byte_index,
               std::uint8_t& out_value) {
  const std::size_t block_n = byte_index / k_block_bytes;
  if ((blocks == nullptr) || (block_n >= block_count)) {
    return false;
  }

  view_t block{};
  block.raw16 = blocks[block_n];
  out_value = ((byte_index % k_block_bytes) == 0U) ? block.bytes.lo : block.bytes.hi;
  return true;
}

bool write_byte(std::uint16_t* blocks,
                const std::size_t block_count,
                const std::size_t byte_index,
                const std::uint8_t value) {
  const std::size_t block_n = byte_index / k_block_bytes;
  if ((blocks == nullptr) || (block_n >= block_count)) {
    return false;
  }

  view_t block{};
  block.raw16 = blocks[block_n];
  if ((byte_index % k_block_bytes) == 0U) {
    block.bytes.lo = value;
  } else {
    block.bytes.hi = value;
  }
  blocks[block_n] = block.raw16;
  return true;
}

void copy_blocks(std::uint16_t* dst,
                 const std::size_t dst_block_count,
                 const std::size_t dst_block_offset,
                 const std::uint16_t* src,
                 const std::size_t src_block_count,
                 const std::size_t src_block_offset,
                 const std::size_t blocks_to_copy) {
  if ((dst == nullptr) || (src == nullptr) || (blocks_to_copy == 0U)) {
    return;
  }

  if ((dst_block_offset >= dst_block_count) || (src_block_offset >= src_block_count)) {
    return;
  }

  const std::size_t dst_available = dst_block_count - dst_block_offset;
  const std::size_t src_available = src_block_count - src_block_offset;
  const std::size_t count = std::min(blocks_to_copy, std::min(dst_available, src_available));
  for (std::size_t i = 0; i < count; ++i) {
    dst[dst_block_offset + i] = src[src_block_offset + i];
  }
}

}  // namespace imsgblock
