#pragma once

#include <cstddef>
#include <cstdint>

#include "i_cmd.hpp"
#include "i_sim.hpp"

namespace icmdexec {

enum class mode_t : std::uint8_t {
  bool_pair = 0,
  nonzero_value,
};

struct binding_t {
  mode_t mode{mode_t::nonzero_value};
  std::uint16_t cmd_id{0};
  std::uint16_t alt_cmd_id{0};
  std::uint16_t param_id{0};
};

using status_hook_t = void (*)(void* user,
                               const binding_t& binding,
                               isim::status_t status);

bool apply(const ipar::context_t& ctx,
           const binding_t& binding,
           status_hook_t status_hook = nullptr,
           void* user = nullptr);

std::size_t apply_all(const ipar::context_t& ctx,
                      const binding_t* bindings,
                      std::size_t count,
                      status_hook_t status_hook = nullptr,
                      void* user = nullptr);

}  // namespace icmdexec
