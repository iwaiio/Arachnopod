#include "imodule.hpp"
#include <iostream>

// Static member initialization
uint64_t BaseModule::s_next_request_id = 1;

BaseModule::BaseModule(Protocol::ModuleID id, const std::string& name, double frequency_hz)
    : m_module_id(id), m_module_name(name), m_timer(frequency_hz) {
    std::cout << "[" << m_module_name << "] Module created (ID: " << (int)m_module_id 
              << ", freq: " << frequency_hz << " Hz)" << std::endl;
}

BaseModule::~BaseModule() {
    stop();
    std::cout << "[" << m_module_name << "] Module destroyed" << std::endl;
}

Protocol::ModuleID BaseModule::getModuleID() const {
    return m_module_id;
}

std::string BaseModule::getModuleName() const {
    return m_module_name;
}

bool BaseModule::initialize(std::shared_ptr<IBus> bus) {
    std::cout << "[" << m_module_name << "] Initializing..." << std::endl;
    
    m_bus = bus;
    if (m_bus) {
        m_bus->subscribe(m_module_id, 
            [this](const Protocol::Message& msg) { 
                this->enqueueMessage(msg); 
            });
        
        if (onInitialize()) {
            std::cout << "[" << m_module_name << "] Initialization completed" << std::endl;
            return true;
        }
    }
    
    std::cout << "[" << m_module_name << "] ERROR: Initialization failed" << std::endl;
    return false;
}

bool BaseModule::start() {
    if (m_is_running) return true;
    
    std::cout << "[" << m_module_name << "] Starting..." << std::endl;
    
    if (onStart()) {
        m_is_running = true;
        m_worker_thread = std::thread(&BaseModule::workerThreadFunction, this);
        std::cout << "[" << m_module_name << "] Started successfully" << std::endl;
        return true;
    }
    
    std::cout << "[" << m_module_name << "] ERROR: Start failed" << std::endl;
    return false;
}

void BaseModule::stop() {
    if (!m_is_running) return;
    
    std::cout << "[" << m_module_name << "] Stopping..." << std::endl;
    
    m_is_running = false;
    
    if (m_worker_thread.joinable()) {
        m_worker_thread.join();
    }
    
    if (m_bus) {
        m_bus->unsubscribe(m_module_id);
    }
    
    onStop();
    std::cout << "[" << m_module_name << "] Stopped" << std::endl;
}

void BaseModule::handleMessage(const Protocol::Message& message) {
    // This is called by the bus, so we just enqueue the message
    enqueueMessage(message);
}

bool BaseModule::isPowered() const {
    return m_is_powered.load();
}

void BaseModule::setPowered(bool powered) {
    bool was_powered = m_is_powered.exchange(powered);
    
    if (powered && !was_powered) {
        std::cout << "[" << m_module_name << "] Power ON" << std::endl;
        onPowerOn();
    } else if (!powered && was_powered) {
        std::cout << "[" << m_module_name << "] Power OFF" << std::endl;
        onPowerOff();
    }
}

void BaseModule::onPingReceived(const Protocol::Message& message) {
    // Send PONG response
    auto pong = Protocol::createPongMessage(m_module_id);
    sendMessage(pong);
    std::cout << "[" << m_module_name << "] PONG sent" << std::endl;
}

bool BaseModule::sendMessage(const Protocol::Message& message) {
    if (m_bus) {
        return m_bus->sendMessage(message);
    }
    
    std::cout << "[" << m_module_name << "] ERROR: No bus connection" << std::endl;
    return false;
}

bool BaseModule::sendResponse(Protocol::ModuleID target, Protocol::CommandCode response_type, 
                             const std::vector<Protocol::DataBlock>& data) {
    Protocol::Message response(target, response_type, data);
    return sendMessage(response);
}

void BaseModule::enqueueMessage(const Protocol::Message& message) {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_message_queue.push(message);
}

void BaseModule::processMessageQueue() {
    std::queue<Protocol::Message> temp_queue;
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        temp_queue = std::move(m_message_queue);
        m_message_queue = std::queue<Protocol::Message>(); // Clear
    }
    
    while (!temp_queue.empty()) {
        const auto& message = temp_queue.front();
        
        // Route message based on command type
        switch (message.getCommand()) {
            case Protocol::CommandCode::PING:
                onPingReceived(message);
                break;
                
            case Protocol::CommandCode::REQUEST_DATA:
                onDataRequest(message);
                break;
                
            case Protocol::CommandCode::EXECUTE_COMMAND:
                onCommandReceived(message);
                break;
                
            case Protocol::CommandCode::DATA_RESPONSE:
            case Protocol::CommandCode::COMMAND_ACK:
            case Protocol::CommandCode::ERROR_RESPONSE:
            case Protocol::CommandCode::PONG:
                onResponseReceived(message);
                break;
                
            default:
                std::cout << "[" << m_module_name << "] Unknown command: " 
                          << (int)message.getCommand() << std::endl;
                break;
        }
        
        temp_queue.pop();
    }
}

uint64_t BaseModule::generateRequestId() {
    return s_next_request_id++;
}

void BaseModule::trackRequest(uint64_t request_id) {
    std::lock_guard<std::mutex> lock(m_pending_requests_mutex);
    m_pending_requests[request_id] = Timing::getCurrentTimeMs();
}

bool BaseModule::checkRequestTimeout(uint64_t request_id) {
    std::lock_guard<std::mutex> lock(m_pending_requests_mutex);
    auto it = m_pending_requests.find(request_id);
    if (it == m_pending_requests.end()) return false;
    
    uint64_t elapsed = Timing::getCurrentTimeMs() - it->second;
    return elapsed > Protocol::ERROR_TIMEOUT_MS;
}

void BaseModule::clearExpiredRequests() {
    std::lock_guard<std::mutex> lock(m_pending_requests_mutex);
    uint64_t current_time = Timing::getCurrentTimeMs();
    
    auto it = m_pending_requests.begin();
    while (it != m_pending_requests.end()) {
        if (current_time - it->second > Protocol::ERROR_TIMEOUT_MS) {
            std::cout << "[" << m_module_name << "] Request " << it->first 
                      << " timed out" << std::endl;
            it = m_pending_requests.erase(it);
        } else {
            ++it;
        }
    }
}

void BaseModule::workerThreadFunction() {
    std::cout << "[" << m_module_name << "] Worker thread started" << std::endl;
    
    m_timer.reset();
    
    while (m_is_running) {
        // Only process if powered
        if (m_is_powered) {
            // Process incoming messages
            processMessageQueue();
            
            // Run module-specific update
            onUpdate();
            
            // Clear expired requests
            clearExpiredRequests();
        }
        
        // Wait for next cycle (maintains precise timing)
        m_timer.waitForNextCycle();
        
        // Check for timing overruns
        if (m_timer.isOverrun()) {
            std::cout << "[" << m_module_name << "] WARNING: Timing overrun detected" << std::endl;
        }
    }
    
    std::cout << "[" << m_module_name << "] Worker thread stopped" << std::endl;
}