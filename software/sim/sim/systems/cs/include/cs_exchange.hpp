#pragma once

#include <cstdint>

namespace cs::exchange {

void init();
void step(std::uint32_t tick);

void set_enabled(bool enabled);
bool is_enabled();

void request_exchange();
void queue_command(std::uint16_t cmd_id, std::int32_t value = 1);

}  // namespace cs::exchange
