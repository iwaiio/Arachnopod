#pragma once

#include <cstddef>
#include <cstdint>

#include "i_par.hpp"

namespace pss {

void init_msg_buffers();
void bind_ipar_context();

const ipar::context_t& ipar_context();

}  // namespace pss
