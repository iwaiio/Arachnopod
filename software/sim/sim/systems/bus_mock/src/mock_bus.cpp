#include "mock_bus.hpp"
#include "i_mock_bus.hpp"

#include <chrono>

namespace
{
    mock_bus_state_t g_bus;

    static std::uint64_t now_ns()
    {
        using clock = std::chrono::steady_clock;
        const auto t = clock::now().time_since_epoch();
        return static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t).count());
    }
} // namespace

void mock_bus_init()
{
    g_bus.data.store(0, std::memory_order_relaxed);
    g_bus.is_new.store(0, std::memory_order_relaxed);
    g_bus.writer_addr.store(0, std::memory_order_relaxed);
    g_bus.writer_tick.store(0, std::memory_order_relaxed);
    g_bus.write_time_ns.store(0, std::memory_order_relaxed);
    g_bus.seq.store(0, std::memory_order_relaxed);
}

void mock_bus_clear()
{
    g_bus.data.store(0, std::memory_order_relaxed);
    g_bus.is_new.store(0, std::memory_order_relaxed);
    // g_bus.writer_addr.store(0, std::memory_order_relaxed);
    // g_bus.writer_tick.store(0, std::memory_order_relaxed);
    // g_bus.write_time_ns.store(0, std::memory_order_relaxed);
}

void mock_bus_write_u8(std::uint8_t writer_addr, std::uint32_t writer_tick, std::uint8_t value)
{
    g_bus.data.store(value, std::memory_order_relaxed);
    g_bus.writer_addr.store(writer_addr, std::memory_order_relaxed);
    g_bus.writer_tick.store(writer_tick, std::memory_order_relaxed);
    g_bus.write_time_ns.store(now_ns(), std::memory_order_relaxed);
    g_bus.seq.fetch_add(1, std::memory_order_relaxed);
    g_bus.is_new.store(1, std::memory_order_release);
}

bool mock_bus_read_u8(std::uint8_t* out_value)
{
    if (!out_value)
        return false;

    if (g_bus.is_new.load(std::memory_order_acquire) == 0)
        return false;

    *out_value = g_bus.data.load(std::memory_order_relaxed);
    return true;
}

bool mock_bus_has_new()
{
    return g_bus.is_new.load(std::memory_order_acquire) != 0;
}

std::uint32_t mock_bus_seq()
{
    return g_bus.seq.load(std::memory_order_relaxed);
}

std::uint8_t mock_bus_writer_addr()
{
    return g_bus.writer_addr.load(std::memory_order_relaxed);
}

std::uint32_t mock_bus_writer_tick()
{
    return g_bus.writer_tick.load(std::memory_order_relaxed);
}

std::uint64_t mock_bus_write_time_ns()
{
    return g_bus.write_time_ns.load(std::memory_order_relaxed);
}