#include "i_sim.hpp"

#include <algorithm>
#include <cmath>
#include <mutex>

#include "sim_base.hpp"

namespace isim {
namespace {

thread_local const registry_t* G_REGISTRY = nullptr;

const binding_t* find_binding(const registry_t& registry, const std::uint16_t id) {
  if ((registry.bindings == nullptr) || (registry.count == 0U)) {
    return nullptr;
  }

  const binding_t* begin = registry.bindings;
  const binding_t* end = begin + registry.count;
  const binding_t* it = std::lower_bound(begin, end, id, [](const binding_t& lhs, const std::uint16_t rhs_id) {
    return lhs.id < rhs_id;
  });
  if ((it == end) || (it->id != id)) {
    return nullptr;
  }
  return it;
}

std::int32_t clamp_i32(const std::int32_t value, const std::int32_t lo, const std::int32_t hi) {
  return std::clamp(value, lo, hi);
}

float clamp_f32_domain(const binding_t& binding, const float value) {
  if (binding.domain == value_domain_t::binary) {
    return (value != 0.0F) ? 1.0F : 0.0F;
  }
  if ((binding.domain == value_domain_t::non_negative) && (value < 0.0F)) {
    return 0.0F;
  }
  return value;
}

status_t read_typed_value(const binding_t& binding, value_t* out_value) {
  if (out_value == nullptr) {
    return status_t::bad_argument;
  }
  if (binding.ptr == nullptr) {
    return status_t::null_pointer;
  }

  out_value->kind = binding.kind;
  switch (binding.kind) {
    case value_kind_t::u8:
      out_value->data.u8 = *static_cast<const std::uint8_t*>(binding.ptr);
      return status_t::ok;
    case value_kind_t::s8:
      out_value->data.s8 = *static_cast<const std::int8_t*>(binding.ptr);
      return status_t::ok;
    case value_kind_t::u16:
      out_value->data.u16 = *static_cast<const std::uint16_t*>(binding.ptr);
      return status_t::ok;
    case value_kind_t::s16:
      out_value->data.s16 = *static_cast<const std::int16_t*>(binding.ptr);
      return status_t::ok;
    case value_kind_t::u32:
      out_value->data.u32 = *static_cast<const std::uint32_t*>(binding.ptr);
      return status_t::ok;
    case value_kind_t::s32:
      out_value->data.s32 = *static_cast<const std::int32_t*>(binding.ptr);
      return status_t::ok;
    case value_kind_t::f32:
      out_value->data.f32 = *static_cast<const float*>(binding.ptr);
      return status_t::ok;
    default:
      return status_t::type_mismatch;
  }
}

status_t write_typed_value(const binding_t& binding, const value_t* in_value) {
  if (in_value == nullptr) {
    return status_t::bad_argument;
  }
  if (binding.ptr == nullptr) {
    return status_t::null_pointer;
  }
  if (in_value->kind != binding.kind) {
    return status_t::type_mismatch;
  }

  switch (binding.kind) {
    case value_kind_t::u8: {
      std::uint8_t value = in_value->data.u8;
      if (binding.domain == value_domain_t::binary) {
        value = (value != 0U) ? 1U : 0U;
      }
      *static_cast<std::uint8_t*>(binding.ptr) = value;
      return status_t::ok;
    }
    case value_kind_t::s8:
      *static_cast<std::int8_t*>(binding.ptr) = in_value->data.s8;
      return status_t::ok;
    case value_kind_t::u16:
      *static_cast<std::uint16_t*>(binding.ptr) = in_value->data.u16;
      return status_t::ok;
    case value_kind_t::s16:
      *static_cast<std::int16_t*>(binding.ptr) = in_value->data.s16;
      return status_t::ok;
    case value_kind_t::u32:
      *static_cast<std::uint32_t*>(binding.ptr) = in_value->data.u32;
      return status_t::ok;
    case value_kind_t::s32:
      *static_cast<std::int32_t*>(binding.ptr) = in_value->data.s32;
      return status_t::ok;
    case value_kind_t::f32:
      *static_cast<float*>(binding.ptr) = clamp_f32_domain(binding, in_value->data.f32);
      return status_t::ok;
    default:
      return status_t::type_mismatch;
  }
}

}  // namespace

void bind_registry(const registry_t* registry) {
  G_REGISTRY = registry;
}

const registry_t* bound_registry() {
  return G_REGISTRY;
}

std::int32_t ISIMI32(const value_t& value) {
  switch (value.kind) {
    case value_kind_t::u8:
      return static_cast<std::int32_t>(value.data.u8);
    case value_kind_t::s8:
      return static_cast<std::int32_t>(value.data.s8);
    case value_kind_t::u16:
      return static_cast<std::int32_t>(value.data.u16);
    case value_kind_t::s16:
      return static_cast<std::int32_t>(value.data.s16);
    case value_kind_t::u32:
      return static_cast<std::int32_t>(value.data.u32);
    case value_kind_t::s32:
      return value.data.s32;
    case value_kind_t::f32:
      return static_cast<std::int32_t>(std::lround(value.data.f32));
    default:
      return 0;
  }
}

float ISIMF32(const value_t& value) {
  switch (value.kind) {
    case value_kind_t::u8:
      return static_cast<float>(value.data.u8);
    case value_kind_t::s8:
      return static_cast<float>(value.data.s8);
    case value_kind_t::u16:
      return static_cast<float>(value.data.u16);
    case value_kind_t::s16:
      return static_cast<float>(value.data.s16);
    case value_kind_t::u32:
      return static_cast<float>(value.data.u32);
    case value_kind_t::s32:
      return static_cast<float>(value.data.s32);
    case value_kind_t::f32:
      return value.data.f32;
    default:
      return 0.0F;
  }
}

status_t ISIMGET(const std::uint16_t id, value_t* out_value) {
  if (G_REGISTRY == nullptr) {
    return status_t::no_registry;
  }

  std::lock_guard<std::recursive_mutex> lock(sim_base::model_data_mutex());
  const binding_t* binding = find_binding(*G_REGISTRY, id);
  if (binding == nullptr) {
    return status_t::id_not_found;
  }

  return read_typed_value(*binding, out_value);
}

status_t ISIMSET(const std::uint16_t id, const value_t* in_value) {
  if (G_REGISTRY == nullptr) {
    return status_t::no_registry;
  }

  std::lock_guard<std::recursive_mutex> lock(sim_base::model_data_mutex());
  const binding_t* binding = find_binding(*G_REGISTRY, id);
  if (binding == nullptr) {
    return status_t::id_not_found;
  }

  return write_typed_value(*binding, in_value);
}

status_t ISIMGETI32(const std::uint16_t id, std::int32_t* out_value) {
  if (out_value == nullptr) {
    return status_t::bad_argument;
  }

  value_t value{};
  const status_t status = ISIMGET(id, &value);
  if (status != status_t::ok) {
    return status;
  }

  *out_value = ISIMI32(value);
  return status_t::ok;
}

status_t ISIMSETI32(const std::uint16_t id, const std::int32_t value) {
  if (G_REGISTRY == nullptr) {
    return status_t::no_registry;
  }

  std::lock_guard<std::recursive_mutex> lock(sim_base::model_data_mutex());
  const binding_t* binding = find_binding(*G_REGISTRY, id);
  if (binding == nullptr) {
    return status_t::id_not_found;
  }

  value_t payload{};
  payload.kind = binding->kind;
  switch (binding->kind) {
    case value_kind_t::u8: {
      const std::int32_t clamped = clamp_i32(value, 0, 255);
      payload.data.u8 = static_cast<std::uint8_t>(clamped);
      break;
    }
    case value_kind_t::s8:
      payload.data.s8 = static_cast<std::int8_t>(clamp_i32(value, -128, 127));
      break;
    case value_kind_t::u16:
      payload.data.u16 = static_cast<std::uint16_t>(clamp_i32(value, 0, 65535));
      break;
    case value_kind_t::s16:
      payload.data.s16 = static_cast<std::int16_t>(clamp_i32(value, -32768, 32767));
      break;
    case value_kind_t::u32:
      if (value < 0) {
        return status_t::value_out_of_range;
      }
      payload.data.u32 = static_cast<std::uint32_t>(value);
      break;
    case value_kind_t::s32:
      payload.data.s32 = value;
      break;
    case value_kind_t::f32:
      payload.data.f32 = static_cast<float>(value);
      break;
    default:
      return status_t::type_mismatch;
  }

  return write_typed_value(*binding, &payload);
}

status_t ISIMGETF32(const std::uint16_t id, float* out_value) {
  if (out_value == nullptr) {
    return status_t::bad_argument;
  }

  value_t value{};
  const status_t status = ISIMGET(id, &value);
  if (status != status_t::ok) {
    return status;
  }

  *out_value = ISIMF32(value);
  return status_t::ok;
}

status_t ISIMSETF32(const std::uint16_t id, const float value) {
  if (G_REGISTRY == nullptr) {
    return status_t::no_registry;
  }

  std::lock_guard<std::recursive_mutex> lock(sim_base::model_data_mutex());
  const binding_t* binding = find_binding(*G_REGISTRY, id);
  if (binding == nullptr) {
    return status_t::id_not_found;
  }

  value_t payload{};
  payload.kind = binding->kind;
  switch (binding->kind) {
    case value_kind_t::u8: {
      const auto rounded = static_cast<std::int32_t>(std::lround(value));
      payload.data.u8 = static_cast<std::uint8_t>(clamp_i32(rounded, 0, 255));
      break;
    }
    case value_kind_t::s8: {
      const auto rounded = static_cast<std::int32_t>(std::lround(value));
      payload.data.s8 = static_cast<std::int8_t>(clamp_i32(rounded, -128, 127));
      break;
    }
    case value_kind_t::u16: {
      const auto rounded = static_cast<std::int32_t>(std::lround(value));
      payload.data.u16 = static_cast<std::uint16_t>(clamp_i32(rounded, 0, 65535));
      break;
    }
    case value_kind_t::s16: {
      const auto rounded = static_cast<std::int32_t>(std::lround(value));
      payload.data.s16 = static_cast<std::int16_t>(clamp_i32(rounded, -32768, 32767));
      break;
    }
    case value_kind_t::u32:
      if (value < 0.0F) {
        return status_t::value_out_of_range;
      }
      payload.data.u32 = static_cast<std::uint32_t>(std::lround(value));
      break;
    case value_kind_t::s32:
      payload.data.s32 = static_cast<std::int32_t>(std::lround(value));
      break;
    case value_kind_t::f32:
      payload.data.f32 = value;
      break;
    default:
      return status_t::type_mismatch;
  }

  return write_typed_value(*binding, &payload);
}

const char* to_string(const status_t status) {
  switch (status) {
    case status_t::ok:
      return "OK";
    case status_t::no_registry:
      return "NO_REGISTRY";
    case status_t::bad_argument:
      return "BAD_ARGUMENT";
    case status_t::id_not_found:
      return "ID_NOT_FOUND";
    case status_t::type_mismatch:
      return "TYPE_MISMATCH";
    case status_t::value_out_of_range:
      return "VALUE_OUT_OF_RANGE";
    case status_t::null_pointer:
      return "NULL_POINTER";
    default:
      return "UNKNOWN_STATUS";
  }
}

}  // namespace isim
