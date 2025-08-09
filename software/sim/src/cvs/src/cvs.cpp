#include "cvs.hpp"
#include <iostream>

CVS::CVS() : BaseModule(Protocol::ModuleID::CVS, "CVS", Timing::CVS_FREQUENCY_HZ),
             m_system_state(SystemState::INITIALIZING),
             m_update_counter(0),
             m_system_uptime_ms(0),
             m_total_messages_sent(0),
             m_total_responses_received(0) {
    
    initializeModuleRegistry();
    m_power_manager = std::make_unique<ISES::PowerManager>();
    
    std::cout << "[CVS] Central Computing System created, known modules: " 
              << m_module_registry.size() << std::endl;
}

bool CVS::onInitialize() {
    std::cout << "[CVS] Initializing central computing system..." << std::endl;
    
    // CVS is always powered (master module)
    setPowered(true);
    
    m_system_state = SystemState::DISCOVERING;
    return true;
}

bool CVS::onStart() {
    std::cout << "[CVS] Starting central computing operations" << std::endl;
    
    m_system_state = SystemState::DISCOVERING;
    
    // Start module discovery
    discoverAllModules();
    
    return true;
}

void CVS::onStop() {
    std::cout << "[CVS] Central computing system stopped" << std::endl;
    m_system_state = SystemState::SHUTDOWN;
}

void CVS::onUpdate() {
    m_update_counter++;
    m_system_uptime_ms = m_update_counter * (1000.0 / Timing::CVS_FREQUENCY_HZ);
    
    // State machine update
    updateSystemState();
    
    // Ping modules every 100 updates (~2 seconds at 50Hz)
    if (m_update_counter % 100 == 0) {
        pingAllModules();
    }
    
    // Request data from modules every 50 updates (~1 second)
    if (m_update_counter % 50 == 0) {
        requestModuleData();
    }
    
    // Check timeouts every 25 updates (~0.5 seconds)
    if (m_update_counter % 25 == 0) {
        checkModuleTimeouts();
    }
    
    // Send demo commands every 200 updates (~4 seconds)
    if (m_update_counter % 200 == 0) {
        sendDemoCommands();
    }
    
    // Print system status every 500 updates (~10 seconds)
    if (m_update_counter % 500 == 0) {
        printSystemStatus();
    }
}

void CVS::onDataRequest(const Protocol::Message& message) {
    std::cout << "[CVS] Data request received (not implemented yet)" << std::endl;
    
    // CVS doesn't typically respond to data requests, but could send system status
    auto response = Protocol::createDataResponse(message.getTargetModule(), {});
    sendMessage(response);
}

void CVS::onCommandReceived(const Protocol::Message& message) {
    std::cout << "[CVS] Command received from module " << (int)message.getTargetModule() << std::endl;
    
    // Process commands to CVS (like system shutdown, mode changes, etc.)
    // For now, just acknowledge
    auto ack = Protocol::createCommandAck(message.getTargetModule(), true);
    sendMessage(ack);
}

void CVS::onResponseReceived(const Protocol::Message& message) {
    m_total_responses_received++;
    handleModuleResponse(message);
}

// Private methods implementation
void CVS::initializeModuleRegistry() {
    m_module_registry.emplace(Protocol::ModuleID::SES, ModuleInfo("Power Supply System"));
    m_module_registry.emplace(Protocol::ModuleID::DS, ModuleInfo("Drive System"));
    m_module_registry.emplace(Protocol::ModuleID::OS, ModuleInfo("Orientation System"));
    m_module_registry.emplace(Protocol::ModuleID::VIS, ModuleInfo("Vision System"));
}

void CVS::discoverAllModules() {
    std::cout << "[CVS] Starting module discovery..." << std::endl;
    
    for (auto& [module_id, info] : m_module_registry) {
        auto ping_msg = Protocol::createPingMessage(module_id);
        if (sendMessage(ping_msg)) {
            m_total_messages_sent++;
            std::cout << "[CVS] PING sent to " << info.name << std::endl;
        }
    }
}

void CVS::pingAllModules() {
    for (auto& [module_id, info] : m_module_registry) {
        auto ping_msg = Protocol::createPingMessage(module_id);
        if (sendMessage(ping_msg)) {
            m_total_messages_sent++;
            info.last_ping_time = m_system_uptime_ms;
        }
    }
}

void CVS::requestModuleData() {
    // Request data from SES (most important for system health)
    auto data_request = Protocol::createDataRequest(Protocol::ModuleID::SES);
    if (sendMessage(data_request)) {
        m_total_messages_sent++;
        std::cout << "[CVS] Data request sent to SES" << std::endl;
    }
}

void CVS::checkModuleTimeouts() {
    uint64_t current_time = m_system_uptime_ms;
    
    for (auto& [module_id, info] : m_module_registry) {
        uint64_t time_since_response = current_time - info.last_response_time;
        
        if (time_since_response > 5000 && info.is_alive) { // 5 second timeout
            info.is_alive = false;
            info.timeout_count++;
            std::cout << "[CVS] WARNING: Module " << info.name 
                      << " timeout (count: " << info.timeout_count << ")" << std::endl;
        }
    }
}

void CVS::handleModuleResponse(const Protocol::Message& message) {
    auto module_id = message.getTargetModule();
    auto it = m_module_registry.find(module_id);
    
    if (it != m_module_registry.end()) {
        it->second.is_alive = true;
        it->second.last_response_time = m_system_uptime_ms;
        
        std::cout << "[CVS] Response from " << it->second.name 
                  << " (cmd: " << (int)message.getCommand() << ")" << std::endl;
        
        // Handle specific response types
        switch (message.getCommand()) {
            case Protocol::CommandCode::PONG:
                std::cout << "[CVS] PONG received from " << it->second.name << std::endl;
                break;
                
            case Protocol::CommandCode::DATA_RESPONSE:
                if (module_id == Protocol::ModuleID::SES) {
                    handleSESStatus(message);
                }
                break;
                
            case Protocol::CommandCode::ERROR_RESPONSE:
                std::cout << "[CVS] ERROR reported by " << it->second.name << std::endl;
                break;
                
            default:
                break;
        }
    }
}

void CVS::handleSESStatus(const Protocol::Message& message) {
    if (message.data_blocks.size() >= 4) {
        ISES::CompleteStatus status;
        if (ISES::CompleteStatus::fromDataBlocks(message.data_blocks, status)) {
            double voltage = status.status.bits.voltage_mv / 1000.0;
            uint32_t charge = status.status.bits.charge_percent;
            double power = status.power.bits.power_mw / 1000.0;
            
            std::cout << "[CVS] SES Status: " << voltage << "V, " 
                      << charge << "%, " << power << "W, state=" 
                      << (int)status.status.bits.power_state << std::endl;
            
            // Check if we need to take action based on power status
            if (charge < 20) {
                std::cout << "[CVS] Low battery detected, consider power saving" << std::endl;
            }
        }
    }
}

void CVS::sendDemoCommands() {
    // Send power saving command to SES
    ISES::CommandBits power_cmd;
    
    // Alternate between enabling and disabling power saving
    if ((m_update_counter / 200) % 2 == 0) {
        power_cmd.bits.enable_power_saving = 1;
        std::cout << "[CVS] Sending command: Enable power saving" << std::endl;
    } else {
        power_cmd.bits.disable_power_saving = 1;
        std::cout << "[CVS] Sending command: Disable power saving" << std::endl;
    }
    
    auto command_msg = Protocol::createCommandRequest(Protocol::ModuleID::SES, {power_cmd.raw});
    if (sendMessage(command_msg)) {
        m_total_messages_sent++;
    }
}

void CVS::printSystemStatus() {
    std::cout << "\n[CVS] === System Status Report ===" << std::endl;
    std::cout << "[CVS] System state: " << (int)m_system_state << std::endl;
    std::cout << "[CVS]