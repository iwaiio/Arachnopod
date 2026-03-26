#include "i_isim_registry.hpp"

namespace isimreg {

isim::value_kind_t ISIMKIND(const sim_base::param_entry_t& entry) {
  switch (entry.physical_type) {
    case sim_base::physical_type_t::boolean:
    case sim_base::physical_type_t::uint8:
      return isim::value_kind_t::u8;
    case sim_base::physical_type_t::int8:
      return isim::value_kind_t::s8;
    case sim_base::physical_type_t::uint16:
      return isim::value_kind_t::u16;
    case sim_base::physical_type_t::int16:
      return isim::value_kind_t::s16;
    case sim_base::physical_type_t::float32:
    case sim_base::physical_type_t::ufloat32:
      return isim::value_kind_t::f32;
    default:
      return isim::value_kind_t::u8;
  }
}

isim::value_domain_t ISIMDOMAIN(const sim_base::param_entry_t& entry) {
  switch (entry.physical_type) {
    case sim_base::physical_type_t::boolean:
      return isim::value_domain_t::binary;
    case sim_base::physical_type_t::uint8:
    case sim_base::physical_type_t::uint16:
    case sim_base::physical_type_t::ufloat32:
      return isim::value_domain_t::non_negative;
    default:
      return isim::value_domain_t::plain;
  }
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
        .domain = ISIMDOMAIN(params[i]),
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
