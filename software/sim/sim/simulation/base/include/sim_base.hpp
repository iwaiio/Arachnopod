#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string_view>

namespace sim_base {

enum class physical_type_t : std::uint8_t {
  boolean = 0,
  int8,
  uint8,
  int16,
  uint16,
  float32,
  ufloat32,
};

struct param_entry_t {
  std::uint16_t id{0};
  const char* key{nullptr};
  std::uint8_t wire_bits{0};
  physical_type_t physical_type{physical_type_t::uint8};
  void* ptr{nullptr};
  double init_value{0.0};
};

inline std::recursive_mutex& model_data_mutex() {
  static std::recursive_mutex mtx;
  return mtx;
}

inline void zero_param(param_entry_t& entry) {
  if (entry.ptr == nullptr) {
    return;
  }

  switch (entry.physical_type) {
    case physical_type_t::boolean:
      *static_cast<std::uint8_t*>(entry.ptr) = 0U;
      break;
    case physical_type_t::int8:
      *static_cast<std::int8_t*>(entry.ptr) = 0;
      break;
    case physical_type_t::uint8:
      *static_cast<std::uint8_t*>(entry.ptr) = 0U;
      break;
    case physical_type_t::int16:
      *static_cast<std::int16_t*>(entry.ptr) = 0;
      break;
    case physical_type_t::uint16:
      *static_cast<std::uint16_t*>(entry.ptr) = 0U;
      break;
    case physical_type_t::float32:
    case physical_type_t::ufloat32:
      *static_cast<float*>(entry.ptr) = 0.0F;
      break;
    default:
      break;
  }
}

inline void init_param(param_entry_t& entry) {
  if (entry.ptr == nullptr) {
    return;
  }

  switch (entry.physical_type) {
    case physical_type_t::boolean:
      *static_cast<std::uint8_t*>(entry.ptr) = (entry.init_value != 0.0) ? 1U : 0U;
      break;
    case physical_type_t::int8:
      *static_cast<std::int8_t*>(entry.ptr) = static_cast<std::int8_t>(entry.init_value);
      break;
    case physical_type_t::uint8:
      *static_cast<std::uint8_t*>(entry.ptr) = static_cast<std::uint8_t>(entry.init_value);
      break;
    case physical_type_t::int16:
      *static_cast<std::int16_t*>(entry.ptr) = static_cast<std::int16_t>(entry.init_value);
      break;
    case physical_type_t::uint16:
      *static_cast<std::uint16_t*>(entry.ptr) = static_cast<std::uint16_t>(entry.init_value);
      break;
    case physical_type_t::float32:
    case physical_type_t::ufloat32:
      *static_cast<float*>(entry.ptr) = static_cast<float>(entry.init_value);
      break;
    default:
      break;
  }
}

template <std::size_t N>
inline void zero_params(std::array<param_entry_t, N>& params) {
  for (auto& entry : params) {
    zero_param(entry);
  }
}

template <std::size_t N>
inline void init_params(std::array<param_entry_t, N>& params) {
  for (auto& entry : params) {
    init_param(entry);
  }
}

template <std::size_t N>
inline param_entry_t* find_param_by_id(std::array<param_entry_t, N>& params, const std::uint16_t id) {
  for (auto& entry : params) {
    if (entry.id == id) {
      return &entry;
    }
  }
  return nullptr;
}

template <std::size_t N>
inline const param_entry_t* find_param_by_id(const std::array<param_entry_t, N>& params, const std::uint16_t id) {
  for (const auto& entry : params) {
    if (entry.id == id) {
      return &entry;
    }
  }
  return nullptr;
}

template <std::size_t N>
inline param_entry_t* find_param_by_key(std::array<param_entry_t, N>& params, const std::string_view key) {
  for (auto& entry : params) {
    if ((entry.key != nullptr) && (key == entry.key)) {
      return &entry;
    }
  }
  return nullptr;
}

template <std::size_t N>
inline const param_entry_t* find_param_by_key(const std::array<param_entry_t, N>& params,
                                              const std::string_view key) {
  for (const auto& entry : params) {
    if ((entry.key != nullptr) && (key == entry.key)) {
      return &entry;
    }
  }
  return nullptr;
}

}  // namespace sim_base
