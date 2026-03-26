#pragma once

#include <cstdint>

void mock_bus_init();
void mock_bus_tick(std::uint32_t tick);

void mock_bus_write_u8(std::uint32_t tick, std::uint8_t value);
bool mock_bus_read_u8(std::uint32_t tick, std::uint8_t* out_value);
