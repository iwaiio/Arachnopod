#pragma once

// Общие константы проекта.

// --- Тактирование и симуляция ---
// Базовая частота "логического" тика.
#define ARP_TICK_HZ                     100u

// Ускорение симуляции в процентах (100 = реальное время, 200 = x2, 50 = x0.5).
#define ARP_SIM_SPEED_PERCENT           10u

// Температура окружающей среды.
#define ARP_ENV_AMBIENT_TEMP_C          25

// --- Шина / протокол ---
#define ARP_BUS_WORD_BITS               8u

#define ARP_BUS_SOF                     0xAAu
#define ARP_BUS_EOF                     0x55u

// Максимальное число 16-бит блоков данных (N) в сообщении.
#define ARP_BUS_MAX_DATA_BLOCKS         31u

// Максимальная длина кадра: SOF(1) + HEADER(2) + N*2 + EOF(1)
#define ARP_BUS_MAX_FRAME_BYTES         (1u + 2u + (ARP_BUS_MAX_DATA_BLOCKS * 2u) + 1u)

// Флаги сообщения
#define ARP_FLAG_CMD_REQ                0u
#define ARP_FLAG_CMD_RESP               1u
#define ARP_FLAG_DATA_REQ               2u
#define ARP_FLAG_DATA_RESP              3u
#define ARP_FLAG_NONE                   4u

// Ожидание ответного сообщения от оконечного устройства (мс)
#define ARP_BUS_RESPONSE_TIMEOUT_MS     2500u

// РћР¶РёРґР°РЅРёРµ РЅР°С‡Р°Р»Р° РѕС‚РІРµС‚Р° (SOF) РІ РєР°РґСЂР°С… СЃРёРјСѓР»СЏС†РёРё.
#define ARP_BUS_RESPONSE_TIMEOUT_FRAMES 10u

// --- Адреса устройств на шине ---
#define ARP_ADDR_CS                     0x01u
#define ARP_ADDR_PSS                    0x02u
#define ARP_ADDR_TCS                    0x03u
#define ARP_ADDR_TMS                    0x04u
#define ARP_ADDR_MNS                    0x05u
#define ARP_ADDR_LS                     0x06u

// --- Фазы систем ---
#define ARP_PHASE_TX                    0u
#define ARP_PHASE_RX                    1u
#define ARP_PHASE_WAIT_SOF              2u
