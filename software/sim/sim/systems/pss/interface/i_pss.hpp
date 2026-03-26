#pragma once

#include <cstdint>

namespace pss {

// System software loop API for the PSS thread.
void runtime_init();
void runtime_step(std::uint32_t tick);

}  // namespace pss
