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
  bool is_signed{false};
  std::int8_t tar_a{0};
  std::int8_t tar_b{0};
  std::int8_t tar_c{0};
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

namespace PAR {

double ac_forward(const resolved_item_t& item, const double raw_value) {
  const double a = static_cast<double>(item.tar_a);
  const double b = static_cast<double>(item.tar_b);
  const double c = static_cast<double>(item.tar_c);
  return (a * raw_value * raw_value) + (b * raw_value) + c;
}

float process_value(const resolved_item_t& item, const std::int32_t raw_value) {
  if (item.alg == ALG_AC) {
    return static_cast<float>(ac_forward(item, static_cast<double>(raw_value)));
  }

  // ALG_AN: no processing and no tar coefficients.
  return static_cast<float>(raw_value);
}

}  // namespace PAR

namespace GEN {

void add_candidate(std::int64_t* candidates,
                   std::size_t& count,
                   const std::size_t capacity,
                   const std::int64_t value) {
  if ((value < static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min())) ||
      (value > static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max()))) {
    return;
  }

  for (std::size_t i = 0; i < count; ++i) {
    if (candidates[i] == value) {
      return;
    }
  }

  if (count < capacity) {
    candidates[count++] = value;
  }
}

void add_root_candidates(std::int64_t* candidates,
                         std::size_t& count,
                         const std::size_t capacity,
                         const double root) {
  if (!std::isfinite(root)) {
    return;
  }

  add_candidate(candidates, count, capacity, static_cast<std::int64_t>(std::llround(root)));
  add_candidate(candidates, count, capacity, static_cast<std::int64_t>(std::floor(root)));
  add_candidate(candidates, count, capacity, static_cast<std::int64_t>(std::ceil(root)));
}

status_t apply_ac_inverse(const resolved_item_t& item,
                          const std::int32_t processed_value,
                          std::int32_t& raw_out) {
  constexpr std::size_t k_max_candidates = 8U;
  std::int64_t candidates[k_max_candidates]{};
  std::size_t candidate_count = 0U;

  const double y = static_cast<double>(processed_value);
  const double a = static_cast<double>(item.tar_a);
  const double b = static_cast<double>(item.tar_b);
  const double c = static_cast<double>(item.tar_c);

  if (item.tar_a == 0) {
    if (item.tar_b == 0) {
      if (processed_value == item.tar_c) {
        raw_out = 0;
        return status_t::ok;
      }
      return status_t::value_out_of_range;
    }

    const double root = (y - c) / b;
    add_root_candidates(candidates, candidate_count, k_max_candidates, root);
  } else {
    const double discr = (b * b) - (4.0 * a * (c - y));
    if (discr < 0.0) {
      return status_t::value_out_of_range;
    }

    const double sqrt_discr = std::sqrt(discr);
    const double denom = 2.0 * a;
    add_root_candidates(candidates, candidate_count, k_max_candidates, (-b + sqrt_discr) / denom);
    add_root_candidates(candidates, candidate_count, k_max_candidates, (-b - sqrt_discr) / denom);
  }

  if (candidate_count == 0U) {
    return status_t::value_out_of_range;
  }

  std::int64_t best_candidate = candidates[0];
  double best_error = std::numeric_limits<double>::infinity();
  for (std::size_t i = 0; i < candidate_count; ++i) {
    const double eval = PAR::ac_forward(item, static_cast<double>(candidates[i]));
    const double error = std::abs(eval - y);
    if (error < best_error) {
      best_error = error;
      best_candidate = candidates[i];
    }
  }

  if (best_error > 1e-6) {
    return status_t::value_out_of_range;
  }

  raw_out = static_cast<std::int32_t>(best_candidate);
  return status_t::ok;
}

status_t prepare_raw_value(const resolved_item_t& item,
                           const std::int32_t input_value,
                           std::int32_t& raw_value_out) {
  if (item.alg == ALG_AC) {
    return apply_ac_inverse(item, input_value, raw_value_out);
  }

  // ALG_AN: direct write without processing.
  raw_value_out = input_value;
  return status_t::ok;
}

}  // namespace GEN

status_t encode_to_bits(const std::int32_t value,
                        const std::uint8_t width_bits,
                        const bool is_signed,
                        std::uint32_t& raw_out) {
  const std::uint32_t mask = (1U << width_bits) - 1U;
  if (is_signed) {
    const std::int64_t min_v = -(1LL << (width_bits - 1U));
    const std::int64_t max_v = (1LL << (width_bits - 1U)) - 1LL;
    if ((static_cast<std::int64_t>(value) < min_v) || (static_cast<std::int64_t>(value) > max_v)) {
      return status_t::value_out_of_range;
    }
    raw_out = static_cast<std::uint32_t>(value) & mask;
    return status_t::ok;
  }

  if (value < 0) {
    return status_t::value_out_of_range;
  }

  if (static_cast<std::uint32_t>(value) > mask) {
    return status_t::value_out_of_range;
  }

  raw_out = static_cast<std::uint32_t>(value);
  return status_t::ok;
}

status_t fill_item_layout(const std::uint16_t* read_buf,
                          std::uint16_t* write_buf,
                          const std::size_t block_count,
                          const int block_n,
                          const int block_offset,
                          const std::uint8_t type,
                          const std::uint8_t sign,
                          const std::int8_t tar_a,
                          const std::int8_t tar_b,
                          const std::int8_t tar_c,
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
  out.is_signed = (sign == TYPE_SIGN);
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
    return fill_item_layout(ctx.msg_par,
                            nullptr,
                            ctx.msg_par_blocks,
                            static_cast<int>(p.msg_cs_block_n),
                            static_cast<int>(p.msg_block_offset),
                            static_cast<std::uint8_t>(p.type),
                            static_cast<std::uint8_t>(p.sign),
                            static_cast<std::int8_t>(p.tar_a),
                            static_cast<std::int8_t>(p.tar_b),
                            static_cast<std::int8_t>(p.tar_c),
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

  return fill_item_layout(ctx.msg_com,
                          nullptr,
                          ctx.msg_com_blocks,
                          static_cast<int>(c.msg_block_n),
                          static_cast<int>(c.msg_block_offset),
                          static_cast<std::uint8_t>(c.type),
                          static_cast<std::uint8_t>(c.sign),
                          static_cast<std::int8_t>(c.tar_a),
                          static_cast<std::int8_t>(c.tar_b),
                          static_cast<std::int8_t>(c.tar_c),
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
                            ctx.msg_com,
                            ctx.msg_com_blocks,
                            static_cast<int>(c.msg_cs_block_n),
                            static_cast<int>(c.msg_block_offset),
                            static_cast<std::uint8_t>(c.type),
                            static_cast<std::uint8_t>(c.sign),
                            static_cast<std::int8_t>(c.tar_a),
                            static_cast<std::int8_t>(c.tar_b),
                            static_cast<std::int8_t>(c.tar_c),
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
                          ctx.msg_par,
                          ctx.msg_par_blocks,
                          static_cast<int>(p.msg_block_n),
                          static_cast<int>(p.msg_block_offset),
                          static_cast<std::uint8_t>(p.type),
                          static_cast<std::uint8_t>(p.sign),
                          static_cast<std::int8_t>(p.tar_a),
                          static_cast<std::int8_t>(p.tar_b),
                          static_cast<std::int8_t>(p.tar_c),
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

parse_result_t IPAR(const context_t& ctx, const std::uint16_t id) {
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

  const std::int32_t decoded = decode_raw(raw, item.width_bits, item.is_signed);
  const float processed = PAR::process_value(item, decoded);
  return {status_t::ok, processed};
}

parse_result_t IPAR(const std::uint16_t id) {
  if (g_ctx == nullptr) {
    return {status_t::no_context, 0.0F};
  }
  return IPAR(*g_ctx, id);
}

status_t IGEN(const context_t& ctx, const std::uint16_t id, const std::int32_t value) {
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
  const status_t encode_status = encode_to_bits(prepared_raw, item.width_bits, item.is_signed, raw);
  if (encode_status != status_t::ok) {
    return encode_status;
  }

  if (!imsgblock::write_value(item.write_buf, item.block_count, item.block_n, item.block_offset, item.width_bits, raw)) {
    return status_t::buffer_too_small;
  }
  return status_t::ok;
}

status_t IGEN(const std::uint16_t id, const std::int32_t value) {
  if (g_ctx == nullptr) {
    return status_t::no_context;
  }
  return IGEN(*g_ctx, id, value);
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
