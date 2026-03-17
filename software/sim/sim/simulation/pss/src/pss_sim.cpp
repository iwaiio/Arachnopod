#include "i_pss_sim.hpp"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <mutex>
#include <sstream>
#include <string_view>

#include "common.hpp"
#include "sim_pss_base.hpp"

namespace sim_pss_base {

decltype(P_PSSPWR) P_PSSPWR = 0;
decltype(P_PSSPWR12) P_PSSPWR12 = 0;
decltype(P_PSSPWR6) P_PSSPWR6 = 0;
decltype(P_PSSPWR5) P_PSSPWR5 = 0;
decltype(P_PSSPWR33) P_PSSPWR33 = 0;
decltype(P_PSSECO) P_PSSECO = 0;
decltype(P_PSSCLOCK) P_PSSCLOCK = 0;
decltype(P_PSSMAXT1) P_PSSMAXT1 = 0;
decltype(P_PSSMAXT2) P_PSSMAXT2 = 0;
decltype(P_PSSMAXA) P_PSSMAXA = 0;
decltype(P_PSSMAXW) P_PSSMAXW = 0;
decltype(P_PSSMINC) P_PSSMINC = 0;
decltype(P_PSSMINV) P_PSSMINV = 0;
decltype(P_PSSMAXA12) P_PSSMAXA12 = 0;
decltype(P_PSSMAXA6) P_PSSMAXA6 = 0;
decltype(P_PSSMAXA5) P_PSSMAXA5 = 0;
decltype(P_PSSMAXA33) P_PSSMAXA33 = 0;
decltype(P_PSST1) P_PSST1 = 0;
decltype(P_PSST2) P_PSST2 = 0;
decltype(P_PSSA) P_PSSA = 0;
decltype(P_PSSW) P_PSSW = 0;
decltype(P_PSSV) P_PSSV = 0;
decltype(P_PSSC) P_PSSC = 0;
decltype(P_PSSA12) P_PSSA12 = 0;
decltype(P_PSSV12) P_PSSV12 = 0;
decltype(P_PSSA6) P_PSSA6 = 0;
decltype(P_PSSV6) P_PSSV6 = 0;
decltype(P_PSSA5) P_PSSA5 = 0;
decltype(P_PSSV5) P_PSSV5 = 0;
decltype(P_PSSA33) P_PSSA33 = 0;
decltype(P_PSSV33) P_PSSV33 = 0;

}  // namespace sim_pss_base

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

    if (entry.bits <= 8U) {
      if (entry.type_r == sim_base::real_type_t::sfloat) {
        const auto raw = *static_cast<sim_base::float8_t*>(entry.ptr);
        const float value = sim_base::unpack_float8(raw);
        oss << static_cast<int>(raw) << '(' << std::fixed << std::setprecision(2) << value << ')';
        oss.unsetf(std::ios::floatfield);
      } else if (entry.type_r == sim_base::real_type_t::ufloat) {
        const auto raw = *static_cast<sim_base::ufloat8_t*>(entry.ptr);
        const float value = sim_base::unpack_ufloat8(raw);
        oss << static_cast<unsigned>(raw) << '(' << std::fixed << std::setprecision(2) << value << ')';
        oss.unsetf(std::ios::floatfield);
      } else if (entry.is_signed) {
        const auto raw = *static_cast<std::int8_t*>(entry.ptr);
        oss << static_cast<int>(raw);
      } else {
        const auto raw = *static_cast<std::uint8_t*>(entry.ptr);
        oss << static_cast<unsigned>(raw);
      }
    } else {
      if (entry.type_r == sim_base::real_type_t::sfloat) {
        const auto raw = *static_cast<sim_base::float16_t*>(entry.ptr);
        const float value = sim_base::unpack_float16(raw);
        oss << raw << '(' << std::fixed << std::setprecision(2) << value << ')';
        oss.unsetf(std::ios::floatfield);
      } else if (entry.type_r == sim_base::real_type_t::ufloat) {
        const auto raw = *static_cast<sim_base::ufloat16_t*>(entry.ptr);
        const float value = sim_base::unpack_ufloat16(raw);
        oss << raw << '(' << std::fixed << std::setprecision(2) << value << ')';
        oss.unsetf(std::ios::floatfield);
      } else if (entry.is_signed) {
        const auto raw = *static_cast<std::int16_t*>(entry.ptr);
        oss << raw;
      } else {
        const auto raw = *static_cast<std::uint16_t*>(entry.ptr);
        oss << raw;
      }
    }
  }

  common::log::info(oss.str());
}

template <typename T>
int as_int(const T value) {
  return static_cast<int>(value);
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

int move_towards(const int current, const int target, const int delta) {
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

  sim_pss_base::P_PSSMAXT1 = clamp_i16(as_int(sim_pss_base::P_PSSMAXT1));
  sim_pss_base::P_PSSMAXT2 = clamp_i16(as_int(sim_pss_base::P_PSSMAXT2));
  sim_pss_base::P_PSSMAXA = clamp_i8(std::max(0, as_int(sim_pss_base::P_PSSMAXA)));
  sim_pss_base::P_PSSMAXW = clamp_i8(std::max(0, as_int(sim_pss_base::P_PSSMAXW)));
  sim_pss_base::P_PSSMINC = clamp_i8(std::max(0, as_int(sim_pss_base::P_PSSMINC)));
  sim_pss_base::P_PSSMINV = clamp_i8(std::max(0, as_int(sim_pss_base::P_PSSMINV)));
  sim_pss_base::P_PSSMAXA12 = clamp_i8(std::max(0, as_int(sim_pss_base::P_PSSMAXA12)));
  sim_pss_base::P_PSSMAXA6 = clamp_i8(std::max(0, as_int(sim_pss_base::P_PSSMAXA6)));
  sim_pss_base::P_PSSMAXA5 = clamp_i8(std::max(0, as_int(sim_pss_base::P_PSSMAXA5)));
  sim_pss_base::P_PSSMAXA33 = clamp_i8(std::max(0, as_int(sim_pss_base::P_PSSMAXA33)));

  if (sim_pss_base::P_PSSPWR == 0U) {
    sim_pss_base::P_PSSPWR12 = 0U;
    sim_pss_base::P_PSSPWR6 = 0U;
    sim_pss_base::P_PSSPWR5 = 0U;
    sim_pss_base::P_PSSPWR33 = 0U;
  }
}

void set_defaults() {
  sim_base::zero_params(sim_pss_base::PARAMS);
}

void simulate_physics() {
  apply_control_limits();

  if (sim_pss_base::P_PSSPWR == 0U) {
    sim_pss_base::P_PSSA12 = 0;
    sim_pss_base::P_PSSV12 = 0;
    sim_pss_base::P_PSSA6 = 0;
    sim_pss_base::P_PSSV6 = 0;
    sim_pss_base::P_PSSA5 = 0;
    sim_pss_base::P_PSSV5 = 0;
    sim_pss_base::P_PSSA33 = 0;
    sim_pss_base::P_PSSV33 = 0;
    sim_pss_base::P_PSSA = 0;
    sim_pss_base::P_PSSW = 0;
    sim_pss_base::P_PSSV = 0;

    sim_pss_base::P_PSST1 = clamp_i16(move_towards(as_int(sim_pss_base::P_PSST1), 0, 1));
    sim_pss_base::P_PSST2 = clamp_i16(move_towards(as_int(sim_pss_base::P_PSST2), 0, 1));
    return;
  }

  int i12 = (sim_pss_base::P_PSSPWR12 != 0U) ? as_int(sim_pss_base::P_PSSMAXA12) : 0;
  int i6 = (sim_pss_base::P_PSSPWR6 != 0U) ? as_int(sim_pss_base::P_PSSMAXA6) : 0;
  int i5 = (sim_pss_base::P_PSSPWR5 != 0U) ? as_int(sim_pss_base::P_PSSMAXA5) : 0;
  int i33 = (sim_pss_base::P_PSSPWR33 != 0U) ? as_int(sim_pss_base::P_PSSMAXA33) : 0;

  if (sim_pss_base::P_PSSECO != 0U) {
    i12 /= 2;
    i6 /= 2;
    i5 /= 2;
    i33 /= 2;
  }

  int total_i = i12 + i6 + i5 + i33;
  const int total_i_limit = std::max(0, as_int(sim_pss_base::P_PSSMAXA));
  if ((total_i_limit > 0) && (total_i > total_i_limit) && (total_i > 0)) {
    i12 = (i12 * total_i_limit) / total_i;
    i6 = (i6 * total_i_limit) / total_i;
    i5 = (i5 * total_i_limit) / total_i;
    i33 = (i33 * total_i_limit) / total_i;
    total_i = i12 + i6 + i5 + i33;
  }

  sim_pss_base::P_PSSA12 = clamp_i8(i12);
  sim_pss_base::P_PSSA6 = clamp_i8(i6);
  sim_pss_base::P_PSSA5 = clamp_i8(i5);
  sim_pss_base::P_PSSA33 = clamp_i8(i33);
  sim_pss_base::P_PSSA = clamp_i8(total_i);

  const int rail_v = std::max(0, as_int(sim_pss_base::P_PSSMINV));
  sim_pss_base::P_PSSV12 = clamp_i8((sim_pss_base::P_PSSPWR12 != 0U) ? rail_v : 0);
  sim_pss_base::P_PSSV6 = clamp_i8((sim_pss_base::P_PSSPWR6 != 0U) ? rail_v : 0);
  sim_pss_base::P_PSSV5 = clamp_i8((sim_pss_base::P_PSSPWR5 != 0U) ? rail_v : 0);
  sim_pss_base::P_PSSV33 = clamp_i8((sim_pss_base::P_PSSPWR33 != 0U) ? rail_v : 0);
  sim_pss_base::P_PSSV = clamp_i8(rail_v);

  int total_w = total_i;
  total_w = std::min(total_w, std::max(0, as_int(sim_pss_base::P_PSSMAXW)));
  sim_pss_base::P_PSSW = clamp_i8(total_w);

  int charge = as_int(sim_pss_base::P_PSSC);
  if (charge > 0) {
    charge -= 1;
  }
  charge = std::max(charge, std::max(0, as_int(sim_pss_base::P_PSSMINC)));
  sim_pss_base::P_PSSC = clamp_i8(charge);

  sim_pss_base::P_PSST1 = clamp_i16(move_towards(as_int(sim_pss_base::P_PSST1), as_int(sim_pss_base::P_PSSMAXT1), 1));
  sim_pss_base::P_PSST2 = clamp_i16(move_towards(as_int(sim_pss_base::P_PSST2), as_int(sim_pss_base::P_PSSMAXT2), 1));
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
