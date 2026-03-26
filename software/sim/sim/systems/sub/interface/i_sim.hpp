#pragma once

#include <cstddef>
#include <cstdint>

#include "../../base/local_param_list.hpp"

namespace isim {

enum class value_kind_t : std::uint8_t {
  u8 = 0,
  s8,
  u16,
  s16,
  u32,
  s32,
  f32,
};

enum class value_domain_t : std::uint8_t {
  plain = 0,
  non_negative,
  binary,
};

enum class status_t : std::uint8_t {
  ok = 0,
  no_registry,
  bad_argument,
  id_not_found,
  type_mismatch,
  value_out_of_range,
  null_pointer,
};

struct value_t {
  value_kind_t kind{value_kind_t::s32};
  union {
    std::uint8_t u8;
    std::int8_t s8;
    std::uint16_t u16;
    std::int16_t s16;
    std::uint32_t u32;
    std::int32_t s32;
    float f32;
  } data{.s32 = 0};
};

struct binding_t {
  std::uint16_t id{0};
  value_kind_t kind{value_kind_t::s32};
  value_domain_t domain{value_domain_t::plain};
  void* ptr{nullptr};
};

struct registry_t {
  const binding_t* bindings{nullptr};  // Must be sorted by id (ascending).
  std::size_t count{0};
  std::uint8_t system_id{0};
};

void bind_registry(const registry_t* registry);
const registry_t* bound_registry();

std::int32_t ISIMI32(const value_t& value);
float ISIMF32(const value_t& value);
status_t ISIMGET(std::uint16_t id, value_t* out_value);
status_t ISIMSET(std::uint16_t id, const value_t* in_value);
status_t ISIMGETI32(std::uint16_t id, std::int32_t* out_value);
status_t ISIMSETI32(std::uint16_t id, std::int32_t value);
status_t ISIMGETF32(std::uint16_t id, float* out_value);
status_t ISIMSETF32(std::uint16_t id, float value);

const char* to_string(status_t status);

}  // namespace isim
