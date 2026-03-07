#pragma once

#include <cstddef>
#include <cstdint>

// Интерфейсные функции mock_bus чтение/запись/обнуление.
// Для потоков используется атомарное хранение полей, без mutex/очередей.

// Инициализация.
void mock_bus_init();

// Обнуление данных на шине.
void mock_bus_clear();

// Записать 8-битное значение на шину.
// writer_tick — тик системы-писателя.
void mock_bus_write_u8(std::uint8_t writer_addr, std::uint32_t writer_tick, std::uint8_t value);

// Прочитать текущее значение на шине.
// Возвращает false, если "нового" байта нет (is_new == 0).
bool mock_bus_read_u8(std::uint8_t* out_value);

// Для отладки.
bool mock_bus_has_new();
std::uint32_t mock_bus_seq();
std::uint8_t mock_bus_writer_addr();
std::uint32_t mock_bus_writer_tick();
std::uint64_t mock_bus_write_time_ns();