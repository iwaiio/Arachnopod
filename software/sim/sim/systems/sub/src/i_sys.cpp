#include "i_sys.hpp"

namespace isys {

void ISYSRESET(tick_state_t& state) {
  state = tick_state_t{};
}

void ISYSTEP(tick_state_t& state) {
  ++state.count;
  state.clock = (state.clock == 0xFFU) ? 1U : static_cast<std::uint8_t>(state.clock + 1U);
}

void ISYSCLEAR(std::uint16_t* buf, const std::size_t count) {
  if (buf == nullptr) {
    return;
  }
  for (std::size_t i = 0; i < count; ++i) {
    buf[i] = 0U;
  }
}

ipar::context_t ISYSCTX(const ipar::role_t role,
                        const std::uint8_t system_id,
                        std::uint16_t* msg_par,
                        const std::size_t msg_par_blocks,
                        std::uint16_t* msg_com,
                        const std::size_t msg_com_blocks) {
  ipar::context_t ctx{};
  ctx.role = role;
  ctx.system_id = system_id;
  ctx.msg_par = msg_par;
  ctx.msg_par_blocks = msg_par_blocks;
  ctx.msg_com = msg_com;
  ctx.msg_com_blocks = msg_com_blocks;
  return ctx;
}

}  // namespace isys
