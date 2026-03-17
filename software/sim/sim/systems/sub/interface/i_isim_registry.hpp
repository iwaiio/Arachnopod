#pragma once

#include <cstddef>
#include <cstdint>

#include "i_sim.hpp"
#include "sim_base.hpp"

namespace isimreg {

isim::value_kind_t ISIMKIND(const sim_base::param_entry_t& entry);

void ISIMREG_BUILD(const sim_base::param_entry_t* params,
                   std::size_t count,
                   isim::binding_t* bindings,
                   isim::registry_t& registry,
                   std::uint8_t system_id);

}  // namespace isimreg
