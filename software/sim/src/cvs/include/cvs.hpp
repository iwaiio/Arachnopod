#pragma once

#include "imodule.hpp"
#include <unordered_map>
#include <memory>

// ===================================================================
// CENTRAL COMPUTING SYSTEM MODULE
// Центральная вычислительная система (CVS) робота Arachnopod
// ===================================================================

// Предварительные объявления
namespace ISES {
    union CommandBits;
    struct CompleteStatus;
}

class CVS : public BaseModule {
private:
    // === Информация о модулях системы ===
    
    struct ModuleInfo {
        std::string name;                  // Человекочитаемое имя модуля
        bool is_alive;                     // Модуль отвечает на запросы
        bool is_powered;                   // Модуль включен (по данным SES)
        uint64_t last_ping_time_ms;        // Время последнего ping запроса
        uint64_t last_response_time_ms;    // Время последнего ответа
        uint32_t ping_count;               // Количество отправленных ping
        uint32_t response_count;           // Количество полученных ответов
        uint32_t timeout_count;            // Количество таймаутов
        uint32_t error_count;              // Количество ошибок
        double average_response_time_ms;   // Среднее время ответа
        
        ModuleInfo(const std::string& module_name) 
            : name(module_name), 
              is_alive(false), 
              is_powered(false),
              last_ping_time_ms(0), 
              last_response_time_ms(0),
              ping_count(0),
              response_count(0),
              timeout_count(0),
              error_count(0),
              average_response_time_ms(0.0) {}
    };
    
    std::unordered_map<Protocol::ModuleID, ModuleInfo> m_module_registry;
    
    // === Состояние системы ===
    
    enum class SystemState {
        INITIALIZING,      // Инициализация системы
        DISCOVERING,       // Обнаружение модулей
        CONFIGURING,       // Конфигурирование модулей
        ACTIVE,           // Активная работа
        DEGRADED,         // Работа с ограничениями (некоторые модули недоступны)
        ERROR_RECOVERY,   // Восстановление после ошибки
        EMERGENCY,        // Аварийный режим
        SHUTDOWN          // Завершение работы
    };
    
    SystemState m_system_state;
    SystemState m_previous_state;
    
    // === Управление миссией/задачами ===
    
    enum class MissionState {
        IDLE,             // Ожидание задачи
        PLANNING,         // Планирование маршрута/действий
        EXECUTING,        // Выполнение задачи
        PAUSED,          // Задача приостановлена
        COMPLETED,       // Задача выполнена
        ABORTED          // Задача прервана
    };
    
    MissionState m_mission_state;
    std::string m_current_mission_name;
    uint32_t m_mission_progress_percent;
    
    // === Счетчики и статистика ===
    
    uint32_t m_update_counter;
    uint64_t m_system_uptime_ms;
    uint32_t m_total_messages_sent;
    uint32_t m_total_responses_received;
    uint32_t m_total_errors;
    uint32_t m_total_commands_sent;
    
    // === Данные о системе питания ===
    
    struct PowerSystemStatus {
        double battery_voltage;
        double battery_charge_percent;
        double power_consumption_w;
        double remaining_time_hours;
        bool power_saving_active;
        bool is_critical;
        uint32_t last_update_ms;
    } m_power_status;
    
    // === Параметры управления ===
    
    struct SystemParameters {
        uint32_t module_ping_interval_ms;      // Интервал опроса модулей
        uint32_t data_request_interval_ms;     // Интервал запроса данных
        uint32_t timeout_check_interval_ms;    // Интервал проверки таймаутов
        uint32_t status_report_interval_ms;    // Интервал отчета о состоянии
        uint32_t module_timeout_threshold_ms;  // Порог таймаута модуля
        uint32_t max_retry_count;              // Максимальное количество повторов
        bool auto_recovery_enabled;            // Автоматическое восстановление
        bool verbose_logging;                  // Подробное логирование
    } m_parameters;
    
    // === Флаги состояния ===
    
    bool m_emergency_stop_requested;
    bool m_power_critical_mode;
    bool m_all_modules_discovered;
    bool m_system_ready;
    
public:
    CVS();
    virtual ~CVS();

protected:
    // === Переопределение методов BaseModule ===
    
    bool onInitialize() override;
    bool onStart() override;
    void onStop() override;
    void onUpdate() override;
    
    // === Обработка сообщений ===
    
    void onDataRequest(const Protocol::Message& message) override;
    void onCommandReceived(const Protocol::Message& message) override;
    void onResponseReceived(const Protocol::Message& message) override;

private:
    // === Методы управления системой ===
    
    void updateSystemState();
    void handleStateTransition(SystemState new_state);
    void performSystemHealthCheck();
    void handleEmergencyConditions();
    
    // === Методы работы с модулями ===
    
    void initializeModuleRegistry();
    void discoverAllModules();
    void pingModule(Protocol::ModuleID module);
    void pingAllModules();
    void requestModuleData(Protocol::ModuleID module);
    void requestAllModuleData();
    void checkModuleTimeouts();
    void updateModuleStatistics(Protocol::ModuleID module, uint64_t response_time);
    
    // === Методы обработки ответов ===
    
    void handleModuleResponse(const Protocol::Message& message);
    void handlePongResponse(const Protocol::Message& message);
    void handleDataResponse(const Protocol::Message& message);
    void handleErrorResponse(const Protocol::Message& message);
    void handleSESStatus(const Protocol::Message& message);
    
    // === Методы управления питанием ===
    
    void sendPowerCommand(const ISES::CommandBits& command);
    void requestPowerStatus();
    void handleLowBattery();
    void enablePowerSaving();
    void disablePowerSaving();
    void powerOnModule(Protocol::ModuleID module);
    void powerOffModule(Protocol::ModuleID module);
    
    // === Методы управления миссией ===
    
    void startMission(const std::string& mission_name);
    void pauseMission();
    void resumeMission();
    void abortMission();
    void updateMissionProgress();
    
    // === Демонстрационные и тестовые методы ===
    
    void sendDemoCommands();
    void testModuleCommunication();
    void simulateEmergency();
    
    // === Методы отчетности и логирования ===
    
    void printSystemStatus();
    void printModuleStatus();
    void printPowerStatus();
    void printMissionStatus();
    void logEvent(const std::string& event);
    void logError(const std::string& error);
    
    // === Вспомогательные методы ===
    
    void loadSystemConfiguration();
    void saveSystemState();
    bool isModuleEssential(Protocol::ModuleID module) const;
    bool canOperateWithoutModule(Protocol::ModuleID module) const;
    std::string getModuleName(Protocol::ModuleID module) const;
    std::string getSystemStateString() const;
    std::string getMissionStateString() const;
};