#include "i_isim_registry.hpp"

namespace isimreg {

isim::value_kind_t ISIMKIND(const sim_base::param_entry_t& entry) {
  if (entry.bits >= 16U) {
    return entry.is_signed ? isim::value_kind_t::s16 : isim::value_kind_t::u16;
  }
  return entry.is_signed ? isim::value_kind_t::s8 : isim::value_kind_t::u8;
}

void ISIMREG_BUILD(const sim_base::param_entry_t* params,
                   const std::size_t count,
                   isim::binding_t* bindings,
                   isim::registry_t& registry,
                   const std::uint8_t system_id) {
  if (params == nullptr || bindings == nullptr || count == 0U) {
    registry = isim::registry_t{};
    return;
  }

  for (std::size_t i = 0; i < count; ++i) {
    bindings[i] = isim::binding_t{
        .id = params[i].id,
        .kind = ISIMKIND(params[i]),
        .ptr = params[i].ptr,
    };
  }

  registry = isim::registry_t{
      .bindings = bindings,
      .count = count,
      .system_id = system_id,
  };
  isim::bind_registry(&registry);
}

}  // namespace isimreg
