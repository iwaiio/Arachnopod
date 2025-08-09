// sodf.hpp
#pragma once

#include "imodule.hpp"

// ===================================================================
// DRIVE SYSTEM MODULE (SODF - Система Обеспечения Двигательных Функций)
// Минимальная реализация для демонстрации протокола
// ===================================================================

class DriveSystem : public BaseModule {
private:
    // Состояние приводов
    struct ServoState {
        double position_deg;      // Позиция в градусах
        double target_position;   // Целевая позиция
        double speed_deg_per_sec; // Скорость движения
        double current_ma;        // Потребляемый ток
        bool is_moving;          // Движется ли привод
        bool has_error;          // Есть ли ошибка
    };
    
    // 6 ног * 3 сервопривода на ногу = 18 приводов
    static constexpr size_t NUM_LEGS = 6;
    static constexpr size_t SERVOS_PER_LEG = 3;
    static constexpr size_t TOTAL_SERVOS = NUM_LEGS * SERVOS_PER_LEG;
    
    std::array<ServoState, TOTAL_SERVOS> m_servos;
    
    // Общее состояние системы движения
    enum class MotionState {
        IDLE,        // Покой
        STANDING,    // Стоит
        WALKING,     // Идет
        TURNING,     // Поворачивается
        CALIBRATING  // Калибровка
    } m_motion_state;
    
    double m_total_current_consumption_ma;
    uint32_t m_update_counter;
    
public:
    DriveSystem();
    virtual ~DriveSystem() = default;

protected:
    bool onInitialize() override;
    bool onStart() override;
    void onStop() override;
    void onUpdate() override;
    
    void onDataRequest(const Protocol::Message& message) override;
    void onCommandReceived(const Protocol::Message& message) override;
    void onResponseReceived(const Protocol::Message& message) override;

private:
    void updateServoSimulation();
    void sendSystemStatus(Protocol::ModuleID target);
};

// ===================================================================
// IMPLEMENTATION
// ===================================================================

#ifdef DS_IMPLEMENTATION // Обычно в .cpp файле

#include <iostream>
#include <cmath>

DriveSystem::DriveSystem() 
    : BaseModule(Protocol::ModuleID::DS, "DriveSystem", Timing::DS_FREQUENCY_HZ),
      m_motion_state(MotionState::IDLE),
      m_total_current_consumption_ma(0),
      m_update_counter(0) {
    
    // Инициализация сервоприводов
    for (auto& servo : m_servos) {
        servo.position_deg = 0.0;
        servo.target_position = 0.0;
        servo.speed_deg_per_sec = 60.0; // 60 градусов в секунду
        servo.current_ma = 50.0; // Базовое потребление в покое
        servo.is_moving = false;
        servo.has_error = false;
    }
    
    std::cout << "[DS] Drive System created with " << TOTAL_SERVOS << " servos" << std::endl;
}

bool DriveSystem::onInitialize() {
    std::cout << "[DS] Initializing drive system..." << std::endl;
    
    // DS начинает выключенным (включается по команде от CVS через SES)
    setPowered(false);
    
    std::cout << "[DS] Drive system initialized (awaiting power-on)" << std::endl;
    return true;
}

bool DriveSystem::onStart() {
    std::cout << "[DS] Starting drive system operations" << std::endl;
    m_motion_state = MotionState::IDLE;
    return true;
}

void DriveSystem::onStop() {
    std::cout << "[DS] Drive system stopped" << std::endl;
    
    // Остановить все приводы
    for (auto& servo : m_servos) {
        servo.is_moving = false;
        servo.target_position = servo.position_deg;
    }
}

void DriveSystem::onUpdate() {
    if (!isPowered()) {
        // Система выключена, не обновляем
        return;
    }
    
    m_update_counter++;
    
    // Обновление симуляции сервоприводов
    updateServoSimulation();
    
    // Периодический вывод состояния
    if (m_update_counter % 100 == 0) { // Каждую секунду при 100 Hz
        std::cout << "[DS] State: " << (int)m_motion_state 
                  << ", Power: " << (m_total_current_consumption_ma / 1000.0) << "A" 
                  << ", Active servos: ";
        
        int active_count = 0;
        for (const auto& servo : m_servos) {
            if (servo.is_moving) active_count++;
        }
        std::cout << active_count << "/" << TOTAL_SERVOS << std::endl;
    }
}

void DriveSystem::onDataRequest(const Protocol::Message& message) {
    std::cout << "[DS] Data request received" << std::endl;
    sendSystemStatus(Protocol::ModuleID::CVS);
}

void DriveSystem::onCommandReceived(const Protocol::Message& message) {
    if (message.data_blocks.empty()) {
        auto nack = Protocol::createCommandAck(Protocol::ModuleID::CVS, false);
        sendMessage(nack);
        return;
    }
    
    // Простая обработка команд
    uint32_t command = message.data_blocks[0].raw;
    std::cout << "[DS] Command received: 0x" << std::hex << command << std::dec << std::endl;
    
    // Биты команд:
    // bit 0: Start walking
    // bit 1: Stop
    // bit 2: Turn left
    // bit 3: Turn right
    // bit 4: Stand up
    // bit 5: Sit down
    // bit 6: Calibrate
    
    if (command & 0x01) { // Start walking
        m_motion_state = MotionState::WALKING;
        std::cout << "[DS] Starting walking sequence" << std::endl;
        
        // Активируем некоторые сервоприводы для симуляции движения
        for (size_t i = 0; i < TOTAL_SERVOS; i += 3) {
            m_servos[i].target_position = 30.0;
            m_servos[i].is_moving = true;
        }
    }
    
    if (command & 0x02) { // Stop
        m_motion_state = MotionState::IDLE;
        std::cout << "[DS] Stopping all movement" << std::endl;
        
        for (auto& servo : m_servos) {
            servo.is_moving = false;
            servo.target_position = servo.position_deg;
        }
    }
    
    if (command & 0x04) { // Turn left
        m_motion_state = MotionState::TURNING;
        std::cout << "[DS] Turning left" << std::endl;
    }
    
    if (command & 0x08) { // Turn right
        m_motion_state = MotionState::TURNING;
        std::cout << "[DS] Turning right" << std::endl;
    }
    
    if (command & 0x10) { // Stand up
        m_motion_state = MotionState::STANDING;
        std::cout << "[DS] Standing up" << std::endl;
        
        // Все сервоприводы в нейтральное положение
        for (auto& servo : m_servos) {
            servo.target_position = 0.0;
            servo.is_moving = true;
        }
    }
    
    if (command & 0x40) { // Calibrate
        m_motion_state = MotionState::CALIBRATING;
        std::cout << "[DS] Starting calibration" << std::endl;
    }
    
    // Отправляем подтверждение
    auto ack = Protocol::createCommandAck(Protocol::ModuleID::CVS, true);
    sendMessage(ack);
}

void DriveSystem::onResponseReceived(const Protocol::Message& message) {
    // DS обычно не получает ответы, только команды
    std::cout << "[DS] Response received (unexpected)" << std::endl;
}

void DriveSystem::updateServoSimulation() {
    m_total_current_consumption_ma = 0;
    
    double dt = 1.0 / Timing::DS_FREQUENCY_HZ; // Время одного цикла
    
    for (auto& servo : m_servos) {
        if (servo.is_moving) {
            // Движение к целевой позиции
            double error = servo.target_position - servo.position_deg;
            
            if (std::abs(error) > 0.5) { // Порог достижения цели
                double step = servo.speed_deg_per_sec * dt;
                
                if (error > 0) {
                    servo.position_deg += std::min(step, error);
                } else {
                    servo.position_deg -= std::min(step, -error);
                }
                
                // Потребление тока при движении
                servo.current_ma = 200.0; // 200 мА при движении
            } else {
                // Достигли целевой позиции
                servo.position_deg = servo.target_position;
                servo.is_moving = false;
                servo.current_ma = 50.0; // 50 мА в покое
            }
        } else {
            servo.current_ma = 50.0; // Базовое потребление
        }
        
        m_total_current_consumption_ma += servo.current_ma;
    }
    
    // Симуляция паттернов движения
    if (m_motion_state == MotionState::WALKING) {
        // Простая волна движения для имитации ходьбы
        static double walking_phase = 0.0;
        walking_phase += 2.0 * dt; // 2 радиана в секунду
        
        for (size_t leg = 0; leg < NUM_LEGS; ++leg) {
            double leg_phase = walking_phase + (leg * M_PI / 3.0); // Сдвиг фазы между ногами
            
            size_t base_idx = leg * SERVOS_PER_LEG;
            m_servos[base_idx].target_position = 20.0 * std::sin(leg_phase);
            m_servos[base_idx].is_moving = true;
            
            m_servos[base_idx + 1].target_position = 15.0 * std::cos(leg_phase);
            m_servos[base_idx + 1].is_moving = true;
        }
    }
}

void DriveSystem::sendSystemStatus(Protocol::ModuleID target) {
    // Формируем статус системы движения
    // Блок 0: Общее состояние
    Protocol::DataBlock status_block;
    status_block.bits.bit0 = (m_motion_state == MotionState::IDLE) ? 1 : 0;
    status_block.bits.bit1 = (m_motion_state == MotionState::STANDING) ? 1 : 0;
    status_block.bits.bit2 = (m_motion_state == MotionState::WALKING) ? 1 : 0;
    status_block.bits.bit3 = (m_motion_state == MotionState::TURNING) ? 1 : 0;
    status_block.bits.bit4 = (m_motion_state == MotionState::CALIBRATING) ? 1 : 0;
    status_block.bits.bit5 = isPowered() ? 1 : 0;
    
    // Счетчик активных сервоприводов
    uint8_t active_servos = 0;
    uint8_t error_servos = 0;
    for (const auto& servo : m_servos) {
        if (servo.is_moving) active_servos++;
        if (servo.has_error) error_servos++;
    }
    status_block.bytes.byte1 = active_servos;
    status_block.bytes.byte2 = error_servos;
    status_block.bytes.byte3 = TOTAL_SERVOS;
    
    // Блок 1: Потребление энергии
    Protocol::DataBlock power_block;
    power_block.words.word0 = static_cast<uint16_t>(m_total_current_consumption_ma);
    power_block.words.word1 = static_cast<uint16_t>(m_total_current_consumption_ma * 12 / 1000); // мВт при 12В
    
    // Отправляем ответ
    std::vector<Protocol::DataBlock> data = {status_block, power_block};
    auto response = Protocol::createDataResponse(target, data);
    sendMessage(response);
    
    std::cout << "[DS] Status sent: state=" << (int)m_motion_state 
              << ", active=" << (int)active_servos 
              << ", power=" << (m_total_current_consumption_ma/1000.0) << "A" << std::endl;
}

#endif // DS_IMPLEMENTATION