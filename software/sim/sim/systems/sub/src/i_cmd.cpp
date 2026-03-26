#include "i_cmd.hpp"

#include <cmath>

#include "command_tab.hpp"
#include "i_msg_block.hpp"

namespace icmd {
namespace {

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

bool read_command_raw(const ipar::context_t& ctx, const std::uint16_t id, std::uint32_t& out_raw) {
  if ((ctx.msg_cmd == nullptr) || (id < Param_max) || (id >= Param_Comm_max)) {
    return false;
  }

  const std::uint16_t comm_index = static_cast<std::uint16_t>(id - Param_max);
  const auto& entry = S_basecommtab[comm_index];
  const auto width_bits = type_width_bits(static_cast<std::uint8_t>(entry.type));
  if (width_bits == 0U) {
    return false;
  }

  const auto block_n = (ctx.role == ipar::role_t::cs) ? static_cast<std::size_t>(static_cast<unsigned char>(entry.msg_cs_block_n))
                                                      : static_cast<std::size_t>(static_cast<unsigned char>(entry.msg_block_n));

  return imsgblock::read_value(ctx.msg_cmd,
                               ctx.msg_cmd_blocks,
                               block_n,
                               static_cast<std::uint8_t>(entry.msg_block_offset),
                               width_bits,
                               out_raw);
}

}  // namespace

ipar::parse_result_t ICMDPAR(const ipar::context_t& ctx, const std::uint16_t id) {
  return ipar::IMSGGET(ctx, id);
}

float ICMDF32(const ipar::context_t& ctx, const std::uint16_t id, const float fallback) {
  const auto parsed = ipar::IMSGGET(ctx, id);
  if (parsed.status != ipar::status_t::ok) {
    return fallback;
  }
  return parsed.value;
}

std::int32_t ICMDVAL(const ipar::context_t& ctx, const std::uint16_t id, const std::int32_t fallback) {
  const auto parsed = ipar::IMSGGET(ctx, id);
  if (parsed.status != ipar::status_t::ok) {
    return fallback;
  }
  return static_cast<std::int32_t>(std::lround(parsed.value));
}

bool ICMDACT(const ipar::context_t& ctx, const std::uint16_t id) {
  std::uint32_t raw = 0U;
  return read_command_raw(ctx, id, raw) && (raw != 0U);
}

void ICMDCLEAR(const ipar::context_t& ctx, const std::uint16_t id) {
  if ((ctx.msg_cmd == nullptr) || (id < Param_max) || (id >= Param_Comm_max)) {
    return;
  }

  const std::uint16_t comm_index = static_cast<std::uint16_t>(id - Param_max);
  const auto& entry = S_basecommtab[comm_index];
  const auto width_bits = type_width_bits(static_cast<std::uint8_t>(entry.type));
  if (width_bits == 0U) {
    return;
  }

  const auto block_n = (ctx.role == ipar::role_t::cs) ? static_cast<std::size_t>(static_cast<unsigned char>(entry.msg_cs_block_n))
                                                      : static_cast<std::size_t>(static_cast<unsigned char>(entry.msg_block_n));

  (void)imsgblock::write_value(ctx.msg_cmd,
                               ctx.msg_cmd_blocks,
                               block_n,
                               static_cast<std::uint8_t>(entry.msg_block_offset),
                               width_bits,
                               0U);
}

}  // namespace icmd
