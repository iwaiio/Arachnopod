#pragma once

#include <string_view>

#include <cstdint>

#include "i_par.hpp"
#include "i_sim.hpp"
#include "i_sys.hpp"
#include "i_sys_algo.hpp"
#include "system_software_tools.hpp"
#include "cs_scheduler_internal.hpp"

namespace cs::internal {

using wire_storage_t =
    isystools::wire_storage_t<CS_COMM_BUF_BLOCKS, CS_PARAM_BUF_BLOCKS, CS_COMM_MSG_BLOCKS, CS_PARAM_MSG_BLOCKS>;
using software_state_t = isystools::software_state_t;

extern wire_storage_t G_WIRE;
extern software_state_t G_SOFTWARE;
extern ipar::context_t G_IMSG_CTX;
extern isysalgo::bus_state_t G_BUS;
extern bool G_BUS_READY;
extern bool G_LOGGER_READY;
extern std::uint64_t G_LAST_TICK_LOG_COUNT;

void ensure_logger_initialized();
void init_bus_state();
void init_isim_registry();
void log_msg_snapshot(std::string_view stage);
bool is_power_on();
void ILOG(void* user);
void ICSBASEALG(isys::tick_state_t& tick,
                isysalgo::bus_state_t& bus,
                const isysalgo::bus_hooks_t& bus_hooks,
                void* user);
void ICSALG(void* user, isysalgo::bus_state_t& bus);
void ICSCONTROL(void* user);
void ICSHIALG(void* user);
void LOCALSYSALG(void* user);
void ICSNODES(void* user);
void ICSSYSOPERATOR(void* user);
void ICSMODULEOPERATOR(void* user);
void ICSSCHEDULER(void* user, isysalgo::bus_state_t& bus);
void IBUSEXCHANGE(isysalgo::bus_state_t& bus,
                  const isysalgo::bus_hooks_t& hooks,
                  void* user);

extern const isysalgo::bus_hooks_t k_bus_hooks;

}  // namespace cs::internal
