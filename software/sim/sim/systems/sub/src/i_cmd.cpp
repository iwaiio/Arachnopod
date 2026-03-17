#include "i_cmd.hpp"

#include <cmath>

namespace icmd {

ipar::parse_result_t ICMDPAR(const ipar::context_t& ctx, const std::uint16_t id) {
  return ipar::IPAR(ctx, id);
}

std::int32_t ICMDVAL(const ipar::context_t& ctx, const std::uint16_t id, const std::int32_t fallback) {
  const auto parsed = ipar::IPAR(ctx, id);
  if (parsed.status != ipar::status_t::ok) {
    return fallback;
  }
  return static_cast<std::int32_t>(std::lround(parsed.value));
}

bool ICMDACT(const ipar::context_t& ctx, const std::uint16_t id) {
  return ICMDVAL(ctx, id, 0) != 0;
}

}  // namespace icmd
