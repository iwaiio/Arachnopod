#pragma once

#include <array>
#include <cstdint>

#include "arp_config.hpp"

enum class mock_bus_status_t : std::uint8_t {
  ok = 0,
  invalid_argument,
  timeout,
  rx_byte_lost,
  parity_error,
  bad_eof,
  bad_frame,
};

enum class mock_bus_flag_t : std::uint8_t {
  uv = ARP_FLAG_UV,
  uv_resp = ARP_FLAG_UV_RESP,
  req_data = ARP_FLAG_REQ_DATA,
  req_data_resp = ARP_FLAG_REQ_DATA_RESP,
};

constexpr std::uint8_t MOCK_BUS_HDR_PARITY_SHIFT = 0u;
constexpr std::uint8_t MOCK_BUS_HDR_ADDR_SHIFT = 1u;
constexpr std::uint8_t MOCK_BUS_HDR_FLAG_SHIFT = 6u;
constexpr std::uint8_t MOCK_BUS_HDR_MARKER_SHIFT = 8u;
constexpr std::uint8_t MOCK_BUS_HDR_NBLOCKS_SHIFT = 11u;

constexpr std::uint16_t MOCK_BUS_HDR_PARITY_MASK = 0x0001u;
constexpr std::uint16_t MOCK_BUS_HDR_ADDR_MASK = 0x001Fu;
constexpr std::uint16_t MOCK_BUS_HDR_FLAG_MASK = 0x0003u;
constexpr std::uint16_t MOCK_BUS_HDR_MARKER_MASK = 0x0007u;
constexpr std::uint16_t MOCK_BUS_HDR_NBLOCKS_MASK = 0x001Fu;

struct mock_bus_header_t {
  std::uint8_t parity{0};
  std::uint8_t addr{0};
  std::uint8_t flag{0};
  std::uint8_t marker{0};
  std::uint8_t nblocks{0};
};

struct mock_bus_frame_t {
  std::uint8_t src_addr{0};
  std::uint8_t dst_addr{0};
  std::uint8_t flag{0};
  std::uint8_t marker{0};
  std::uint8_t block_count{0};
  std::array<std::uint16_t, ARP_BUS_MAX_DATA_BLOCKS> blocks{};
};

void mock_bus_init();
void mock_bus_clear();

void mock_bus_write_u8(std::uint8_t writer_addr, std::uint32_t writer_tick, std::uint8_t value);
bool mock_bus_read_u8(std::uint8_t* out_value);
bool mock_bus_has_new();

// Debug metadata of the latest byte present on bus.
std::uint32_t mock_bus_seq();
std::uint8_t mock_bus_writer_addr();
std::uint32_t mock_bus_writer_tick();
std::uint64_t mock_bus_write_time_ns();

std::uint16_t mock_bus_make_header(std::uint8_t addr,
                                   std::uint8_t flag,
                                   std::uint8_t marker,
                                   std::uint8_t nblocks);
bool mock_bus_header_parity_ok(std::uint16_t raw_header);
bool mock_bus_parse_header(std::uint16_t raw_header, mock_bus_header_t* out_header);

mock_bus_status_t mock_bus_broadcast_frame(std::uint8_t writer_addr,
                                           std::uint32_t writer_tick,
                                           const mock_bus_frame_t& frame);
// Wait only for response start byte (SOF) and prime RX parser state.
// Returns OK when SOF is consumed by parser.
mock_bus_status_t mock_bus_wait_sof(std::uint8_t self_addr, std::uint32_t timeout_ms);

mock_bus_status_t mock_bus_listen_frame(std::uint8_t self_addr,
                                        std::uint32_t timeout_ms,
                                        mock_bus_frame_t* out_frame);

const char* mock_bus_status_to_string(mock_bus_status_t status);
