#pragma once

#include <cstdint>

namespace cs {

// Runtime loop API for CS system thread.
void runtime_init();
void runtime_step(std::uint32_t tick);

}  // namespace cs
