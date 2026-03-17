#pragma once

#include <cstdint>

#include "i_par.hpp"

namespace icmd {

ipar::parse_result_t ICMDPAR(const ipar::context_t& ctx, std::uint16_t id);
std::int32_t ICMDVAL(const ipar::context_t& ctx, std::uint16_t id, std::int32_t fallback = 0);
bool ICMDACT(const ipar::context_t& ctx, std::uint16_t id);

}  // namespace icmd
