#pragma once

#include "imodule.hpp"

// ===================================================================
// POWER SUPPLY SYSTEM MODULE
// Модуль системы энергоснабжения (SES) для робота Arachnopod
// ===================================================================

// Предварительное объявление пространства имен
namespace ISES {
    enum class PowerState : uint8_t;
    enum class ErrorCode : uint8_t;
    union CommandBits;
    union StatusData;
    union PowerData;
    union TemperatureData;
    union ModulePowerStatus;
    union BatteryExtendedData;
    struct CompleteStatus;
}

class PowerSupplySystem : public BaseModule {
private:
    // === Состояние батареи и системы питания ===
    
    // Основные параметры батареи
    double m_battery_voltage;           // Напряжение батареи в вольтах
    double m_battery_charge_percent;    // Процент заряда батареи (0-100)
    double m_battery_capacity_mah;      // Емкость батареи в мАч
    double m_battery_health_percent;    // Здоровье батареи (0-100)
    uint32_t m_battery_cycles;          // Количество циклов заряда/разряда
    
    // Параметры потребления
    double m_power_consumption_mw;      // Общее потребление в милливаттах
    double m_current_consumption_ma;    // Общий потребляемый ток в миллиамперах
    double m_peak_power_mw;             // Пиковое потребление за период
    double m_average_power_mw;          // Среднее потребление за период
    
    // Температурные параметры
    double m_battery_temp;              // Температура батареи в °C
    double m_ambient_temp;              // Температура окружающей среды в °C
    double m_pcb_temp;                  // Температура платы управления в °C
    double m_max_module_temp;           // Максимальная температура среди модулей в °C
    
    // === Состояние системы ===
    
    ISES::PowerState* m_power_state;    // Текущее состояние системы питания
    ISES::ErrorCode* m_last_error;      // Последняя зарегистрированная ошибка
    bool m_is_healthy;                  // Общее состояние системы (true = исправна)
    bool m_power_saving_enabled;        // Режим энергосбережения активен
    bool m_is_charging;                 // Идет процесс зарядки
    bool m_is_balancing;                // Идет балансировка ячеек
    
    // === Управление питанием модулей ===
    
    struct ModulePowerInfo {
        bool is_powered;                // Модуль включен
        bool has_fault;                 // Есть ошибка питания
        double power_consumption_mw;    // Потребление модуля в мВт
        uint64_t last_power_change_ms;  // Время последнего изменения состояния
    };
    
    std::unordered_map<Protocol::ModuleID, ModulePowerInfo> m_module_power_states;
    
    // === Внутренние структуры данных для протокола ===
    
    ISES::StatusData* m_status_data;
    ISES::PowerData* m_power_data;
    ISES::TemperatureData* m_temp_data;
    ISES::ModulePowerStatus* m_module_power_data;
    ISES::BatteryExtendedData* m_battery_extended_data;
    
    // === Счетчики и статистика ===
    
    uint32_t m_update_counter;          // Счетчик циклов обновления
    uint64_t m_total_energy_consumed_mwh; // Общее потребление энергии в мВт*ч
    uint32_t m_error_count;             // Количество зарегистрированных ошибок
    uint32_t m_commands_received;       // Количество полученных команд
    uint32_t m_status_requests_served;  // Количество обслуженных запросов статуса
    
    // === Параметры симуляции ===
    
    double m_discharge_rate_per_hour;   // Скорость разряда в % в час
    double m_charge_rate_per_hour;      // Скорость заряда в % в час
    double m_temp_rise_rate;            // Скорость нагрева при нагрузке
    bool m_simulate_battery_aging;      // Симулировать старение батареи
    
public:
    PowerSupplySystem();
    virtual ~PowerSupplySystem();

protected:
    // === Переопределение методов BaseModule ===
    
    bool onInitialize() override;
    bool onStart() override;
    void onStop() override;
    void onPowerOn() override;
    void onPowerOff() override;
    void onUpdate() override;
    
    // === Обработка сообщений ===
    
    void onDataRequest(const Protocol::Message& message) override;
    void onCommandReceived(const Protocol::Message& message) override;
    void onResponseReceived(const Protocol::Message& message) override;

private:
    // === Методы симуляции батареи ===
    
    void updateBatterySimulation();
    void simulateBatteryDischarge();
    void simulateBatteryCharge();
    void updateBatteryHealth();
    
    // === Методы управления температурой ===
    
    void updateTemperatures();
    void calculateThermalLoad();
    void applyThermalDissipation();
    
    // === Методы управления питанием модулей ===
    
    void updateModulePowerStates();
    void powerOnModule(Protocol::ModuleID module);
    void powerOffModule(Protocol::ModuleID module);
    void calculateTotalPowerConsumption();
    
    // === Методы обновления данных ===
    
    void updateDataStructures();
    void updateStatusData();
    void updatePowerData();
    void updateTemperatureData();
    void updateModulePowerData();
    void updateBatteryExtendedData();
    
    // === Методы проверки состояния ===
    
    void checkCriticalConditions();
    void checkBatteryLevel();
    void checkTemperatureLimits();
    void checkCurrentLimits();
    void checkModuleFaults();
    
    // === Методы обработки команд ===
    
    void handlePowerCommand(const ISES::CommandBits& command);
    void handlePowerSavingCommand(bool enable);
    void handleModulePowerCommand(const ISES::CommandBits& command);
    void handleSystemReset();
    void handleEmergencyShutdown();
    void handleCalibration();
    
    // === Методы отправки данных ===
    
    void sendCompleteStatus(Protocol::ModuleID target);
    void sendErrorNotification(ISES::ErrorCode error);
    void sendPowerChangeNotification(Protocol::ModuleID module, bool powered);
    
    // === Вспомогательные методы ===
    
    void applyPowerSavingMode(bool enabled);
    void resetSystemData();
    void logSystemStatus();
    double calculateEfficiency() const;
    double estimateRemainingTime() const;
    
    // === Методы для работы с конфигурацией ===
    
    void loadConfiguration();
    void saveStatistics();
};