#pragma once

#include <cstdint>

namespace pss {

// Runtime loop API for PSS system thread.
void runtime_init();
void runtime_step(std::uint32_t tick);

}  // namespace pss
