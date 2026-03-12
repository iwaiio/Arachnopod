#include "mock_bus.hpp"
#include "i_mock_bus.hpp"
#include "common.hpp"

#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

namespace {

mock_bus_state_t G_BUS{};

constexpr std::size_t k_addr_slots = 32U;
constexpr auto k_idle_sleep = std::chrono::microseconds(100);
constexpr std::string_view k_bus_module_name = "bus";
constexpr common::log::init_config_t k_bus_log_cfg_truncate{
    .min_level = common::log::level_t::info,
    .truncate_on_init = true,
};
constexpr common::log::init_config_t k_bus_log_cfg_append{
    .min_level = common::log::level_t::info,
    .truncate_on_init = false,
};

enum class rx_phase_t : std::uint8_t {
  wait_sof = 0,
  header_lo,
  header_hi,
  skip_bytes,
  payload_lo,
  payload_hi,
  eof,
};

struct tx_context_t {
  bool active{false};
  std::array<std::uint8_t, ARP_BUS_MAX_FRAME_BYTES> bytes{};
  std::uint8_t total_bytes{0};
  std::uint8_t next_index{0};
  std::uint64_t next_write_time_ns{0};
};

struct rx_context_t {
  bool active{false};
  rx_phase_t phase{rx_phase_t::wait_sof};
  std::uint32_t last_seq{0};
  std::uint8_t src_addr{0};
  std::uint16_t raw_header{0};
  mock_bus_header_t header{};
  std::uint8_t payload_index{0};
  std::uint8_t skip_remaining{0};
  std::uint16_t partial_word{0};
  mock_bus_frame_t frame{};
};

std::array<tx_context_t, k_addr_slots> G_TX{};
std::array<rx_context_t, k_addr_slots> G_RX{};
std::array<std::mutex, k_addr_slots> G_TX_MUTEX{};
std::array<std::mutex, k_addr_slots> G_RX_MUTEX{};

enum class rx_pull_status_t : std::uint8_t {
  no_data = 0,
  ok,
  byte_lost,
};

std::uint64_t now_ns() {
  using clock = std::chrono::steady_clock;
  const auto t = clock::now().time_since_epoch();
  return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t).count());
}

std::uint64_t byte_period_ns() {
  constexpr std::uint32_t k_default_hz = 1000U;
  const auto hz = (ARP_TICK_HZ == 0U) ? k_default_hz : ARP_TICK_HZ;
  const auto speed_percent = (ARP_SIM_SPEED_PERCENT == 0U) ? 100U : ARP_SIM_SPEED_PERCENT;

  const double base_ns = 1'000'000'000.0 / static_cast<double>(hz);
  const double scaled_ns = base_ns * (100.0 / static_cast<double>(speed_percent));

  const auto period = static_cast<std::uint64_t>(std::llround(scaled_ns));
  return (period == 0U) ? 1U : period;
}

std::uint8_t calc_parity_bit(const std::uint16_t raw_without_parity) {
  const unsigned ones = std::popcount(static_cast<unsigned>(raw_without_parity >> 1U));
  return static_cast<std::uint8_t>(ones & 0x1U);
}

void bus_log_write(const common::log::level_t level,
                   const std::string_view message,
                   const bool truncate = false) {
  const auto cfg = truncate ? k_bus_log_cfg_truncate : k_bus_log_cfg_append;
  common::log::write_for_module(k_bus_module_name, level, message, cfg);
}

std::string snapshot_bus_state(const std::string_view op) {
  std::ostringstream oss;
  oss << op << ": data=0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
      << static_cast<unsigned>(G_BUS.data.load(std::memory_order_relaxed)) << std::dec
      << " is_new=" << static_cast<unsigned>(G_BUS.is_new.load(std::memory_order_relaxed))
      << " seq=" << G_BUS.seq.load(std::memory_order_relaxed)
      << " writer=" << static_cast<unsigned>(G_BUS.writer_addr.load(std::memory_order_relaxed))
      << " tick=" << G_BUS.writer_tick.load(std::memory_order_relaxed)
      << " t_ns=" << G_BUS.write_time_ns.load(std::memory_order_relaxed);
  return oss.str();
}

const char* bus_flag_to_string(const std::uint8_t flag) {
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

std::string format_frame(const mock_bus_frame_t& frame, const std::uint8_t src_hint = 0U) {
  const std::uint8_t src_addr = (frame.src_addr == 0U) ? src_hint : frame.src_addr;

  std::ostringstream oss;
  oss << "src=" << static_cast<unsigned>(src_addr) << " dst=" << static_cast<unsigned>(frame.dst_addr)
      << " flag=" << bus_flag_to_string(frame.flag) << " marker=" << static_cast<unsigned>(frame.marker)
      << " blocks=" << static_cast<unsigned>(frame.block_count);
  return oss.str();
}

void bus_log_status(const std::string_view prefix, const mock_bus_status_t status) {
  std::string msg(prefix);
  msg += mock_bus_status_to_string(status);
  bus_log_write(common::log::level_t::warning, msg);
}

void reset_rx_parser(rx_context_t& ctx) {
  ctx.active = false;
  ctx.phase = rx_phase_t::wait_sof;
  ctx.src_addr = 0;
  ctx.raw_header = 0;
  ctx.header = mock_bus_header_t{};
  ctx.payload_index = 0;
  ctx.skip_remaining = 0;
  ctx.partial_word = 0;
  ctx.frame = mock_bus_frame_t{};
}

rx_pull_status_t pull_new_byte(rx_context_t& ctx, std::uint8_t* out_byte, std::uint8_t* out_writer_addr) {
  if ((out_byte == nullptr) || (out_writer_addr == nullptr)) {
    bus_log_write(common::log::level_t::error, snapshot_bus_state("pull_new_byte invalid arguments"));
    return rx_pull_status_t::no_data;
  }

  if (G_BUS.is_new.load(std::memory_order_acquire) == 0U) {
    return rx_pull_status_t::no_data;
  }

  const auto seq = G_BUS.seq.load(std::memory_order_relaxed);
  if (seq == ctx.last_seq) {
    return rx_pull_status_t::no_data;
  }

  const bool lost = ctx.active && (seq > (ctx.last_seq + 1U));

  ctx.last_seq = seq;
  *out_byte = G_BUS.data.load(std::memory_order_relaxed);
  *out_writer_addr = G_BUS.writer_addr.load(std::memory_order_relaxed);

  {
    std::ostringstream oss;
    oss << snapshot_bus_state("pull_new_byte") << " rx=0x" << std::hex << std::uppercase << std::setw(2)
        << std::setfill('0') << static_cast<unsigned>(*out_byte) << std::dec;
    bus_log_write(common::log::level_t::info, oss.str());
  }

  if (lost) {
    return rx_pull_status_t::byte_lost;
  }

  return rx_pull_status_t::ok;
}

mock_bus_status_t process_rx_byte(rx_context_t& ctx,
                                  const std::uint8_t self_addr,
                                  const std::uint8_t byte,
                                  const std::uint8_t writer_addr,
                                  mock_bus_frame_t* out_frame) {
  switch (ctx.phase) {
    case rx_phase_t::wait_sof: {
      if (byte != ARP_BUS_SOF) {
        return mock_bus_status_t::timeout;
      }

      reset_rx_parser(ctx);
      ctx.active = true;
      ctx.phase = rx_phase_t::header_lo;
      ctx.src_addr = writer_addr;
      return mock_bus_status_t::timeout;
    }

    case rx_phase_t::header_lo: {
      if (writer_addr != ctx.src_addr) {
        reset_rx_parser(ctx);
        return mock_bus_status_t::bad_frame;
      }

      ctx.raw_header = static_cast<std::uint16_t>(byte);
      ctx.phase = rx_phase_t::header_hi;
      return mock_bus_status_t::timeout;
    }

    case rx_phase_t::header_hi: {
      if (writer_addr != ctx.src_addr) {
        reset_rx_parser(ctx);
        return mock_bus_status_t::bad_frame;
      }

      ctx.raw_header |= static_cast<std::uint16_t>(byte) << 8U;
      if (!mock_bus_parse_header(ctx.raw_header, &ctx.header)) {
        reset_rx_parser(ctx);
        return mock_bus_status_t::parity_error;
      }

      ctx.frame = mock_bus_frame_t{};
      ctx.frame.src_addr = ctx.src_addr;
      ctx.frame.dst_addr = ctx.header.addr;
      ctx.frame.flag = ctx.header.flag;
      ctx.frame.marker = ctx.header.marker;
      ctx.frame.block_count = ctx.header.nblocks;

      if (ctx.header.nblocks > ARP_BUS_MAX_DATA_BLOCKS) {
        reset_rx_parser(ctx);
        return mock_bus_status_t::bad_frame;
      }

      if (ctx.header.addr != self_addr) {
        ctx.phase = rx_phase_t::skip_bytes;
        ctx.skip_remaining = static_cast<std::uint8_t>(ctx.header.nblocks * 2U + 1U);
      } else if (ctx.header.nblocks == 0U) {
        ctx.phase = rx_phase_t::eof;
      } else {
        ctx.phase = rx_phase_t::payload_lo;
      }

      return mock_bus_status_t::timeout;
    }

    case rx_phase_t::skip_bytes: {
      if (writer_addr != ctx.src_addr) {
        reset_rx_parser(ctx);
        return mock_bus_status_t::bad_frame;
      }

      if (ctx.skip_remaining == 0U) {
        reset_rx_parser(ctx);
        return mock_bus_status_t::bad_frame;
      }

      --ctx.skip_remaining;
      if (ctx.skip_remaining == 0U) {
        if (byte != ARP_BUS_EOF) {
          reset_rx_parser(ctx);
          return mock_bus_status_t::bad_eof;
        }

        reset_rx_parser(ctx);
      }

      return mock_bus_status_t::timeout;
    }

    case rx_phase_t::payload_lo: {
      if (writer_addr != ctx.src_addr) {
        reset_rx_parser(ctx);
        return mock_bus_status_t::bad_frame;
      }

      ctx.partial_word = static_cast<std::uint16_t>(byte);
      ctx.phase = rx_phase_t::payload_hi;
      return mock_bus_status_t::timeout;
    }

    case rx_phase_t::payload_hi: {
      if (writer_addr != ctx.src_addr) {
        reset_rx_parser(ctx);
        return mock_bus_status_t::bad_frame;
      }

      ctx.partial_word |= static_cast<std::uint16_t>(byte) << 8U;

      if (ctx.payload_index >= ARP_BUS_MAX_DATA_BLOCKS) {
        reset_rx_parser(ctx);
        return mock_bus_status_t::bad_frame;
      }

      ctx.frame.blocks[ctx.payload_index] = ctx.partial_word;
      ++ctx.payload_index;

      if (ctx.payload_index >= ctx.header.nblocks) {
        ctx.phase = rx_phase_t::eof;
      } else {
        ctx.phase = rx_phase_t::payload_lo;
      }

      return mock_bus_status_t::timeout;
    }

    case rx_phase_t::eof: {
      if (writer_addr != ctx.src_addr) {
        reset_rx_parser(ctx);
        return mock_bus_status_t::bad_frame;
      }

      if (byte != ARP_BUS_EOF) {
        reset_rx_parser(ctx);
        return mock_bus_status_t::bad_eof;
      }

      if (out_frame == nullptr) {
        reset_rx_parser(ctx);
        return mock_bus_status_t::invalid_argument;
      }

      *out_frame = ctx.frame;
      reset_rx_parser(ctx);
      return mock_bus_status_t::ok;
    }

    default:
      reset_rx_parser(ctx);
      return mock_bus_status_t::bad_frame;
  }
}

void build_tx_frame(tx_context_t& tx, const mock_bus_frame_t& frame) {
  tx = tx_context_t{};
  tx.active = true;

  const auto block_count = frame.block_count;
  const auto frame_len = static_cast<std::uint8_t>(1U + 2U + block_count * 2U + 1U);
  tx.total_bytes = frame_len;

  tx.bytes[0] = ARP_BUS_SOF;

  const auto raw_header = mock_bus_make_header(frame.dst_addr, frame.flag, frame.marker, frame.block_count);
  tx.bytes[1] = static_cast<std::uint8_t>(raw_header & 0x00FFU);
  tx.bytes[2] = static_cast<std::uint8_t>((raw_header >> 8U) & 0x00FFU);

  std::size_t bi = 3U;
  for (std::uint8_t i = 0; i < block_count; ++i) {
    const auto word = frame.blocks[i];
    tx.bytes[bi++] = static_cast<std::uint8_t>(word & 0x00FFU);
    tx.bytes[bi++] = static_cast<std::uint8_t>((word >> 8U) & 0x00FFU);
  }

  tx.bytes[bi] = ARP_BUS_EOF;
}

}  // namespace

void mock_bus_init() {
  G_BUS.data.store(0, std::memory_order_relaxed);
  G_BUS.is_new.store(0, std::memory_order_relaxed);
  G_BUS.rx_addr.store(0, std::memory_order_relaxed);
  G_BUS.seq.store(0, std::memory_order_relaxed);
  G_BUS.writer_addr.store(0, std::memory_order_relaxed);
  G_BUS.writer_tick.store(0, std::memory_order_relaxed);
  G_BUS.write_time_ns.store(0, std::memory_order_relaxed);

  for (auto& tx : G_TX) {
    tx = tx_context_t{};
  }

  for (auto& rx : G_RX) {
    rx = rx_context_t{};
  }

  bus_log_write(common::log::level_t::info, snapshot_bus_state("bus initialized"), true);
}

void mock_bus_clear() {
  G_BUS.data.store(0, std::memory_order_relaxed);
  G_BUS.is_new.store(0, std::memory_order_relaxed);
  bus_log_write(common::log::level_t::info, snapshot_bus_state("bus cleared"));
}

void mock_bus_write_u8(const std::uint8_t writer_addr, const std::uint32_t writer_tick, const std::uint8_t value) {
  G_BUS.data.store(value, std::memory_order_relaxed);
  G_BUS.writer_addr.store(writer_addr, std::memory_order_relaxed);
  G_BUS.writer_tick.store(writer_tick, std::memory_order_relaxed);
  G_BUS.write_time_ns.store(now_ns(), std::memory_order_relaxed);
  const auto seq = G_BUS.seq.fetch_add(1, std::memory_order_relaxed) + 1U;
  G_BUS.is_new.store(1, std::memory_order_release);

  std::ostringstream oss;
  oss << snapshot_bus_state("write_u8") << " seq_written=" << seq;
  bus_log_write(common::log::level_t::info, oss.str());
}

bool mock_bus_read_u8(std::uint8_t* out_value) {
  if (out_value == nullptr) {
    bus_log_write(common::log::level_t::error, snapshot_bus_state("read_u8 failed: out_value=nullptr"));
    return false;
  }

  if (G_BUS.is_new.load(std::memory_order_acquire) == 0U) {
    bus_log_write(common::log::level_t::info, snapshot_bus_state("read_u8: no data"));
    return false;
  }

  *out_value = G_BUS.data.load(std::memory_order_relaxed);

  std::ostringstream oss;
  oss << snapshot_bus_state("read_u8: ok") << " out=0x" << std::hex << std::uppercase << std::setw(2)
      << std::setfill('0') << static_cast<unsigned>(*out_value) << std::dec;
  bus_log_write(common::log::level_t::info, oss.str());

  return true;
}

bool mock_bus_has_new() {
  return G_BUS.is_new.load(std::memory_order_acquire) != 0U;
}

std::uint32_t mock_bus_seq() {
  return G_BUS.seq.load(std::memory_order_relaxed);
}

std::uint8_t mock_bus_writer_addr() {
  return G_BUS.writer_addr.load(std::memory_order_relaxed);
}

std::uint32_t mock_bus_writer_tick() {
  return G_BUS.writer_tick.load(std::memory_order_relaxed);
}

std::uint64_t mock_bus_write_time_ns() {
  return G_BUS.write_time_ns.load(std::memory_order_relaxed);
}

std::uint16_t mock_bus_make_header(const std::uint8_t addr,
                                   const std::uint8_t flag,
                                   const std::uint8_t marker,
                                   const std::uint8_t nblocks) {
  std::uint16_t raw = 0;

  raw |= (static_cast<std::uint16_t>(addr) & MOCK_BUS_HDR_ADDR_MASK) << MOCK_BUS_HDR_ADDR_SHIFT;
  raw |= (static_cast<std::uint16_t>(flag) & MOCK_BUS_HDR_FLAG_MASK) << MOCK_BUS_HDR_FLAG_SHIFT;
  raw |= (static_cast<std::uint16_t>(marker) & MOCK_BUS_HDR_MARKER_MASK) << MOCK_BUS_HDR_MARKER_SHIFT;
  raw |= (static_cast<std::uint16_t>(nblocks) & MOCK_BUS_HDR_NBLOCKS_MASK) << MOCK_BUS_HDR_NBLOCKS_SHIFT;

  raw &= static_cast<std::uint16_t>(~MOCK_BUS_HDR_PARITY_MASK);
  raw |= static_cast<std::uint16_t>(calc_parity_bit(raw));
  return raw;
}

bool mock_bus_header_parity_ok(const std::uint16_t raw_header) {
  const auto raw_without_parity = static_cast<std::uint16_t>(raw_header & ~MOCK_BUS_HDR_PARITY_MASK);
  const auto expected_parity = calc_parity_bit(raw_without_parity);
  const auto parity = static_cast<std::uint8_t>((raw_header >> MOCK_BUS_HDR_PARITY_SHIFT) & MOCK_BUS_HDR_PARITY_MASK);
  return parity == expected_parity;
}

bool mock_bus_parse_header(const std::uint16_t raw_header, mock_bus_header_t* out_header) {
  if (out_header == nullptr) {
    return false;
  }

  if (!mock_bus_header_parity_ok(raw_header)) {
    return false;
  }

  out_header->parity =
      static_cast<std::uint8_t>((raw_header >> MOCK_BUS_HDR_PARITY_SHIFT) & MOCK_BUS_HDR_PARITY_MASK);
  out_header->addr = static_cast<std::uint8_t>((raw_header >> MOCK_BUS_HDR_ADDR_SHIFT) & MOCK_BUS_HDR_ADDR_MASK);
  out_header->flag = static_cast<std::uint8_t>((raw_header >> MOCK_BUS_HDR_FLAG_SHIFT) & MOCK_BUS_HDR_FLAG_MASK);
  out_header->marker =
      static_cast<std::uint8_t>((raw_header >> MOCK_BUS_HDR_MARKER_SHIFT) & MOCK_BUS_HDR_MARKER_MASK);
  out_header->nblocks =
      static_cast<std::uint8_t>((raw_header >> MOCK_BUS_HDR_NBLOCKS_SHIFT) & MOCK_BUS_HDR_NBLOCKS_MASK);
  return true;
}

mock_bus_status_t mock_bus_broadcast_frame(const std::uint8_t writer_addr,
                                           const std::uint32_t writer_tick,
                                           const mock_bus_frame_t& frame) {
  if (writer_addr >= k_addr_slots) {
    bus_log_write(common::log::level_t::error, snapshot_bus_state("broadcast failed: writer address out of range"));
    return mock_bus_status_t::invalid_argument;
  }

  if (frame.block_count > ARP_BUS_MAX_DATA_BLOCKS) {
    bus_log_write(common::log::level_t::error, snapshot_bus_state("broadcast failed: block_count out of range"));
    return mock_bus_status_t::invalid_argument;
  }

  std::lock_guard<std::mutex> lock(G_TX_MUTEX[writer_addr]);
  tx_context_t& tx = G_TX[writer_addr];
  if (!tx.active) {
    build_tx_frame(tx, frame);
    std::ostringstream oss;
    oss << snapshot_bus_state("frame tx begin") << " writer=" << static_cast<unsigned>(writer_addr)
        << " tick=" << writer_tick << ' ' << format_frame(frame, writer_addr);
    bus_log_write(common::log::level_t::info, oss.str());
  }

  const auto now = now_ns();
  if (now < tx.next_write_time_ns) {
    return mock_bus_status_t::timeout;
  }

  if (tx.next_index >= tx.total_bytes) {
    tx.active = false;
    bus_log_write(common::log::level_t::info, snapshot_bus_state("frame tx done"));
    return mock_bus_status_t::ok;
  }

  mock_bus_write_u8(writer_addr, writer_tick, tx.bytes[tx.next_index]);
  ++tx.next_index;
  tx.next_write_time_ns = now + byte_period_ns();

  if (tx.next_index >= tx.total_bytes) {
    tx.active = false;
    bus_log_write(common::log::level_t::info, snapshot_bus_state("frame tx done"));
    return mock_bus_status_t::ok;
  }

  return mock_bus_status_t::timeout;
}

mock_bus_status_t mock_bus_wait_sof(const std::uint8_t self_addr, const std::uint32_t timeout_ms) {
  if (self_addr >= k_addr_slots) {
    bus_log_write(common::log::level_t::error, snapshot_bus_state("wait_sof failed: invalid arguments"));
    return mock_bus_status_t::invalid_argument;
  }

  std::lock_guard<std::mutex> lock(G_RX_MUTEX[self_addr]);
  rx_context_t& rx = G_RX[self_addr];

  // Start timeout is measured from a clean state for a new response.
  if (rx.active && (rx.phase != rx_phase_t::wait_sof)) {
    reset_rx_parser(rx);
  }

  const auto deadline = now_ns() + static_cast<std::uint64_t>(timeout_ms) * 1'000'000ULL;
  while (now_ns() <= deadline) {
    std::uint8_t byte = 0;
    std::uint8_t writer = 0;
    const auto pull_status = pull_new_byte(rx, &byte, &writer);

    if (pull_status == rx_pull_status_t::no_data) {
      std::this_thread::sleep_for(k_idle_sleep);
      continue;
    }

    if (pull_status == rx_pull_status_t::byte_lost) {
      reset_rx_parser(rx);
      bus_log_status("wait_sof failed: ", mock_bus_status_t::rx_byte_lost);
      return mock_bus_status_t::rx_byte_lost;
    }

    const auto status = process_rx_byte(rx, self_addr, byte, writer, nullptr);
    if ((status == mock_bus_status_t::timeout) && rx.active && (rx.phase == rx_phase_t::header_lo)) {
      bus_log_write(common::log::level_t::info, snapshot_bus_state("wait_sof: sof detected"));
      return mock_bus_status_t::ok;
    }

    if ((status != mock_bus_status_t::timeout) && (status != mock_bus_status_t::ok)) {
      bus_log_status("wait_sof failed: ", status);
      return status;
    }
  }

  return mock_bus_status_t::timeout;
}

mock_bus_status_t mock_bus_listen_frame(const std::uint8_t self_addr,
                                        const std::uint32_t timeout_ms,
                                        mock_bus_frame_t* out_frame) {
  if ((self_addr >= k_addr_slots) || (out_frame == nullptr)) {
    bus_log_write(common::log::level_t::error, snapshot_bus_state("listen failed: invalid arguments"));
    return mock_bus_status_t::invalid_argument;
  }

  std::lock_guard<std::mutex> lock(G_RX_MUTEX[self_addr]);
  rx_context_t& rx = G_RX[self_addr];
  const auto deadline = now_ns() + static_cast<std::uint64_t>(timeout_ms) * 1'000'000ULL;

  while (now_ns() <= deadline) {
    std::uint8_t byte = 0;
    std::uint8_t writer = 0;
    const auto pull_status = pull_new_byte(rx, &byte, &writer);

    if (pull_status == rx_pull_status_t::no_data) {
      std::this_thread::sleep_for(k_idle_sleep);
      continue;
    }

    if (pull_status == rx_pull_status_t::byte_lost) {
      reset_rx_parser(rx);
      bus_log_status("listen failed: ", mock_bus_status_t::rx_byte_lost);
      return mock_bus_status_t::rx_byte_lost;
    }

    const auto status = process_rx_byte(rx, self_addr, byte, writer, out_frame);
    if (status != mock_bus_status_t::timeout) {
      if (status == mock_bus_status_t::ok) {
        std::ostringstream oss;
        oss << snapshot_bus_state("frame rx") << " self=" << static_cast<unsigned>(self_addr) << ' '
            << format_frame(*out_frame);
        bus_log_write(common::log::level_t::info, oss.str());
      } else {
        bus_log_status("listen failed: ", status);
      }
      return status;
    }
  }

  return mock_bus_status_t::timeout;
}

const char* mock_bus_status_to_string(const mock_bus_status_t status) {
  switch (status) {
    case mock_bus_status_t::ok:
      return "OK";
    case mock_bus_status_t::invalid_argument:
      return "INVALID_ARGUMENT";
    case mock_bus_status_t::timeout:
      return "TIMEOUT";
    case mock_bus_status_t::rx_byte_lost:
      return "RX_BYTE_LOST";
    case mock_bus_status_t::parity_error:
      return "PARITY_ERROR";
    case mock_bus_status_t::bad_eof:
      return "BAD_EOF";
    case mock_bus_status_t::bad_frame:
      return "BAD_FRAME";
    default:
      return "UNKNOWN";
  }
}
