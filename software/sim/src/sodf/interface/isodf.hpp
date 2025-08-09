#pragma once

#include "protocol.hpp"
#include <vector>
#include <array>

// ===================================================================
// DRIVE SYSTEM INTERFACE (ISODF)
// Интерфейс системы обеспечения двигательных функций
// Используется в CVS для управления и в SODF для реализации
// ===================================================================

namespace ISODF {
    
    // Состояния системы движения
    enum class MotionState : uint8_t {
        IDLE = 0,           // Покой, все приводы выключены
        STANDING = 1,       // Стоит на месте
        WALKING = 2,        // Идет вперед
        WALKING_BACK = 3,   // Идет назад
        TURNING_LEFT = 4,   // Поворачивает влево
        TURNING_RIGHT = 5,  // Поворачивает вправо
        CLIMBING = 6,       // Преодолевает препятствие
        BALANCING = 7,      // Балансирует
        CALIBRATING = 8,    // Калибровка приводов
        ERROR = 9,          // Ошибка системы движения
        EMERGENCY_STOP = 10 // Аварийная остановка
    };
    
    // Типы походок
    enum class GaitType : uint8_t {
        TRIPOD = 0,         // Трехногая походка (быстрая)
        WAVE = 1,           // Волновая походка (медленная, стабильная)
        RIPPLE = 2,         // Рябь (средняя скорость)
        QUADRUPED = 3,      // Четырехногая походка
        CUSTOM = 4          // Пользовательская походка
    };
    
    // Коды ошибок системы движения
    enum class ErrorCode : uint8_t {
        NONE = 0,                   // Нет ошибок
        SERVO_OVERLOAD = 1,         // Перегрузка сервопривода
        SERVO_OVERHEAT = 2,         // Перегрев сервопривода
        SERVO_DISCONNECTED = 3,     // Сервопривод не отвечает
        POSITION_ERROR = 4,         // Ошибка позиционирования
        POWER_INSUFFICIENT = 5,     // Недостаточно питания
        MECHANICAL_BLOCK = 6,       // Механическая блокировка
        CALIBRATION_FAILED = 7,     // Ошибка калибровки
        COMMUNICATION_ERROR = 8,    // Ошибка связи с приводами
        BALANCE_LOST = 9           // Потеря равновесия
    };
    
    // Команды управления движением (32 бита)
    union CommandBits {
        Protocol::DataBlock raw;
        struct {
            // Основные команды движения (биты 0-7)
            uint32_t start_walking      : 1;  // бит 0 - начать движение вперед
            uint32_t stop_motion        : 1;  // бит 1 - остановить все движение
            uint32_t walk_backward      : 1;  // бит 2 - движение назад
            uint32_t turn_left          : 1;  // бит 3 - поворот влево
            uint32_t turn_right         : 1;  // бит 4 - поворот вправо
            uint32_t stand_up           : 1;  // бит 5 - встать
            uint32_t sit_down           : 1;  // бит 6 - сесть
            uint32_t emergency_stop     : 1;  // бит 7 - аварийная остановка
            
            // Параметры движения (биты 8-15)
            uint32_t speed_level        : 3;  // биты 8-10 - уровень скорости (0-7)
            uint32_t gait_type          : 3;  // биты 11-13 - тип походки
            uint32_t height_adjust      : 2;  // биты 14-15 - регулировка высоты
            
            // Специальные команды (биты 16-23)
            uint32_t calibrate_servos   : 1;  // бит 16 - калибровка приводов
            uint32_t reset_position     : 1;  // бит 17 - сброс в исходное положение
            uint32_t balance_mode       : 1;  // бит 18 - режим балансировки
            uint32_t climb_mode         : 1;  // бит 19 - режим преодоления препятствий
            uint32_t dance_mode         : 1;  // бит 20 - демо режим "танец"
            uint32_t low_power_mode     : 1;  // бит 21 - режим низкого потребления
            uint32_t test_servo         : 1;  // бит 22 - тест конкретного привода
            uint32_t lock_servos        : 1;  // бит 23 - заблокировать все приводы
            
            // Параметры команды (биты 24-31)
            uint32_t servo_id           : 5;  // биты 24-28 - ID привода для теста
            uint32_t reserved           : 3;  // биты 29-31 - резерв
        } bits;
        
        CommandBits() { raw.raw = 0; }
        explicit CommandBits(uint32_t value) { raw.raw = value; }
    };
    
    // Данные о состоянии системы движения (32 бита)
    union StatusData {
        Protocol::DataBlock raw;
        struct {
            uint32_t motion_state    : 4;   // Текущее состояние MotionState
            uint32_t gait_type       : 3;   // Текущий тип походки
            uint32_t is_moving       : 1;   // Движется ли робот
            uint32_t is_balanced     : 1;   // Сбалансирован ли робот
            uint32_t is_calibrated   : 1;   // Откалиброван ли
            uint32_t has_error       : 1;   // Есть ли ошибка
            uint32_t speed_level     : 3;   // Текущий уровень скорости
            uint32_t height_level    : 2;   // Уровень высоты корпуса
            uint32_t active_servos   : 6;   // Количество активных приводов
            uint32_t error_servos    : 6;   // Количество приводов с ошибками
            uint32_t error_code      : 4;   // Код последней ошибки
        } bits;
        
        StatusData() { raw.raw = 0; }
    };
    
    // Данные о сервоприводах (32 бита на группу)
    union ServoGroupData {
        Protocol::DataBlock raw;
        struct {
            // Данные для группы из 4 сервоприводов (по 8 бит на привод)
            uint32_t servo0_position : 8;  // Позиция привода 0 (0-255 -> 0-360°)
            uint32_t servo1_position : 8;  // Позиция привода 1
            uint32_t servo2_position : 8;  // Позиция привода 2
            uint32_t servo3_position : 8;  // Позиция привода 3
        } positions;
        
        struct {
            // Альтернативное представление - токи потребления
            uint32_t servo0_current  : 8;  // Ток привода 0 в единицах 10мА
            uint32_t servo1_current  : 8;  // Ток привода 1
            uint32_t servo2_current  : 8;  // Ток привода 2
            uint32_t servo3_current  : 8;  // Ток привода 3
        } currents;
        
        ServoGroupData() { raw.raw = 0; }
    };
    
    // Данные о потреблении энергии системой движения (32 бита)
    union PowerData {
        Protocol::DataBlock raw;
        struct {
            uint32_t total_current_ma   : 16;  // Общий ток всех приводов в мА
            uint32_t peak_current_ma    : 16;  // Пиковый ток за период в мА
        } bits;
        
        PowerData() { raw.raw = 0; }
    };
    
    // Данные о позиции и ориентации робота (32 бита)
    union PoseData {
        Protocol::DataBlock raw;
        struct {
            uint32_t body_height_mm  : 10;  // Высота корпуса в мм (0-1023)
            uint32_t pitch_deg       : 7;   // Наклон вперед/назад (-64..+63°)
            uint32_t roll_deg        : 7;   // Наклон влево/вправо (-64..+63°)
            uint32_t yaw_deg         : 8;   // Поворот (0-255 -> 0-360°)
        } bits;
        
        PoseData() { raw.raw = 0; }
    };
    
    // Информация о конкретной ноге (32 бита)
    union LegData {
        Protocol::DataBlock raw;
        struct {
            uint32_t coxa_angle      : 8;   // Угол базового сустава (0-255 -> 0-360°)
            uint32_t femur_angle     : 8;   // Угол бедра (0-255 -> 0-360°)
            uint32_t tibia_angle     : 8;   // Угол голени (0-255 -> 0-360°)
            uint32_t is_grounded     : 1;   // Нога на земле
            uint32_t has_contact     : 1;   // Есть контакт с поверхностью
            uint32_t has_error       : 1;   // Ошибка в ноге
            uint32_t leg_id          : 3;   // ID ноги (0-5)
            uint32_t reserved        : 2;   // Резерв
        } bits;
        
        LegData() { raw.raw = 0; }
    };
    
    // Полный статус системы движения (7 блоков данных)
    struct CompleteStatus {
        StatusData status;                      // Блок 0: основной статус
        PowerData power;                       // Блок 1: потребление энергии
        PoseData pose;                         // Блок 2: поза робота
        std::array<LegData, 6> legs;          // Блоки 3-8: данные о ногах
        
        // Преобразование в блоки данных для передачи
        std::vector<Protocol::DataBlock> toDataBlocks() const {
            std::vector<Protocol::DataBlock> blocks;
            blocks.reserve(9);
            
            blocks.push_back(status.raw);
            blocks.push_back(power.raw);
            blocks.push_back(pose.raw);
            
            for (const auto& leg : legs) {
                blocks.push_back(leg.raw);
            }
            
            return blocks;
        }
        
        // Создание из полученных блоков данных
        static bool fromDataBlocks(const std::vector<Protocol::DataBlock>& blocks, CompleteStatus& result) {
            if (blocks.size() < 9) return false;
            
            result.status.raw = blocks[0];
            result.power.raw = blocks[1];
            result.pose.raw = blocks[2];
            
            for (size_t i = 0; i < 6; ++i) {
                result.legs[i].raw = blocks[3 + i];
            }
            
            return true;
        }
    };
    
    // Параметры движения для отправки роботу
    struct MotionParameters {
        Protocol::DataBlock raw;
        struct {
            uint32_t forward_speed_mmps : 10;  // Скорость вперед в мм/с (0-1023)
            uint32_t lateral_speed_mmps : 10;  // Боковая скорость в мм/с
            uint32_t rotation_speed_dps  : 9;   // Скорость поворота в град/с (0-511)
            uint32_t step_height_mm     : 3;   // Высота шага в условных единицах
        } bits;
        
        MotionParameters() { raw.raw = 0; }
    };
    
    // ===== Вспомогательные функции для создания сообщений =====
    
    // Создать команду движения
    inline Protocol::Message createMotionCommand(const CommandBits& command) {
        return Protocol::Message(Protocol::ModuleID::DS, 
                                Protocol::CommandCode::EXECUTE_COMMAND, 
                                {command.raw});
    }
    
    // Создать команду с параметрами движения
    inline Protocol::Message createMotionCommandWithParams(const CommandBits& command, 
                                                           const MotionParameters& params) {
        return Protocol::Message(Protocol::ModuleID::DS, 
                                Protocol::CommandCode::EXECUTE_COMMAND, 
                                {command.raw, params.raw});
    }
    
    // Запросить статус системы движения
    inline Protocol::Message createStatusRequest() {
        return Protocol::Message(Protocol::ModuleID::DS, 
                                Protocol::CommandCode::REQUEST_DATA);
    }
    
    // Создать ответ со статусом
    inline Protocol::Message createStatusResponse(Protocol::ModuleID target, 
                                                  const CompleteStatus& status) {
        return Protocol::Message(target, 
                                Protocol::CommandCode::DATA_RESPONSE, 
                                status.toDataBlocks());
    }
    
    // Парсинг ответа со статусом
    inline bool parseStatusResponse(const Protocol::Message& msg, CompleteStatus& status) {
        if (msg.getCommand() == Protocol::CommandCode::DATA_RESPONSE && 
            msg.data_blocks.size() >= 9) {
            return CompleteStatus::fromDataBlocks(msg.data_blocks, status);
        }
        return false;
    }
    
    // Создать уведомление об ошибке
    inline Protocol::Message createErrorNotification(Protocol::ModuleID target, ErrorCode error) {
        return Protocol::createErrorResponse(target, static_cast<uint8_t>(error));
    }
    
    // ===== Утилиты для работы с углами =====
    
    inline uint8_t angleToUint8(float angle_deg) {
        // Преобразование угла 0-360° в 0-255
        return static_cast<uint8_t>((angle_deg / 360.0f) * 255.0f);
    }
    
    inline float uint8ToAngle(uint8_t angle_byte) {
        // Преобразование 0-255 в угол 0-360°
        return (static_cast<float>(angle_byte) / 255.0f) * 360.0f;
    }
    
    inline uint8_t signedAngleToUint7(float angle_deg) {
        // Преобразование угла -64..+63° в 7 бит
        angle_deg = std::max(-64.0f, std::min(63.0f, angle_deg));
        return static_cast<uint8_t>(angle_deg + 64);
    }
    
    inline float uint7ToSignedAngle(uint8_t angle_byte) {
        // Преобразование 7 бит в угол -64..+63°
        return static_cast<float>(angle_byte & 0x7F) - 64.0f;
    }
    
} // namespace ISODF