#include "i_sim.hpp"

#include <algorithm>
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
    case value_kind_t::u8:
      *static_cast<std::uint8_t*>(binding.ptr) = in_value->data.u8;
      return status_t::ok;
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
      *static_cast<float*>(binding.ptr) = in_value->data.f32;
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

status_t ISIMPAR(const std::uint16_t id, value_t* out_value) {
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
