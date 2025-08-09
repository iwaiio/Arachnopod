#include "ses.hpp"
#include <iostream>

PowerSupplySystem::PowerSupplySystem() 
    : BaseModule(Protocol::ModuleID::SES, "PowerSupplySystem", Timing::SES_FREQUENCY_HZ),
      m_battery_voltage(12.0),
      m_battery_charge_percent(100.0),
      m_power_consumption_mw(5000.0),  // 5W in mW
      m_current_consumption_ma(417.0), // 5W / 12V = 0.417A
      m_battery_temp(25.0),
      m_ambient_temp(22.0),
      m_pcb_temp(28.0),
      m_power_state(ISES::PowerState::NORMAL),
      m_last_error(ISES::ErrorCode::NONE),
      m_is_healthy(true),
      m_power_saving_enabled(false),
      m_update_counter(0) {
    
    m_power_manager = std::make_unique<ISES::PowerManager>();
    resetSystemData();
    
    std::cout << "[SES] Power Supply System created" << std::endl;
}

bool PowerSupplySystem::onInitialize() {
    std::cout << "[SES] Initializing power supply system..." << std::endl;
    
    // SES is always powered (it powers itself)
    setPowered(true);
    
    // Initialize data structures
    updateDataStructures();
    
    std::cout << "[SES] Power system initialized" << std::endl;
    return true;
}

bool PowerSupplySystem::onStart() {
    std::cout << "[SES] Starting power supply operations" << std::endl;
    std::cout << "[SES] Initial state:" << std::endl;
    std::cout << "[SES]   Voltage: " << m_battery_voltage << "V" << std::endl;
    std::cout << "[SES]   Charge: " << m_battery_charge_percent << "%" << std::endl;
    std::cout << "[SES]   Power: " << (m_power_consumption_mw/1000.0) << "W" << std::endl;
    std::cout << "[SES]   Temperature: " << m_battery_temp << "C" << std::endl;
    
    return true;
}

void PowerSupplySystem::onStop() {
    std::cout << "[SES] Power supply system stopped" << std::endl;
}

void PowerSupplySystem::onPowerOn() {
    std::cout << "[SES] Power subsystem activated" << std::endl;
    resetSystemData();
}

void PowerSupplySystem::onPowerOff() {
    std::cout << "[SES] Power subsystem deactivated" << std::endl;
    // Note: SES normally can't be powered off (it powers itself)
}

void PowerSupplySystem::onUpdate() {
    if (!isPowered()) return;
    
    m_update_counter++;
    
    // Update battery simulation every cycle
    updateBatterySimulation();
    
    // Update temperatures less frequently (every 10 cycles)
    if (m_update_counter % 10 == 0) {
        updateTemperatures();
    }
    
    // Update data structures every cycle
    updateDataStructures();
    
    // Check critical conditions every 5 cycles
    if (m_update_counter % 5 == 0) {
        checkCriticalConditions();
    }
    
    // Detailed status every 50 cycles
    if (m_update_counter % 50 == 0) {
        std::cout << "[SES] Status: " << m_battery_voltage << "V, " 
                  << m_battery_charge_percent << "%, " 
                  << (m_power_consumption_mw/1000.0) << "W, " 
                  << m_battery_temp << "C";
        if (m_power_saving_enabled) std::cout << " (power saving)";
        std::cout << std::endl;
    }
}

void PowerSupplySystem::onDataRequest(const Protocol::Message& message) {
    std::cout << "[SES] Data request received, sending complete status" << std::endl;
    sendCompleteStatus(static_cast<Protocol::ModuleID>(Protocol::ModuleID::CVS));
}

void PowerSupplySystem::onCommandReceived(const Protocol::Message& message) {
    if (message.data_blocks.empty()) {
        std::cout << "[SES] Empty command received" << std::endl;
        
        // Send NACK
        auto nack = Protocol::createCommandAck(static_cast<Protocol::ModuleID>(Protocol::ModuleID::CVS), false);
        sendMessage(nack);
        return;
    }
    
    // Decode command using interface
    ISES::CommandBits command;
    command.raw = message.data_blocks[0];
    
    std::cout << "[SES] Command received, raw: 0x" << std::hex << command.raw.raw << std::dec << std::endl;
    
    // Process command
    handlePowerCommand(command);
    
    // Send ACK
    auto ack = Protocol::createCommandAck(static_cast<Protocol::ModuleID>(Protocol::ModuleID::CVS), true);
    sendMessage(ack);
}

void PowerSupplySystem::onResponseReceived(const Protocol::Message& message) {
    std::cout << "[SES] Response received: " << (int)message.getCommand() << std::endl;
}

// Private methods implementation
void PowerSupplySystem::updateBatterySimulation() {
    // Simple battery discharge simulation
    double discharge_rate = (m_power_consumption_mw / 1000.0) / (5.0 * 3600.0); // 5Ah battery
    m_battery_charge_percent -= discharge_rate * (1.0 / Timing::SES_FREQUENCY_HZ) * 100.0;
    
    // Clamp charge
    if (m_battery_charge_percent < 0) m_battery_charge_percent = 0;
    if (m_battery_charge_percent > 100) m_battery_charge_percent = 100;
    
    // Voltage drops with discharge (simplified)
    m_battery_voltage = 10.8 + (m_battery_charge_percent / 100.0) * 1.2;
    
    // Current based on power and voltage
    m_current_consumption_ma = (m_power_consumption_mw / m_battery_voltage);
}

void PowerSupplySystem::updateTemperatures() {
    // Battery temperature increases with current
    double target_battery_temp = 25.0 + (m_current_consumption_ma / 1000.0) * 10.0;
    m_battery_temp += (target_battery_temp - m_battery_temp) * 0.1;
    
    // Ambient temperature simulation (slight variation)
    m_ambient_temp = 22.0 + 3.0 * std::sin(m_update_counter * 0.01);
    
    // PCB temperature between ambient and battery
    m_pcb_temp = (m_ambient_temp + m_battery_temp) / 2.0 + 3.0;
}

void PowerSupplySystem::updateDataStructures() {
    // Update status data
    m_status_data.bits.voltage_mv = static_cast<uint32_t>(m_battery_voltage * 1000);
    m_status_data.bits.charge_percent = static_cast<uint32_t>(m_battery_charge_percent);
    m_status_data.bits.power_state = static_cast<uint32_t>(m_power_state);
    m_status_data.bits.error_code = static_cast<uint32_t>(m_last_error);
    m_status_data.bits.is_healthy = m_is_healthy ? 1 : 0;
    m_status_data.bits.temp_valid = 1;
    
    // Update power data
    m_power_data.bits.current_ma = static_cast<uint32_t>(m_current_consumption_ma);
    m_power_data.bits.power_mw = static_cast<uint32_t>(m_power_consumption_mw);
    
    // Update temperature data (offset by +50 for negative temps)
    m_temp_data.bits.battery_temp = static_cast<uint32_t>(m_battery_temp + 50);
    m_temp_data.bits.ambient_temp = static_cast<uint32_t>(m_ambient_temp + 50);
    m_temp_data.bits.pcb_temp = static_cast<uint32_t>(m_pcb_temp + 50);
    
    // Update module power status
    m_module_power_data = m_power_manager->getPowerStatus();
}

void PowerSupplySystem::checkCriticalConditions() {
    ISES::ErrorCode new_error = ISES::ErrorCode::NONE;
    ISES::PowerState new_state = ISES::PowerState::NORMAL;
    
    // Check battery levels
    if (m_battery_charge_percent < 10.0) {
        new_error = ISES::ErrorCode::CRITICAL_BATTERY;
        new_state = ISES::PowerState::CRITICAL_BATTERY;
        std::cout << "[SES] CRITICAL: Battery critically low!" << std::endl;
    } else if (m_battery_charge_percent < 20.0) {
        new_error = ISES::ErrorCode::LOW_BATTERY;
        new_state = ISES::PowerState::LOW_BATTERY;
        std::cout << "[SES] WARNING: Battery low" << std::endl;
    }
    
    // Check temperature
    if (m_battery_temp > 60.0) {
        new_error = ISES::ErrorCode::OVERHEAT;
        std::cout << "[SES] WARNING: Overheating detected" << std::endl;
    }
    
    // Check overcurrent
    if (m_current_consumption_ma > 2000.0) { // 2A limit
        new_error = ISES::ErrorCode::OVERCURRENT;
        std::cout << "[SES] WARNING: Overcurrent detected" << std::endl;
    }
    
    // Update state if changed
    if (new_state != m_power_state) {
        m_power_state = new_state;
    }
    
    // Send error notification if new error
    if (new_error != ISES::ErrorCode::NONE && new_error != m_last_error) {
        m_last_error = new_error;
        sendErrorNotification(new_error);
    }
}

void PowerSupplySystem::handlePowerCommand(const ISES::CommandBits& command) {
    std::cout << "[SES] Processing power command..." << std::endl;
    
    // Handle power saving commands
    if (command.bits.enable_power_saving) {
        applyPowerSavingMode(true);
        std::cout << "[SES] Power saving mode enabled" << std::endl;
    }
    
    if (command.bits.disable_power_saving) {
        applyPowerSavingMode(false);
        std::cout << "[SES] Power saving mode disabled" << std::endl;
    }
    
    // Handle module power control
    m_power_manager->applyPowerCommand(command);
    
    // Handle emergency shutdown
    if (command.bits.emergency_shutdown) {
        m_power_state = ISES::PowerState::EMERGENCY_SHUTDOWN;
        m_is_healthy = false;
        std::cout << "[SES] EMERGENCY SHUTDOWN initiated!" << std::endl;
    }
    
    // Handle system reset
    if (command.bits.reset_system) {
        resetSystemData();
        std::cout << "[SES] System reset performed" << std::endl;
    }
}

void PowerSupplySystem::sendCompleteStatus(Protocol::ModuleID target) {
    ISES::CompleteStatus status;
    status.status = m_status_data;
    status.power = m_power_data;
    status.temperature = m_temp_data;
    status.module_power = m_module_power_data;
    
    auto data_blocks = status.toDataBlocks();
    auto response = Protocol::createDataResponse(target, data_blocks);
    
    sendMessage(response);
    std::cout << "[SES] Complete status sent to module " << (int)target << std::endl;
}

void PowerSupplySystem::sendErrorNotification(ISES::ErrorCode error) {
    auto error_response = Protocol::createErrorResponse(
        static_cast<Protocol::ModuleID>(Protocol::ModuleID::CVS), 
        static_cast<uint8_t>(error));
    
    sendMessage(error_response);
    std::cout << "[SES] Error notification sent: " << (int)error << std::endl;
}

void PowerSupplySystem::applyPowerSavingMode(bool enabled) {
    m_power_saving_enabled = enabled;
    
    if (enabled) {
        m_power_consumption_mw *= 0.7; // Reduce by 30%
        m_power_state = ISES::PowerState::POWER_SAVING;
    } else {
        m_power_consumption_mw = 5000.0; // Reset to 5W
        m_power_state = ISES::PowerState::NORMAL;
    }
}

void PowerSupplySystem::resetSystemData() {
    m_battery_voltage = 12.0;
    m_battery_charge_percent = 100.0;
    m_power_consumption_mw = 5000.0;
    m_current_consumption_ma = 417.0;
    m_battery_temp = 25.0;
    m_ambient_temp = 22.0;
    m_pcb_temp = 28.0;
    m_power_state = ISES::PowerState::NORMAL;
    m_last_error = ISES::ErrorCode::NONE;
    m_is_healthy = true;
    m_power_saving_enabled = false;
    m_update_counter = 0;
    
    std::cout << "[SES] System data reset to defaults" << std::endl;
}