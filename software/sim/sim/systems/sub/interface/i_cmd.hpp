#pragma once

#include <cstdint>

#include "i_par.hpp"

namespace icmd {

ipar::parse_result_t ICMDPAR(const ipar::context_t& ctx, std::uint16_t id);
float ICMDF32(const ipar::context_t& ctx, std::uint16_t id, float fallback = 0.0F);
std::int32_t ICMDVAL(const ipar::context_t& ctx, std::uint16_t id, std::int32_t fallback = 0);
bool ICMDACT(const ipar::context_t& ctx, std::uint16_t id);
void ICMDCLEAR(const ipar::context_t& ctx, std::uint16_t id);

}  // namespace icmd
