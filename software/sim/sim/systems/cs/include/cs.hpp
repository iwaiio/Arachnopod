#pragma once

#include <cstddef>
#include <cstdint>

#include "i_par.hpp"

namespace isysalgo {
struct bus_state_t;
}

namespace cs {

void init_msg_buffers();
void bind_ipar_context();

const ipar::context_t& ipar_context();
isysalgo::bus_state_t& bus_state();

}  // namespace cs
