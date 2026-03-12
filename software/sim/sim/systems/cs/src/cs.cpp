#include "cs.hpp"

#include <array>

#include "common.hpp"

namespace cs {
namespace {

std::array<std::uint16_t, CS_PARAM_MSG_BLOCKS> MSG_PAR{};
std::array<std::uint16_t, CS_COMM_MSG_BLOCKS> MSG_COM{};
bool G_LOGGER_READY = false;

void ensure_logger_initialized() {
  if (G_LOGGER_READY) {
    return;
  }

  const common::log::init_config_t cfg{
      .min_level = common::log::level_t::info,
      .truncate_on_init = true,
  };
  G_LOGGER_READY = common::log::init_for_module("cs", cfg);
  if (G_LOGGER_READY) {
    common::log::info("logger initialized");
  }
}

ipar::context_t make_context() {
  ipar::context_t ctx{};
  ctx.role = ipar::role_t::cs;
  ctx.system_id = SYS_CS;
  ctx.msg_par = MSG_PAR.data();
  ctx.msg_par_blocks = MSG_PAR.size();
  ctx.msg_com = MSG_COM.data();
  ctx.msg_com_blocks = MSG_COM.size();
  return ctx;
}

ipar::context_t G_IPAR_CTX = make_context();

}  // namespace

void init_msg_buffers() {
  ensure_logger_initialized();
  MSG_PAR.fill(0U);
  MSG_COM.fill(0U);
  if (G_LOGGER_READY) {
    common::log::info("message buffers initialized");
  }
}

void bind_ipar_context() {
  ensure_logger_initialized();
  ipar::bind_context(&G_IPAR_CTX);
  if (G_LOGGER_READY) {
    common::log::info("IPAR context bound");
  }
}

const ipar::context_t& ipar_context() {
  return G_IPAR_CTX;
}

}  // namespace cs
