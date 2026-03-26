#include "i_par.hpp"

#include <cmath>
#include <limits>

#include "command_tab.hpp"
#include "i_msg_block.hpp"
#include "param_tab.hpp"

namespace ipar {
namespace {

struct resolved_item_t {
  const std::uint16_t* read_buf{nullptr};
  std::uint16_t* write_buf{nullptr};
  std::size_t block_count{0};
  std::size_t block_n{0};
  std::uint8_t block_offset{0};
  std::uint8_t width_bits{0};
  bool wire_signed{false};
  std::uint8_t type_real{RTYPE_NONE};
  double tar_a{0.0};
  double tar_b{0.0};
  double tar_c{0.0};
  std::uint8_t alg{ALG_NONE};
};

thread_local const context_t* g_ctx = nullptr;

constexpr bool is_param_id(const std::uint16_t id) {
  return id < Param_max;
}

constexpr bool is_comm_id(const std::uint16_t id) {
  return (id >= Param_max) && (id < Param_Comm_max);
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

std::int32_t decode_raw(const std::uint32_t raw,
                        const std::uint8_t width_bits,
                        const bool is_signed) {
  const std::uint32_t full_mask = (1U << width_bits) - 1U;
  const std::uint32_t clipped = raw & full_mask;
  if (!is_signed) {
    return static_cast<std::int32_t>(clipped);
  }

  const std::uint32_t sign_bit = (1U << (width_bits - 1U));
  if ((clipped & sign_bit) == 0U) {
    return static_cast<std::int32_t>(clipped);
  }

  const std::uint32_t extended = clipped | ~full_mask;
  return static_cast<std::int32_t>(extended);
}

void wire_range(const resolved_item_t& item, std::int64_t& min_v, std::int64_t& max_v) {
  if (item.wire_signed) {
    min_v = -(1LL << (item.width_bits - 1U));
    max_v = (1LL << (item.width_bits - 1U)) - 1LL;
    return;
  }

  min_v = 0;
  max_v = (1LL << item.width_bits) - 1LL;
}

std::int64_t clamp_wire_value(const resolved_item_t& item, const std::int64_t value) {
  std::int64_t min_v = 0;
  std::int64_t max_v = 0;
  wire_range(item, min_v, max_v);
  if (value < min_v) {
    return min_v;
  }
  if (value > max_v) {
    return max_v;
  }
  return value;
}

namespace PAR {

double decode_processed(const resolved_item_t& item, const double raw_value) {
  switch (item.alg) {
    case ALG_NONE:
    case ALG_AN:
      return raw_value;
    case ALG_AC:
      return raw_value + item.tar_c;
    case ALG_ABC:
      return (item.tar_b * raw_value) + item.tar_c;
    case ALG_AAC:
      return (item.tar_a * raw_value * raw_value) + item.tar_c;
    default:
      return raw_value;
  }
}

}  // namespace PAR

namespace GEN {

void add_candidate(std::int64_t* candidates,
                   std::size_t& count,
                   const std::size_t capacity,
                   const std::int64_t value) {
  for (std::size_t i = 0; i < count; ++i) {
    if (candidates[i] == value) {
      return;
    }
  }

  if (count < capacity) {
    candidates[count++] = value;
  }
}

void add_quant_candidates(std::int64_t* candidates,
                          std::size_t& count,
                          const std::size_t capacity,
                          const double q_real) {
  if (!std::isfinite(q_real)) {
    return;
  }

  add_candidate(candidates, count, capacity, static_cast<std::int64_t>(std::llround(q_real)));
  add_candidate(candidates, count, capacity, static_cast<std::int64_t>(std::floor(q_real)));
  add_candidate(candidates, count, capacity, static_cast<std::int64_t>(std::ceil(q_real)));
}

status_t prepare_raw_value(const resolved_item_t& item, const float input_value, std::int32_t& raw_value_out) {
  constexpr std::size_t k_max_candidates = 12U;
  std::int64_t candidates[k_max_candidates]{};
  std::size_t candidate_count = 0U;

  const double desired = static_cast<double>(input_value);
  switch (item.alg) {
    case ALG_NONE:
    case ALG_AN:
      add_quant_candidates(candidates, candidate_count, k_max_candidates, desired);
      break;
    case ALG_AC:
      add_quant_candidates(candidates, candidate_count, k_max_candidates, desired - item.tar_c);
      break;
    case ALG_ABC:
      if (item.tar_b == 0.0) {
        return status_t::value_out_of_range;
      }
      add_quant_candidates(candidates, candidate_count, k_max_candidates, (desired - item.tar_c) / item.tar_b);
      break;
    case ALG_AAC: {
      if (item.tar_a == 0.0) {
        return status_t::value_out_of_range;
      }
      const double ratio = (desired - item.tar_c) / item.tar_a;
      if (ratio < 0.0) {
        return status_t::value_out_of_range;
      }
      const double root = std::sqrt(ratio);
      add_quant_candidates(candidates, candidate_count, k_max_candidates, root);
      add_quant_candidates(candidates, candidate_count, k_max_candidates, -root);
      break;
    }
    default:
      return status_t::bad_type;
  }

  if (candidate_count == 0U) {
    return status_t::value_out_of_range;
  }

  std::int64_t best_candidate = clamp_wire_value(item, candidates[0]);
  double best_error = std::numeric_limits<double>::infinity();
  for (std::size_t i = 0; i < candidate_count; ++i) {
    const std::int64_t clamped = clamp_wire_value(item, candidates[i]);
    const double reconstructed = PAR::decode_processed(item, static_cast<double>(clamped));
    const double error = std::abs(reconstructed - desired);
    if (error < best_error) {
      best_error = error;
      best_candidate = clamped;
    }
  }

  raw_value_out = static_cast<std::int32_t>(best_candidate);
  return status_t::ok;
}

}  // namespace GEN

status_t encode_to_bits(const std::int32_t value,
                        const std::uint8_t width_bits,
                        const bool is_signed,
                        std::uint32_t& raw_out) {
  const std::uint32_t mask = (1U << width_bits) - 1U;
  if (is_signed) {
    raw_out = static_cast<std::uint32_t>(value) & mask;
    return status_t::ok;
  }

  raw_out = static_cast<std::uint32_t>(value) & mask;
  return status_t::ok;
}

status_t fill_item_layout(const std::uint16_t* read_buf,
                          std::uint16_t* write_buf,
                          const std::size_t block_count,
                          const int block_n,
                          const int block_offset,
                          const std::uint8_t type,
                          const std::uint8_t type_real,
                          const std::uint8_t sign,
                          const double tar_a,
                          const double tar_b,
                          const double tar_c,
                          const std::uint8_t alg,
                          resolved_item_t& out) {
  if ((read_buf == nullptr) && (write_buf == nullptr)) {
    return status_t::null_buffer;
  }

  if (block_count == 0U) {
    return status_t::null_buffer;
  }

  const std::uint8_t width = type_width_bits(type);
  if (width == 0U) {
    return status_t::bad_type;
  }

  if ((block_n < 0) || (static_cast<std::size_t>(block_n) >= block_count)) {
    return status_t::bad_layout;
  }

  if (!imsgblock::layout_supported(width, block_offset)) {
    return status_t::bad_layout;
  }

  out.read_buf = read_buf;
  out.write_buf = write_buf;
  out.block_count = block_count;
  out.block_n = static_cast<std::size_t>(block_n);
  out.block_offset = static_cast<std::uint8_t>(block_offset);
  out.width_bits = width;
  out.wire_signed = (sign == TYPE_SIGN);
  out.type_real = type_real;
  out.tar_a = tar_a;
  out.tar_b = tar_b;
  out.tar_c = tar_c;
  out.alg = alg;
  return status_t::ok;
}

bool system_mismatch(const context_t& ctx, const std::uint8_t table_sys_id) {
  if (ctx.system_id == SYS_NONE) {
    return false;
  }
  return ctx.system_id != table_sys_id;
}

status_t resolve_for_parse(const context_t& ctx, const std::uint16_t id, resolved_item_t& out) {
  if (ctx.role == role_t::cs) {
    if (!is_param_id(id)) {
      return status_t::direction_mismatch;
    }

    const S_paramtab& p = S_baseparamtab[id];
    return fill_item_layout(ctx.msg_prm,
                            nullptr,
                            ctx.msg_prm_blocks,
                            static_cast<int>(p.msg_cs_block_n),
                            static_cast<int>(p.msg_block_offset),
                            static_cast<std::uint8_t>(p.type),
                            static_cast<std::uint8_t>(p.type_real),
                            static_cast<std::uint8_t>(p.sign),
                            p.tar_a,
                            p.tar_b,
                            p.tar_c,
                            static_cast<std::uint8_t>(p.alg),
                            out);
  }

  if (!is_comm_id(id)) {
    return status_t::direction_mismatch;
  }

  const std::uint16_t comm_index = static_cast<std::uint16_t>(id - Param_max);
  const S_commtab& c = S_basecommtab[comm_index];
  if (system_mismatch(ctx, static_cast<std::uint8_t>(c.sys_id))) {
    return status_t::system_mismatch;
  }

  return fill_item_layout(ctx.msg_cmd,
                          nullptr,
                          ctx.msg_cmd_blocks,
                          static_cast<int>(c.msg_block_n),
                          static_cast<int>(c.msg_block_offset),
                          static_cast<std::uint8_t>(c.type),
                          static_cast<std::uint8_t>(c.type_real),
                          static_cast<std::uint8_t>(c.sign),
                          c.tar_a,
                          c.tar_b,
                          c.tar_c,
                          static_cast<std::uint8_t>(c.alg),
                          out);
}

status_t resolve_for_generate(const context_t& ctx, const std::uint16_t id, resolved_item_t& out) {
  if (ctx.role == role_t::cs) {
    if (!is_comm_id(id)) {
      return status_t::direction_mismatch;
    }

    const std::uint16_t comm_index = static_cast<std::uint16_t>(id - Param_max);
    const S_commtab& c = S_basecommtab[comm_index];
    return fill_item_layout(nullptr,
                            ctx.msg_cmd,
                            ctx.msg_cmd_blocks,
                            static_cast<int>(c.msg_cs_block_n),
                            static_cast<int>(c.msg_block_offset),
                            static_cast<std::uint8_t>(c.type),
                            static_cast<std::uint8_t>(c.type_real),
                            static_cast<std::uint8_t>(c.sign),
                            c.tar_a,
                            c.tar_b,
                            c.tar_c,
                            static_cast<std::uint8_t>(c.alg),
                            out);
  }

  if (!is_param_id(id)) {
    return status_t::direction_mismatch;
  }

  const S_paramtab& p = S_baseparamtab[id];
  if (system_mismatch(ctx, static_cast<std::uint8_t>(p.sys_id))) {
    return status_t::system_mismatch;
  }

  return fill_item_layout(nullptr,
                          ctx.msg_prm,
                          ctx.msg_prm_blocks,
                          static_cast<int>(p.msg_block_n),
                          static_cast<int>(p.msg_block_offset),
                          static_cast<std::uint8_t>(p.type),
                          static_cast<std::uint8_t>(p.type_real),
                          static_cast<std::uint8_t>(p.sign),
                          p.tar_a,
                          p.tar_b,
                          p.tar_c,
                          static_cast<std::uint8_t>(p.alg),
                          out);
}

}  // namespace

void bind_context(const context_t* ctx) {
  g_ctx = ctx;
}

const context_t* bound_context() {
  return g_ctx;
}

parse_result_t IMSGGET(const context_t& ctx, const std::uint16_t id) {
  if (!is_param_id(id) && !is_comm_id(id)) {
    return {status_t::id_out_of_range, 0.0F};
  }

  resolved_item_t item{};
  const status_t resolve_status = resolve_for_parse(ctx, id, item);
  if (resolve_status != status_t::ok) {
    return {resolve_status, 0.0F};
  }

  std::uint32_t raw = 0U;
  if (!imsgblock::read_value(item.read_buf, item.block_count, item.block_n, item.block_offset, item.width_bits, raw)) {
    return {status_t::buffer_too_small, 0.0F};
  }

  const std::int32_t decoded = decode_raw(raw, item.width_bits, item.wire_signed);
  const float processed = static_cast<float>(PAR::decode_processed(item, static_cast<double>(decoded)));
  return {status_t::ok, processed};
}

parse_result_t IMSGGET(const std::uint16_t id) {
  if (g_ctx == nullptr) {
    return {status_t::no_context, 0.0F};
  }
  return IMSGGET(*g_ctx, id);
}

status_t IMSGSET(const context_t& ctx, const std::uint16_t id, const float value) {
  if (!is_param_id(id) && !is_comm_id(id)) {
    return status_t::id_out_of_range;
  }

  resolved_item_t item{};
  const status_t resolve_status = resolve_for_generate(ctx, id, item);
  if (resolve_status != status_t::ok) {
    return resolve_status;
  }

  std::int32_t prepared_raw = 0;
  const status_t prep_status = GEN::prepare_raw_value(item, value, prepared_raw);
  if (prep_status != status_t::ok) {
    return prep_status;
  }

  std::uint32_t raw = 0U;
  const status_t encode_status = encode_to_bits(prepared_raw, item.width_bits, item.wire_signed, raw);
  if (encode_status != status_t::ok) {
    return encode_status;
  }

  if (!imsgblock::write_value(item.write_buf, item.block_count, item.block_n, item.block_offset, item.width_bits, raw)) {
    return status_t::buffer_too_small;
  }
  return status_t::ok;
}

status_t IMSGSET(const std::uint16_t id, const float value) {
  if (g_ctx == nullptr) {
    return status_t::no_context;
  }
  return IMSGSET(*g_ctx, id, value);
}

const char* to_string(const status_t status) {
  switch (status) {
    case status_t::ok:
      return "OK";
    case status_t::no_context:
      return "NO_CONTEXT";
    case status_t::id_out_of_range:
      return "ID_OUT_OF_RANGE";
    case status_t::direction_mismatch:
      return "DIRECTION_MISMATCH";
    case status_t::system_mismatch:
      return "SYSTEM_MISMATCH";
    case status_t::null_buffer:
      return "NULL_BUFFER";
    case status_t::bad_type:
      return "BAD_TYPE";
    case status_t::bad_layout:
      return "BAD_LAYOUT";
    case status_t::buffer_too_small:
      return "BUFFER_TOO_SMALL";
    case status_t::value_out_of_range:
      return "VALUE_OUT_OF_RANGE";
    default:
      return "UNKNOWN_STATUS";
  }
}

}  // namespace ipar
