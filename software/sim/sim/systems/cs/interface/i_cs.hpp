#pragma once

#include <cstdint>

namespace cs {

// System software loop API for the CS thread.
void runtime_init();
void runtime_step(std::uint32_t tick);

}  // namespace cs
