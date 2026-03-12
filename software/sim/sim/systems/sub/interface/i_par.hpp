#pragma once

#include <cstddef>
#include <cstdint>

#include "sys_list.hpp"

namespace ipar {

constexpr std::size_t k_msg_block_bits = 16U;

enum class role_t : std::uint8_t {
  cs = 0,
  device = 1,
};

enum class status_t : std::uint8_t {
  ok = 0,
  no_context,
  id_out_of_range,
  direction_mismatch,
  system_mismatch,
  null_buffer,
  bad_type,
  bad_layout,
  buffer_too_small,
  value_out_of_range,
};

struct context_t {
  role_t role{role_t::device};
  std::uint8_t system_id{SYS_NONE};
  std::uint16_t* msg_par{nullptr};
  std::size_t msg_par_blocks{0};
  std::uint16_t* msg_com{nullptr};
  std::size_t msg_com_blocks{0};
};

struct parse_result_t {
  status_t status{status_t::ok};
  float value{0.0F};
};

// Bind thread-local context for wrappers IPAR(id)/IGEN(id, value).
void bind_context(const context_t* ctx);

// Read currently bound thread-local context.
const context_t* bound_context();

// Parse incoming value by global identifier (ID from param_comm_list.hpp).
parse_result_t IPAR(const context_t& ctx, std::uint16_t id);
parse_result_t IPAR(std::uint16_t id);

// Generate outgoing value by global identifier.
status_t IGEN(const context_t& ctx, std::uint16_t id, std::int32_t value = 1);
status_t IGEN(std::uint16_t id, std::int32_t value = 1);

const char* to_string(status_t status);

}  // namespace ipar
