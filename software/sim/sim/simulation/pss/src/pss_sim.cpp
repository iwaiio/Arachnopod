#include "i_pss_sim.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <mutex>
#include <sstream>
#include <string_view>

#include "common.hpp"
#include "sim_pss_base.hpp"

namespace pss_sim {
namespace {

void log_state_snapshot(const std::string_view tag) {
  if (!common::log::is_initialized()) {
    return;
  }

  std::ostringstream oss;
  oss << "state[" << tag << "]: ";

  for (std::size_t i = 0; i < sim_pss_base::k_param_count; ++i) {
    const auto& entry = sim_pss_base::PARAMS[i];
    if (i != 0U) {
      oss << ' ';
    }

    oss << entry.key << '=';

    switch (entry.physical_type) {
      case sim_base::physical_type_t::boolean:
        oss << ((*static_cast<const std::uint8_t*>(entry.ptr) != 0U) ? 1 : 0);
        break;
      case sim_base::physical_type_t::int8:
        oss << static_cast<int>(*static_cast<const std::int8_t*>(entry.ptr));
        break;
      case sim_base::physical_type_t::uint8:
        oss << static_cast<unsigned>(*static_cast<const std::uint8_t*>(entry.ptr));
        break;
      case sim_base::physical_type_t::int16:
        oss << *static_cast<const std::int16_t*>(entry.ptr);
        break;
      case sim_base::physical_type_t::uint16:
        oss << *static_cast<const std::uint16_t*>(entry.ptr);
        break;
      case sim_base::physical_type_t::float32:
      case sim_base::physical_type_t::ufloat32:
        oss << std::fixed << std::setprecision(2) << *static_cast<const float*>(entry.ptr);
        oss.unsetf(std::ios::floatfield);
        break;
      default:
        oss << '?';
        break;
    }
  }

  common::log::info(oss.str());
}

template <typename T>
int as_int(const T value) {
  return static_cast<int>(value);
}

template <typename T>
float as_float(const T value) {
  return static_cast<float>(value);
}

std::uint8_t clamp_u1(const int value) {
  return static_cast<std::uint8_t>((value != 0) ? 1 : 0);
}

std::int8_t clamp_i8(const int value) {
  const int lo = static_cast<int>(std::numeric_limits<std::int8_t>::lowest());
  const int hi = static_cast<int>(std::numeric_limits<std::int8_t>::max());
  return static_cast<std::int8_t>(std::clamp(value, lo, hi));
}

std::int16_t clamp_i16(const int value) {
  const int lo = static_cast<int>(std::numeric_limits<std::int16_t>::lowest());
  const int hi = static_cast<int>(std::numeric_limits<std::int16_t>::max());
  return static_cast<std::int16_t>(std::clamp(value, lo, hi));
}

float clamp_non_negative(const float value) {
  return std::max(0.0F, value);
}

int move_towards(const int current, const int target, const int delta) {
  if (current < target) {
    return std::min(current + delta, target);
  }
  if (current > target) {
    return std::max(current - delta, target);
  }
  return current;
}

float move_towards_f32(const float current, const float target, const float delta) {
  if (current < target) {
    return std::min(current + delta, target);
  }
  if (current > target) {
    return std::max(current - delta, target);
  }
  return current;
}

void apply_control_limits() {
  sim_pss_base::P_PSSPWR = clamp_u1(as_int(sim_pss_base::P_PSSPWR));
  sim_pss_base::P_PSSPWR12 = clamp_u1(as_int(sim_pss_base::P_PSSPWR12));
  sim_pss_base::P_PSSPWR6 = clamp_u1(as_int(sim_pss_base::P_PSSPWR6));
  sim_pss_base::P_PSSPWR5 = clamp_u1(as_int(sim_pss_base::P_PSSPWR5));
  sim_pss_base::P_PSSPWR33 = clamp_u1(as_int(sim_pss_base::P_PSSPWR33));
  sim_pss_base::P_PSSECO = clamp_u1(as_int(sim_pss_base::P_PSSECO));

  sim_pss_base::P_PSSMAXA = clamp_non_negative(sim_pss_base::P_PSSMAXA);
  sim_pss_base::P_PSSMAXW = clamp_non_negative(sim_pss_base::P_PSSMAXW);
  sim_pss_base::P_PSSMINC = clamp_i8(std::max(0, as_int(sim_pss_base::P_PSSMINC)));
  sim_pss_base::P_PSSMINV = clamp_non_negative(sim_pss_base::P_PSSMINV);
  sim_pss_base::P_PSSMAXA12 = clamp_non_negative(sim_pss_base::P_PSSMAXA12);
  sim_pss_base::P_PSSMAXA6 = clamp_non_negative(sim_pss_base::P_PSSMAXA6);
  sim_pss_base::P_PSSMAXA5 = clamp_non_negative(sim_pss_base::P_PSSMAXA5);
  sim_pss_base::P_PSSMAXA33 = clamp_non_negative(sim_pss_base::P_PSSMAXA33);

  if (sim_pss_base::P_PSSPWR == 0U) { /// !!! удалить эту проверку, логика не должна так работать !!!
    sim_pss_base::P_PSSPWR12 = 0U;
    sim_pss_base::P_PSSPWR6 = 0U;
    sim_pss_base::P_PSSPWR5 = 0U;
    sim_pss_base::P_PSSPWR33 = 0U;
  }
}

void set_defaults() {
  sim_pss_base::init_defaults();
}

void simulate_physics() {
  apply_control_limits();

  if (sim_pss_base::P_PSSPWR == 0U) {
    sim_pss_base::P_PSSA12 = 0.0F;
    sim_pss_base::P_PSSV12 = 0.0F;
    sim_pss_base::P_PSSA6 = 0.0F;
    sim_pss_base::P_PSSV6 = 0.0F;
    sim_pss_base::P_PSSA5 = 0.0F;
    sim_pss_base::P_PSSV5 = 0.0F;
    sim_pss_base::P_PSSA33 = 0.0F;
    sim_pss_base::P_PSSV33 = 0.0F;
    sim_pss_base::P_PSSA = 0.0F;
    sim_pss_base::P_PSSW = 0.0F;
    sim_pss_base::P_PSSV = 0.0F;

    sim_pss_base::P_PSST1 = move_towards_f32(sim_pss_base::P_PSST1, 0.0F, 1.0F);
    sim_pss_base::P_PSST2 = move_towards_f32(sim_pss_base::P_PSST2, 0.0F, 1.0F);
    return;
  }

  float i12 = (sim_pss_base::P_PSSPWR12 != 0U) ? sim_pss_base::P_PSSMAXA12 : 0.0F;
  float i6 = (sim_pss_base::P_PSSPWR6 != 0U) ? sim_pss_base::P_PSSMAXA6 : 0.0F;
  float i5 = (sim_pss_base::P_PSSPWR5 != 0U) ? sim_pss_base::P_PSSMAXA5 : 0.0F;
  float i33 = (sim_pss_base::P_PSSPWR33 != 0U) ? sim_pss_base::P_PSSMAXA33 : 0.0F;

  if (sim_pss_base::P_PSSECO != 0U) {
    i12 *= 0.5F;
    i6 *= 0.5F;
    i5 *= 0.5F;
    i33 *= 0.5F;
  }

  float total_i = i12 + i6 + i5 + i33;
  const float total_i_limit = sim_pss_base::P_PSSMAXA;
  if ((total_i_limit > 0.0F) && (total_i > total_i_limit) && (total_i > 0.0F)) {
    const float scale = total_i_limit / total_i;
    i12 *= scale;
    i6 *= scale;
    i5 *= scale;
    i33 *= scale;
    total_i = i12 + i6 + i5 + i33;
  }

  sim_pss_base::P_PSSA12 = i12;
  sim_pss_base::P_PSSA6 = i6;
  sim_pss_base::P_PSSA5 = i5;
  sim_pss_base::P_PSSA33 = i33;
  sim_pss_base::P_PSSA = total_i;

  const float rail_v = sim_pss_base::P_PSSMINV;
  sim_pss_base::P_PSSV12 = (sim_pss_base::P_PSSPWR12 != 0U) ? rail_v : 0.0F;
  sim_pss_base::P_PSSV6 = (sim_pss_base::P_PSSPWR6 != 0U) ? rail_v : 0.0F;
  sim_pss_base::P_PSSV5 = (sim_pss_base::P_PSSPWR5 != 0U) ? rail_v : 0.0F;
  sim_pss_base::P_PSSV33 = (sim_pss_base::P_PSSPWR33 != 0U) ? rail_v : 0.0F;
  sim_pss_base::P_PSSV = rail_v;

  float total_w = total_i;
  total_w = std::min(total_w, sim_pss_base::P_PSSMAXW);
  sim_pss_base::P_PSSW = total_w;

  int charge = as_int(sim_pss_base::P_PSSC);
  if (charge > 0) {
    charge -= 1;
  }
  charge = std::max(charge, std::max(0, as_int(sim_pss_base::P_PSSMINC)));
  sim_pss_base::P_PSSC = clamp_i8(charge);

  sim_pss_base::P_PSST1 = move_towards_f32(sim_pss_base::P_PSST1, sim_pss_base::P_PSSMAXT1, 1.0F);
  sim_pss_base::P_PSST2 = move_towards_f32(sim_pss_base::P_PSST2, sim_pss_base::P_PSSMAXT2, 1.0F);
}

}  // namespace

void init() {
  std::lock_guard<std::recursive_mutex> lock(sim_base::model_data_mutex());
  set_defaults();
  log_state_snapshot("init");
}

void step(const std::uint32_t) {
  std::lock_guard<std::recursive_mutex> lock(sim_base::model_data_mutex());
  simulate_physics();
  log_state_snapshot("tick");
}

}  // namespace pss_sim
