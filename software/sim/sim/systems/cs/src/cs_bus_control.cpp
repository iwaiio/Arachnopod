#include "cs_bus_control.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

#include "common.hpp"
#include "i_mock_bus.hpp"

namespace cs::bus_control {
namespace {

constexpr std::uint8_t k_cs_addr = ARP_ADDR_CS;
constexpr std::uint8_t k_auto_rsp_flag = 0xFF;

const char* flag_to_string(const std::uint8_t flag) {
  switch (flag) {
    case ARP_FLAG_UV:
      return "UV";
    case ARP_FLAG_UV_RESP:
      return "UV_RESP";
    case ARP_FLAG_REQ_DATA:
      return "REQ_DATA";
    case ARP_FLAG_REQ_DATA_RESP:
      return "REQ_DATA_RESP";
    default:
      return "UNKNOWN";
  }
}

std::string format_blocks(const std::array<std::uint16_t, ARP_BUS_MAX_DATA_BLOCKS>& blocks,
                          const std::uint8_t count) {
  std::ostringstream oss;
  oss << '[';
  const std::size_t n = std::min<std::size_t>(count, 8U);
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0U) {
      oss << ' ';
    }
    oss << "0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << blocks[i] << std::dec;
  }
  if (count > n) {
    oss << " ...";
  }
  oss << ']';
  return oss.str();
}

void log_tx_request(const exchange_request_t& request) {
  if (!common::log::is_initialized()) {
    return;
  }

  std::ostringstream oss;
  oss << "bus tx: dst=" << static_cast<unsigned>(request.dst_addr) << " flag=" << flag_to_string(request.flag)
      << " marker=" << static_cast<unsigned>(request.marker)
      << " blocks=" << static_cast<unsigned>(request.tx_block_count)
      << " data=" << format_blocks(request.tx_blocks, request.tx_block_count);
  common::log::info(oss.str());
}

void log_rx_response(const exchange_response_t& response) {
  if (!common::log::is_initialized()) {
    return;
  }

  std::ostringstream oss;
  oss << "bus rx: src=" << static_cast<unsigned>(response.src_addr) << " flag=" << flag_to_string(response.flag)
      << " marker=" << static_cast<unsigned>(response.marker)
      << " blocks=" << static_cast<unsigned>(response.rx_block_count)
      << " data=" << format_blocks(response.rx_blocks, response.rx_block_count);
  common::log::info(oss.str());
}

void log_exchange_warning(const std::string_view prefix, const std::string_view details) {
  if (!common::log::is_initialized()) {
    return;
  }

  std::string msg(prefix);
  msg += details;
  common::log::warning(msg);
}

std::uint8_t auto_rsp_flag(const std::uint8_t req_flag) {
  switch (req_flag) {
    case ARP_FLAG_UV:
      return ARP_FLAG_UV_RESP;
    case ARP_FLAG_REQ_DATA:
      return ARP_FLAG_REQ_DATA_RESP;
    default:
      return req_flag;
  }
}

std::uint32_t frames_to_timeout_ms(const std::uint32_t frames) {
  constexpr std::uint64_t k_default_hz = 1000ULL;
  constexpr std::uint64_t k_us_per_sec = 1'000'000ULL;

  const std::uint64_t hz = (ARP_TICK_HZ == 0U) ? k_default_hz : static_cast<std::uint64_t>(ARP_TICK_HZ);
  const std::uint64_t speed =
      (ARP_SIM_SPEED_PERCENT == 0U) ? 100ULL : static_cast<std::uint64_t>(ARP_SIM_SPEED_PERCENT);
  const std::uint64_t denom = hz * speed;
  if (denom == 0ULL) {
    return 1U;
  }

  // 1 logical frame/tick duration in microseconds with speed scaling:
  // tick_us = (1e6 / hz) * (100 / speed).
  const std::uint64_t tick_us = ((k_us_per_sec * 100ULL) + denom - 1ULL) / denom;
  const std::uint64_t timeout_us = static_cast<std::uint64_t>(std::max<std::uint32_t>(frames, 1U)) * tick_us;
  const std::uint64_t timeout_ms = (timeout_us + 999ULL) / 1000ULL;
  return static_cast<std::uint32_t>(std::max<std::uint64_t>(timeout_ms, 1ULL));
}

mock_bus_status_t tx_frame_blocking(const std::uint32_t writer_tick,
                                    const mock_bus_frame_t& tx_frame,
                                    const std::uint32_t timeout_ms) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  while (std::chrono::steady_clock::now() <= deadline) {
    const auto status = mock_bus_broadcast_frame(k_cs_addr, writer_tick, tx_frame);
    if (status == mock_bus_status_t::ok) {
      return status;
    }

    if (status != mock_bus_status_t::timeout) {
      return status;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  return mock_bus_status_t::timeout;
}

mock_bus_status_t rx_frame_blocking(const std::uint32_t timeout_ms, mock_bus_frame_t* out_frame) {
  constexpr std::uint32_t k_poll_timeout_ms = 1U;

  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  while (std::chrono::steady_clock::now() <= deadline) {
    const auto status = mock_bus_listen_frame(k_cs_addr, k_poll_timeout_ms, out_frame);
    if (status == mock_bus_status_t::ok) {
      return status;
    }

    if (status != mock_bus_status_t::timeout) {
      return status;
    }
  }

  return mock_bus_status_t::timeout;
}

exchange_status_t map_rx_status(const mock_bus_status_t status) {
  switch (status) {
    case mock_bus_status_t::timeout:
      return exchange_status_t::timeout;
    case mock_bus_status_t::rx_byte_lost:
      return exchange_status_t::rx_byte_lost;
    case mock_bus_status_t::parity_error:
      return exchange_status_t::parity_error;
    case mock_bus_status_t::bad_eof:
      return exchange_status_t::bad_eof;
    case mock_bus_status_t::bad_frame:
    case mock_bus_status_t::invalid_argument:
      return exchange_status_t::bad_frame;
    case mock_bus_status_t::ok:
    default:
      return exchange_status_t::bad_frame;
  }
}

}  // namespace

exchange_response_t exchange_once(const exchange_request_t& request,
                                  const std::uint32_t writer_tick,
                                  const std::uint32_t timeout_frames) {
  exchange_response_t rsp{};

  if ((request.dst_addr == 0u) || (request.tx_block_count > ARP_BUS_MAX_DATA_BLOCKS)) {
    rsp.status = exchange_status_t::invalid_request;
    return rsp;
  }

  const auto expected_rsp_flag =
      (request.expected_rsp_flag == k_auto_rsp_flag) ? auto_rsp_flag(request.flag) : request.expected_rsp_flag;

  mock_bus_clear();

  mock_bus_frame_t tx_frame{};
  tx_frame.dst_addr = request.dst_addr;
  tx_frame.flag = request.flag;
  tx_frame.marker = request.marker;
  tx_frame.block_count = request.tx_block_count;

  for (std::uint8_t i = 0; i < request.tx_block_count; ++i) {
    tx_frame.blocks[i] = request.tx_blocks[i];
  }
  log_tx_request(request);

  const std::uint32_t tx_frame_bytes =
      static_cast<std::uint32_t>(1U + 2U + (static_cast<std::uint32_t>(request.tx_block_count) * 2U) + 1U);
  const std::uint32_t tx_timeout_ms = frames_to_timeout_ms(tx_frame_bytes + 1U);
  const auto tx_status = tx_frame_blocking(writer_tick, tx_frame, tx_timeout_ms);
  if (tx_status != mock_bus_status_t::ok) {
    rsp.status = exchange_status_t::tx_error;
    log_exchange_warning("bus tx failed: ", mock_bus_status_to_string(tx_status));
    return rsp;
  }

  // Timeout for response *start byte* (SOF) is defined in logical frames.
  const std::uint32_t sof_timeout_ms = frames_to_timeout_ms(timeout_frames);
  const auto sof_status = mock_bus_wait_sof(k_cs_addr, sof_timeout_ms);
  if (sof_status != mock_bus_status_t::ok) {
    rsp.status = map_rx_status(sof_status);
    log_exchange_warning("bus rx failed before SOF: ", mock_bus_status_to_string(sof_status));
    return rsp;
  }

  mock_bus_frame_t rx_frame{};
  const std::uint32_t rx_timeout_ms = frames_to_timeout_ms(ARP_BUS_MAX_FRAME_BYTES + 1U);
  const auto rx_status = rx_frame_blocking(rx_timeout_ms, &rx_frame);
  if (rx_status != mock_bus_status_t::ok) {
    rsp.status = map_rx_status(rx_status);
    log_exchange_warning("bus rx failed: ", mock_bus_status_to_string(rx_status));
    return rsp;
  }

  // Source address is inferred from transaction context.
  rsp.src_addr = request.dst_addr;
  rsp.flag = rx_frame.flag;
  rsp.marker = rx_frame.marker;
  rsp.rx_block_count = rx_frame.block_count;

  for (std::uint8_t i = 0; i < rx_frame.block_count; ++i) {
    rsp.rx_blocks[i] = rx_frame.blocks[i];
  }

  if (rx_frame.dst_addr != k_cs_addr) {
    rsp.status = exchange_status_t::unexpected_addr;
    log_exchange_warning("bus rx failed: ", "unexpected destination address");
    return rsp;
  }

  if (rsp.flag != expected_rsp_flag) {
    rsp.status = exchange_status_t::unexpected_flag;
    log_exchange_warning("bus rx failed: ", "unexpected response flag");
    return rsp;
  }

  rsp.status = exchange_status_t::ok;
  log_rx_response(rsp);
  return rsp;
}

const char* to_string(const exchange_status_t status) {
  switch (status) {
    case exchange_status_t::ok:
      return "OK";
    case exchange_status_t::invalid_request:
      return "INVALID_REQUEST";
    case exchange_status_t::tx_error:
      return "TX_ERROR";
    case exchange_status_t::timeout:
      return "TIMEOUT";
    case exchange_status_t::rx_byte_lost:
      return "RX_BYTE_LOST";
    case exchange_status_t::parity_error:
      return "PARITY_ERROR";
    case exchange_status_t::bad_eof:
      return "BAD_EOF";
    case exchange_status_t::bad_frame:
      return "BAD_FRAME";
    case exchange_status_t::unexpected_addr:
      return "UNEXPECTED_ADDR";
    case exchange_status_t::unexpected_flag:
      return "UNEXPECTED_FLAG";
    case exchange_status_t::unexpected_source:
      return "UNEXPECTED_SOURCE";
    default:
      return "UNKNOWN_STATUS";
  }
}

}  // namespace cs::bus_control
