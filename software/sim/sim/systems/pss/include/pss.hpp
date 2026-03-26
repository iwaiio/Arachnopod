#pragma once

#include <cstddef>
#include <cstdint>

#include "i_par.hpp"

namespace pss {

// Clear wire data storage for the PSS system software.
void init_msg_buffers();
// Bind thread-local IMSG context for the PSS system software.
void bind_imsg_context();
void bind_ipar_context();

// Access the current PSS IMSG context.
const ipar::context_t& imsg_context();
const ipar::context_t& ipar_context();

}  // namespace pss
