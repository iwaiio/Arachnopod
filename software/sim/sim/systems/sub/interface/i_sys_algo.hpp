#pragma once

#include <cstddef>
#include <cstdint>

#include "arp_config.hpp"
#include "i_sys.hpp"

namespace isysalgo {

enum class bus_role_t : std::uint8_t {
  device = 0,
  master,
};

enum class exchange_flag_t : std::uint8_t {
  rx = 0,
  tx = 1,
  bypass = 2,
};

enum class rx_flag_t : std::uint8_t {
  rx_sof = 0,
  rx_header,
  rx_data,
  rx_eof,
};

enum class tx_flag_t : std::uint8_t {
  tx_sof = 0,
  tx_header,
  tx_data,
  tx_eof,
};

enum class exchange_result_t : std::uint8_t {
  none = 0,
  success,
  timeout,
  invalid_eof,
};

struct bus_state_t {
  bus_role_t role{bus_role_t::device};
  std::uint8_t system_id{SYS_NONE};
  std::uint8_t self_addr{0};
  std::uint8_t target_addr{0};
  std::uint8_t* msg_header_u8{nullptr};
  std::uint16_t* msg_header_u16{nullptr};
  std::uint8_t* msg_cmd_buf_u8{nullptr};
  std::uint16_t* msg_cmd_buf_u16{nullptr};
  std::uint8_t* msg_prm_buf_u8{nullptr};
  std::uint16_t* msg_prm_buf_u16{nullptr};
  std::uint8_t* msg_cmd_u8{nullptr};
  std::uint16_t* msg_cmd_u16{nullptr};
  std::uint8_t* msg_prm_u8{nullptr};
  std::uint16_t* msg_prm_u16{nullptr};
  std::size_t msg_cmd_blocks{0};
  std::size_t msg_cmd_buf_blocks{0};
  std::size_t msg_prm_blocks{0};
  std::size_t msg_prm_buf_blocks{0};
  std::size_t msg_cmd_frame_offset_blocks{0};
  std::size_t msg_cmd_frame_blocks{0};
  std::size_t msg_prm_frame_offset_blocks{0};
  std::size_t msg_prm_frame_blocks{0};
  exchange_flag_t exchange_flag{exchange_flag_t::rx};
  rx_flag_t rx_flag{rx_flag_t::rx_sof};
  tx_flag_t tx_flag{tx_flag_t::tx_sof};
  bool msg_adr_flag{false};
  std::uint8_t msg_n_blocks{0};
  std::uint8_t msg_flag{ARP_FLAG_CMD_REQ};
  std::uint16_t exchange_sub_clock{0};
  std::uint16_t exchange_wait_clock{0};
  bool exchange_busy{false};
  bool result_ready{false};
  exchange_result_t last_result{exchange_result_t::none};
  std::uint32_t writer_tick{0};
};

struct bus_hooks_t {
  void (*parse_header)(void* user, bus_state_t& state) = nullptr;
  void (*gen_header)(void* user, bus_state_t& state) = nullptr;
  void (*bypass)(void* user, bus_state_t& state) = nullptr;
  void (*log_exchange)(void* user, const bus_state_t& state) = nullptr;
  void (*on_error)(void* user, bus_state_t& state) = nullptr;
};

using bus_exchange_fn = void (*)(bus_state_t& state,
                                 const bus_hooks_t& hooks,
                                 void* user);

struct sysalg_hooks_t {
  void (*ICMDUPD)(void* user) = nullptr;
  void (*LOCALSYSALG)(void* user) = nullptr;
  void (*ISYSNODES)(void* user) = nullptr;
  void (*IPRMUPD)(void* user) = nullptr;
};

struct base_hooks_t {
  sysalg_hooks_t ISYSALG{};
  bus_exchange_fn IBUSEXCHANGE = nullptr;
  void (*IEXCHANGECTRL)(void* user, bus_state_t& state) = nullptr;
  void (*ICMDPWRUPD)(void* user) = nullptr;
  void (*ILOG)(void* user) = nullptr;
};

void IBUS_BIND_STD(bus_state_t& state,
                   bus_role_t role,
                   std::uint8_t system_id,
                   std::uint8_t self_addr,
                   std::uint8_t target_addr,
                   std::uint8_t* msg_header_u8,
                   std::uint16_t* msg_header_u16,
                   std::uint8_t* msg_cmd_buf_u8,
                   std::uint16_t* msg_cmd_buf_u16,
                   std::size_t msg_cmd_buf_blocks,
                   std::uint8_t* msg_prm_buf_u8,
                   std::uint16_t* msg_prm_buf_u16,
                   std::size_t msg_prm_buf_blocks,
                   std::uint8_t* msg_cmd_u8,
                   std::uint16_t* msg_cmd_u16,
                   std::uint8_t* msg_prm_u8,
                   std::uint16_t* msg_prm_u16,
                   std::size_t msg_cmd_blocks,
                   std::size_t msg_prm_blocks);

void IBUS_PARSE_STD_HEADER(bus_state_t& state);
void IBUS_GEN_STD_HEADER(bus_state_t& state);
void IBUS_RESET_FRAME_LAYOUT(bus_state_t& state);
bool IBUS_SET_CMD_FRAME(bus_state_t& state, std::size_t offset_blocks, std::size_t block_count);
bool IBUS_SET_PRM_FRAME(bus_state_t& state, std::size_t offset_blocks, std::size_t block_count);
void IBUS_CLEAR_RESULT(bus_state_t& state);
bool IBUS_TAKE_RESULT(bus_state_t& state, exchange_result_t& out_result);
const char* IBUS_RESULT_TO_STRING(exchange_result_t result);
void ISYS_SYNC_MSGCMD_TO_BUF(bus_state_t& state);
void ISYS_SYNC_MSGPRM_TO_BUF(bus_state_t& state);

using model_export_override_fn = bool (*)(void* user, std::uint16_t id, float& out_value);
void ISYS_EXPORT_MODEL_PARAMS(const ipar::context_t& ctx,
                              std::uint16_t base_id,
                              std::uint16_t count,
                              model_export_override_fn override_fn = nullptr,
                              void* user = nullptr);

void ISYSALG(const sysalg_hooks_t& hooks, void* user = nullptr);
void ISYSBASEALG(isys::tick_state_t& tick,
                 bool power_on,
                 bus_state_t& bus,
                 const bus_hooks_t& bus_hooks,
                 const base_hooks_t& base_hooks,
                 void* user = nullptr);

void IBUS_EXCHANGE(bus_state_t& state,
                   const bus_hooks_t& hooks,
                   void* user = nullptr);
void IBUS_RX_EXCHANGE(bus_state_t& state,
                      const bus_hooks_t& hooks,
                      void* user = nullptr);
void IBUS_TX_EXCHANGE(bus_state_t& state,
                      const bus_hooks_t& hooks,
                      void* user = nullptr);
void IBUS_BYPASS(bus_state_t& state,
                 const bus_hooks_t& hooks,
                 void* user = nullptr);

void IRX_SOF(bus_state_t& state,
             const bus_hooks_t& hooks,
             void* user = nullptr);
void IRX_HEADER(bus_state_t& state,
                const bus_hooks_t& hooks,
                void* user = nullptr);
void IRX_DATA(bus_state_t& state,
              const bus_hooks_t& hooks,
              void* user = nullptr);
void IRX_EOF(bus_state_t& state,
             const bus_hooks_t& hooks,
             void* user = nullptr);

void ITX_SOF(bus_state_t& state,
             const bus_hooks_t& hooks,
             void* user = nullptr);
void ITX_HEADER(bus_state_t& state,
                const bus_hooks_t& hooks,
                void* user = nullptr);
void ITX_DATA(bus_state_t& state,
              const bus_hooks_t& hooks,
              void* user = nullptr);
void ITX_EOF(bus_state_t& state,
             const bus_hooks_t& hooks,
             void* user = nullptr);

}  // namespace isysalgo
