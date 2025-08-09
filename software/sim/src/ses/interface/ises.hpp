#pragma once

#include "protocol.hpp"
#include <vector>

// ===================================================================
// POWER SUPPLY SYSTEM INTERFACE (ISES) 
// Чистый интерфейс для взаимодействия с системой энергоснабжения
// Используется как в CVS для управления, так и в SES для реализации
// ===================================================================

namespace ISES {
    
    // Состояния системы питания
    enum class PowerState : uint8_t {
        NORMAL = 0,           // Нормальный режим работы
        POWER_SAVING = 1,     // Режим энергосбережения
        LOW_BATTERY = 2,      // Низкий заряд батареи (20-10%)
        CRITICAL_BATTERY = 3, // Критический заряд батареи (<10%)
        EMERGENCY_SHUTDOWN = 4 // Аварийное отключение
    };
    
    // Коды ошибок
    enum class ErrorCode : uint8_t {
        NONE = 0,                // Нет ошибок
        LOW_BATTERY = 1,         // Низкий заряд батареи
        CRITICAL_BATTERY = 2,    // Критический заряд батареи
        OVERHEAT = 3,           // Перегрев
        OVERCURRENT = 4,        // Превышение тока
        COMMUNICATION_ERROR = 5, // Ошибка связи
        HARDWARE_FAULT = 6      // Аппаратная неисправность
    };
    
    // Биты команд для управления SES (32 бита)
    union CommandBits {
        Protocol::DataBlock raw;
        struct {
            // Команды управления питанием (биты 0-7)
            uint32_t enable_power_saving    : 1;  // бит 0 - включить энергосбережение
            uint32_t disable_power_saving   : 1;  // бит 1 - выключить энергосбережение
            uint32_t emergency_shutdown     : 1;  // бит 2 - аварийное отключение
            uint32_t reset_system          : 1;  // бит 3 - перезагрузка системы
            uint32_t calibrate_sensors     : 1;  // бит 4 - калибровка датчиков
            uint32_t clear_errors          : 1;  // бит 5 - сбросить ошибки
            uint32_t enable_logging        : 1;  // бит 6 - включить логирование
            uint32_t disable_logging       : 1;  // бит 7 - выключить логирование
            
            // Управление питанием модулей (биты 8-15)
            uint32_t power_on_sns          : 1;  // бит 8  - включить SNS
            uint32_t power_off_sns         : 1;  // бит 9  - выключить SNS
            uint32_t power_on_svv          : 1;  // бит 10 - включить SVV
            uint32_t power_off_svv         : 1;  // бит 11 - выключить SVV
            uint32_t power_on_sodf         : 1;  // бит 12 - включить SODF
            uint32_t power_off_sodf        : 1;  // бит 13 - выключить SODF
            uint32_t power_on_sts          : 1;  // бит 14 - включить STS
            uint32_t power_off_sts         : 1;  // бит 15 - выключить STS
            
            // Дополнительные команды (биты 16-23)
            uint32_t power_on_sya          : 1;  // бит 16 - включить SYA
            uint32_t power_off_sya         : 1;  // бит 17 - выключить SYA
            uint32_t power_on_sotr         : 1;  // бит 18 - включить SOTR
            uint32_t power_off_sotr        : 1;  // бит 19 - выключить SOTR
            uint32_t request_detailed_status: 1;  // бит 20 - запросить детальный статус
            uint32_t enter_sleep_mode      : 1;  // бит 21 - войти в спящий режим
            uint32_t wake_from_sleep       : 1;  // бит 22 - выйти из спящего режима
            uint32_t test_mode             : 1;  // бит 23 - тестовый режим
            
            // Резерв для будущего использования (биты 24-31)
            uint32_t reserved              : 8;
        } bits;
        
        CommandBits() { raw.raw = 0; }
        explicit CommandBits(uint32_t value) { raw.raw = value; }
    };
    
    // Данные о состоянии системы (32 бита)
    union StatusData {
        Protocol::DataBlock raw;
        struct {
            uint32_t voltage_mv     : 16;  // Напряжение в милливольтах (0-65535 мВ)
            uint32_t charge_percent : 8;   // Заряд батареи 0-100%
            uint32_t power_state    : 3;   // Состояние PowerState
            uint32_t error_code     : 3;   // Код ошибки ErrorCode
            uint32_t is_healthy     : 1;   // Общее состояние системы
            uint32_t temp_valid     : 1;   // Датчики температуры валидны
        } bits;
        
        StatusData() { raw.raw = 0; }
    };
    
    // Данные о потреблении энергии (32 бита)
    union PowerData {
        Protocol::DataBlock raw;
        struct {
            uint32_t current_ma    : 16;  // Потребляемый ток в мА (0-65535 мА)
            uint32_t power_mw      : 16;  // Потребляемая мощность в мВт (0-65535 мВт)
        } bits;
        
        PowerData() { raw.raw = 0; }
    };
    
    // Данные о температурах (32 бита)
    union TemperatureData {
        Protocol::DataBlock raw;
        struct {
            uint32_t battery_temp   : 8;   // Температура батареи (смещение +50°C)
            uint32_t ambient_temp   : 8;   // Температура окружающей среды (смещение +50°C)
            uint32_t pcb_temp       : 8;   // Температура платы (смещение +50°C)
            uint32_t max_module_temp: 8;   // Максимальная температура среди модулей (смещение +50°C)
        } bits;
        
        TemperatureData() { raw.raw = 0; }
        
        // Вспомогательные методы для преобразования температур
        static uint8_t tempToUint8(float temp_celsius) {
            return static_cast<uint8_t>(temp_celsius + 50.0f);
        }
        
        static float uint8ToTemp(uint8_t temp_offset) {
            return static_cast<float>(temp_offset) - 50.0f;
        }
    };
    
    // Состояние питания модулей (32 бита)
    union ModulePowerStatus {
        Protocol::DataBlock raw;
        struct {
            // Текущее состояние питания модулей (биты 0-7)
            uint32_t cvs_powered    : 1;  // CVS всегда включен
            uint32_t ses_powered    : 1;  // SES всегда включен (сам себя питает)
            uint32_t sns_powered    : 1;  // Система навигации и сканирования
            uint32_t svv_powered    : 1;  // Система визуального восприятия
            uint32_t sodf_powered   : 1;  // Система двигательных функций
            uint32_t sts_powered    : 1;  // Система телеметрии и связи
            uint32_t sya_powered    : 1;  // Система управления аудио
            uint32_t sotr_powered   : 1;  // Система температурного режима
            
            // Флаги ошибок питания модулей (биты 8-15)
            uint32_t cvs_fault      : 1;
            uint32_t ses_fault      : 1;
            uint32_t sns_fault      : 1;
            uint32_t svv_fault      : 1;
            uint32_t sodf_fault     : 1;
            uint32_t sts_fault      : 1;
            uint32_t sya_fault      : 1;
            uint32_t sotr_fault     : 1;
            
            // Дополнительная информация (биты 16-31)
            uint32_t modules_count  : 4;   // Количество активных модулей
            uint32_t faults_count   : 4;   // Количество модулей с ошибками
            uint32_t reserved       : 8;   // Резерв
        } bits;
        
        ModulePowerStatus() { raw.raw = 0; }
    };
    
    // Расширенные данные о батарее (32 бита)
    union BatteryExtendedData {
        Protocol::DataBlock raw;
        struct {
            uint32_t cycles_count   : 16;  // Количество циклов заряда/разряда
            uint32_t health_percent : 8;   // Здоровье батареи 0-100%
            uint32_t cells_count    : 4;   // Количество ячеек в батарее
            uint32_t balancing_active: 1;  // Активна балансировка ячеек
            uint32_t charging       : 1;   // Идет зарядка
            uint32_t discharging    : 1;   // Идет разрядка
            uint32_t reserved       : 1;   // Резерв
        } bits;
        
        BatteryExtendedData() { raw.raw = 0; }
    };
    
    // Полный статус системы для передачи (5 блоков данных)
    struct CompleteStatus {
        StatusData status;                    // Блок 0: основной статус
        PowerData power;                      // Блок 1: данные о потреблении
        TemperatureData temperature;          // Блок 2: температуры
        ModulePowerStatus module_power;       // Блок 3: состояние модулей
        BatteryExtendedData battery_extended; // Блок 4: расширенные данные батареи
        
        // Преобразование в блоки данных для передачи
        std::vector<Protocol::DataBlock> toDataBlocks() const {
            return {
                status.raw, 
                power.raw, 
                temperature.raw, 
                module_power.raw,
                battery_extended.raw
            };
        }
        
        // Создание из полученных блоков данных
        static bool fromDataBlocks(const std::vector<Protocol::DataBlock>& blocks, CompleteStatus& result) {
            if (blocks.size() < 5) return false;
            
            result.status.raw = blocks[0];
            result.power.raw = blocks[1];
            result.temperature.raw = blocks[2];
            result.module_power.raw = blocks[3];
            result.battery_extended.raw = blocks[4];
            
            return true;
        }
    };
    
    // Вспомогательные функции для создания сообщений
    
    // Создать команду управления питанием
    inline Protocol::Message createPowerCommand(const CommandBits& command) {
        return Protocol::Message(Protocol::ModuleID::SES, 
                                Protocol::CommandCode::EXECUTE_COMMAND, 
                                {command.raw});
    }
    
    // Создать запрос статуса
    inline Protocol::Message createStatusRequest() {
        return Protocol::Message(Protocol::ModuleID::SES, 
                                Protocol::CommandCode::REQUEST_DATA);
    }
    
    // Создать ответ со статусом
    inline Protocol::Message createStatusResponse(Protocol::ModuleID target, const CompleteStatus& status) {
        return Protocol::Message(target, 
                                Protocol::CommandCode::DATA_RESPONSE, 
                                status.toDataBlocks());
    }
    
    // Парсинг ответа со статусом
    inline bool parseStatusResponse(const Protocol::Message& msg, CompleteStatus& status) {
        if (msg.getCommand() == Protocol::CommandCode::DATA_RESPONSE && 
            msg.data_blocks.size() >= 5) {
            return CompleteStatus::fromDataBlocks(msg.data_blocks, status);
        }
        return false;
    }
    
    // Создать уведомление об ошибке
    inline Protocol::Message createErrorNotification(Protocol::ModuleID target, ErrorCode error) {
        return Protocol::createErrorResponse(target, static_cast<uint8_t>(error));
    }
    
} // namespace ISES