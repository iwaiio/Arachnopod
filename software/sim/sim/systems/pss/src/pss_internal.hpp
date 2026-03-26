#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "../../base/sys_id_range.hpp"
#include "i_par.hpp"
#include "i_sim.hpp"
#include "i_sys.hpp"
#include "i_sys_algo.hpp"
#include "system_software_tools.hpp"

namespace pss::internal {

using wire_storage_t =
    isystools::wire_storage_t<PSS_COMM_MSG_BLOCKS, PSS_PARAM_MSG_BLOCKS, PSS_COMM_MSG_BLOCKS, PSS_PARAM_MSG_BLOCKS>;
using software_state_t = isystools::software_state_t;

extern bool G_LOGGER_READY;
extern wire_storage_t G_WIRE;
extern ipar::context_t G_IMSG_CTX;
extern software_state_t G_SOFTWARE;
extern isysalgo::bus_state_t G_BUS;
extern bool G_BUS_READY;
extern std::uint64_t G_LAST_TICK_LOG_COUNT;
extern std::array<isim::binding_t, static_cast<std::size_t>(PSS_COUNT)> G_ISIM_BINDINGS;
extern isim::registry_t G_ISIM_REGISTRY;

void init_bus_state();
void init_isim_registry();
void ensure_logger_initialized();
void log_msg_snapshot(std::string_view stage);
bool is_power_on();
void IPRMUPD(void* user);
void ICMDUPD(void* user);
void LOCALSYSALG(void* user);
void ISYSNODES(void* user);
void ICMDPWRUPD(void* user);
void ILOG(void* user);

extern const isysalgo::bus_hooks_t k_bus_hooks;
extern const isysalgo::base_hooks_t k_base_hooks;

}  // namespace pss::internal
