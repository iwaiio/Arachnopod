#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

#include "i_isim_registry.hpp"
#include "i_sys.hpp"
#include "i_sys_algo.hpp"

namespace isystools {

template <std::size_t CmdBufBlocks,
          std::size_t PrmBufBlocks,
          std::size_t CmdBlocks,
          std::size_t PrmBlocks>
struct wire_storage_t {
  isys::msg_storage_t<1> msg_header{};
  isys::msg_storage_t<CmdBufBlocks> msg_cmd_buf{};
  isys::msg_storage_t<PrmBufBlocks> msg_prm_buf{};
  std::array<std::uint16_t, PrmBlocks> msg_prm{};
  std::array<std::uint16_t, CmdBlocks> msg_cmd{};
};

struct software_state_t {
  bool initialized{false};
  isys::tick_state_t tick{};
  std::uint32_t current_tick{0U};
};

template <typename Storage>
ipar::context_t make_imsg_context(const ipar::role_t role, const std::uint8_t system_id, Storage& wire) {
  return isys::ISYSCTX(role, system_id, wire.msg_prm.data(), wire.msg_prm.size(), wire.msg_cmd.data(), wire.msg_cmd.size());
}

template <typename Storage>
void clear_wire_storage(Storage& wire) {
  isys::ISYSCLEAR(wire.msg_prm.data(), wire.msg_prm.size());
  isys::ISYSCLEAR(wire.msg_cmd.data(), wire.msg_cmd.size());
  isys::ISYSCLEAR(wire.msg_cmd_buf.u16.data(), wire.msg_cmd_buf.u16.size());
  isys::ISYSCLEAR(wire.msg_prm_buf.u16.data(), wire.msg_prm_buf.u16.size());
  wire.msg_header.u16[0] = 0U;
}

inline void reset_software_state(software_state_t& state, std::uint64_t& last_tick_log_count) {
  state = software_state_t{};
  state.initialized = true;
  last_tick_log_count = 0U;
}

inline bool should_log_tick(const isys::tick_state_t& tick,
                            std::uint64_t& last_tick_log_count,
                            const std::uint32_t tick_log_period) {
  const bool should_log = (tick.count <= 1U) || (tick.clock == 1U) || (tick.count >= (last_tick_log_count + tick_log_period));
  if (!should_log) {
    return false;
  }

  last_tick_log_count = tick.count;
  return true;
}

template <typename Storage>
void bind_standard_bus(isysalgo::bus_state_t& bus,
                       const isysalgo::bus_role_t role,
                       const std::uint8_t system_id,
                       const std::uint8_t self_addr,
                       const std::uint8_t target_addr,
                       Storage& wire) {
  isysalgo::IBUS_BIND_STD(bus,
                          role,
                          system_id,
                          self_addr,
                          target_addr,
                          wire.msg_header.u8.data(),
                          wire.msg_header.u16.data(),
                          wire.msg_cmd_buf.u8.data(),
                          wire.msg_cmd_buf.u16.data(),
                          wire.msg_cmd_buf.u16.size(),
                          wire.msg_prm_buf.u8.data(),
                          wire.msg_prm_buf.u16.data(),
                          wire.msg_prm_buf.u16.size(),
                          reinterpret_cast<std::uint8_t*>(wire.msg_cmd.data()),
                          wire.msg_cmd.data(),
                          reinterpret_cast<std::uint8_t*>(wire.msg_prm.data()),
                          wire.msg_prm.data(),
                          wire.msg_cmd.size(),
                          wire.msg_prm.size());
}

template <std::size_t BindingCount>
void build_isim_registry(const sim_base::param_entry_t* params,
                         const std::size_t count,
                         std::array<isim::binding_t, BindingCount>& bindings,
                         isim::registry_t& registry,
                         const std::uint8_t system_id) {
  isimreg::ISIMREG_BUILD(params, count, bindings.data(), registry, system_id);
}

inline std::string format_msg_words(const std::uint16_t* data, const std::size_t count) {
  if ((data == nullptr) || (count == 0U)) {
    return "[]";
  }

  std::ostringstream oss;
  oss << '[';
  for (std::size_t i = 0; i < count; ++i) {
    if (i != 0U) {
      oss << ' ';
    }
    oss << std::setw(2) << std::setfill('0') << i << ":0x" << std::hex << std::uppercase << std::setw(4)
        << std::setfill('0') << data[i] << std::dec;
  }
  oss << ']';
  return oss.str();
}

}  // namespace isystools
