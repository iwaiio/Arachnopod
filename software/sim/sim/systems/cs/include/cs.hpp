#pragma once

#include <cstddef>
#include <cstdint>

#include "i_par.hpp"

namespace isysalgo {
struct bus_state_t;
}

namespace cs {

// Clear wire data storage for the CS system software.
void init_msg_buffers();
// Bind thread-local IMSG context for the CS system software.
void bind_imsg_context();
void bind_ipar_context();

// Access the current CS IMSG context.
const ipar::context_t& imsg_context();
const ipar::context_t& ipar_context();
// Access the CS bus state used by the system software.
isysalgo::bus_state_t& bus_state();

}  // namespace cs
