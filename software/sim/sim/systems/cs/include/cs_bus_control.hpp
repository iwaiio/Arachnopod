#pragma once

#include <array>
#include <cstdint>

#include "arp_config.hpp"

namespace cs::bus_control {

enum class exchange_status_t : std::uint8_t {
  ok = 0,
  invalid_request,
  tx_error,
  timeout,
  rx_byte_lost,
  parity_error,
  bad_eof,
  bad_frame,
  unexpected_addr,
  unexpected_flag,
  unexpected_source,
};

struct exchange_request_t {
  std::uint8_t dst_addr{0};
  std::uint8_t flag{ARP_FLAG_UV};
  std::uint8_t marker{0};
  std::uint8_t tx_block_count{0};
  std::uint8_t expected_rsp_flag{0xFF};  // 0xFF -> auto by request.flag.
  std::array<std::uint16_t, ARP_BUS_MAX_DATA_BLOCKS> tx_blocks{};
};

struct exchange_response_t {
  exchange_status_t status{exchange_status_t::bad_frame};
  std::uint8_t src_addr{0};
  std::uint8_t flag{0};
  std::uint8_t marker{0};
  std::uint8_t rx_block_count{0};
  std::array<std::uint16_t, ARP_BUS_MAX_DATA_BLOCKS> rx_blocks{};
};

// One full CS transaction: TX request frame -> wait and parse RX response frame.
exchange_response_t exchange_once(const exchange_request_t& request,
                                  std::uint32_t writer_tick = 0,
                                  std::uint32_t timeout_frames = ARP_BUS_RESPONSE_TIMEOUT_FRAMES);

const char* to_string(exchange_status_t status);

}  // namespace cs::bus_control
