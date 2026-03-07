#pragma once

#include <atomic>
#include <cstdint>

// Приватные данные mock_bus.
// Шина моделируется как "провод".

struct mock_bus_state_t
{
    // Текущее значение на шине.
    std::atomic<std::uint8_t> data{0};

    // Признак того, что на шине находится "новый" байт (последний write).
    // Сбрасывается через mock_bus_clear().
    std::atomic<std::uint8_t> is_new{0};

    // Счётчик записей (инкрементируется при каждом write).
    std::atomic<std::uint32_t> seq{0};

    // Адрес системы, последней записавшей байт.
    std::atomic<std::uint8_t> writer_addr{0};

    // Тик системы-писателя.
    std::atomic<std::uint32_t> writer_tick{0};

    // Время записи (ns, steady_clock::now().time_since_epoch()).
    std::atomic<std::uint64_t> write_time_ns{0};
};