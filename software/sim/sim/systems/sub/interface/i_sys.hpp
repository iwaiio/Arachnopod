#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "i_par.hpp"

namespace isys {

template <std::size_t Blocks>
union msg_storage_t {
  std::array<std::uint8_t, Blocks * 2U> u8;
  std::array<std::uint16_t, Blocks> u16;
};

struct tick_state_t {
  std::uint64_t count{0U};
  std::uint8_t clock{0U};
};

void ISYSRESET(tick_state_t& state);
void ISYSTEP(tick_state_t& state);

void ISYSCLEAR(std::uint16_t* buf, std::size_t count);

ipar::context_t ISYSCTX(ipar::role_t role,
                        std::uint8_t system_id,
                        std::uint16_t* msg_prm,
                        std::size_t msg_prm_blocks,
                        std::uint16_t* msg_cmd,
                        std::size_t msg_cmd_blocks);

}  // namespace isys
