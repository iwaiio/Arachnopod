#include "mock_bus.hpp"
#include <iostream>
#include <iomanip>

MockBus::MockBus() : m_timer(Timing::BUS_FREQUENCY_HZ) {
    std::cout << "[MockBus] UART-like bus initialized (freq: " 
              << Timing::BUS_FREQUENCY_HZ << " Hz)" << std::endl;
    
    m_running = true;
    m_bus_thread = std::thread(&MockBus::processingThreadFunction, this);
}

MockBus::~MockBus() {
    shutdown();
}

void MockBus::shutdown() {
    if (m_running) {
        std::cout << "[MockBus] Shutting down..." << std::endl;
        m_running = false;
        
        if (m_bus_thread.joinable()) {
            m_bus_thread.join();
        }
        
        std::cout << "[MockBus] Bus stopped. Final stats: sent=" 
                  << m_messages_sent.load() << ", delivered=" << m_messages_delivered.load()
                  << ", dropped=" << m_messages_dropped.load() << std::endl;
    }
}

bool MockBus::sendMessage(const Protocol::Message& message) {
    // Validate message format
    if (!validateMessage(message)) {
        m_protocol_errors++;
        std::cout << "[MockBus] Protocol error: Invalid message format" << std::endl;
        return false;
    }
    
    // Serialize and show raw bytes (simulate UART transmission)
    auto serialized = message.serialize();
    
    // Детальный вывод для отладки
    std::cout << "[MockBus] TX [" 
              << std::setw(2) << std::setfill('0') << std::hex 
              << (int)message.getTargetModule() << std::dec
              << " <- CMD:" << (int)message.getCommand() 
              << " BLOCKS:" << (int)message.getDataBlockCount()
              << "] ";
    
    // Вывод первых байтов сообщения в hex
    std::cout << "Raw: ";
    for (size_t i = 0; i < std::min(serialized.size(), size_t(16)); ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << (int)serialized[i] << " ";
    }
    if (serialized.size() > 16) {
        std::cout << "... ";
    }
    std::cout << std::dec << "(" << serialized.size() << " bytes)" << std::endl;
    
    // Add to processing queue
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        
        // Проверка на переполнение очереди
        if (m_message_queue.size() > 1000) {
            std::cout << "[MockBus] WARNING: Message queue overflow!" << std::endl;
            m_messages_dropped++;
            return false;
        }
        
        m_message_queue.push(message);
    }
    
    m_messages_sent++;
    return true;
}

void MockBus::subscribe(Protocol::ModuleID module_id, 
                       std::function<void(const Protocol::Message&)> handler) {
    std::lock_guard<std::mutex> lock(m_subscribers_mutex);
    m_subscribers[module_id] = handler;
    std::cout << "[MockBus] Module " << std::hex << "0x" << (int)module_id << std::dec 
              << " subscribed to bus" << std::endl;
}

void MockBus::unsubscribe(Protocol::ModuleID module_id) {
    std::lock_guard<std::mutex> lock(m_subscribers_mutex);
    auto it = m_subscribers.find(module_id);
    if (it != m_subscribers.end()) {
        m_subscribers.erase(it);
        std::cout << "[MockBus] Module " << std::hex << "0x" << (int)module_id << std::dec 
                  << " unsubscribed from bus" << std::endl;
    }
}

void MockBus::printStatistics() const {
    std::cout << "\n[MockBus] === Communication Statistics ===" << std::endl;
    std::cout << "[MockBus] Messages sent:      " << m_messages_sent.load() << std::endl;
    std::cout << "[MockBus] Messages delivered: " << m_messages_delivered.load() << std::endl;
    std::cout << "[MockBus] Messages dropped:   " << m_messages_dropped.load() << std::endl;
    std::cout << "[MockBus] Protocol errors:    " << m_protocol_errors.load() << std::endl;
    std::cout << "[MockBus] Active subscribers: " << m_subscribers.size() << std::endl;
    
    if (m_messages_sent.load() > 0) {
        double success_rate = (double)m_messages_delivered.load() / m_messages_sent.load() * 100.0;
        std::cout << "[MockBus] Delivery success rate: " 
                  << std::fixed << std::setprecision(2) << success_rate << "%" << std::endl;
        
        double error_rate = (double)m_protocol_errors.load() / m_messages_sent.load() * 100.0;
        std::cout << "[MockBus] Protocol error rate:   " 
                  << std::fixed << std::setprecision(2) << error_rate << "%" << std::endl;
    }
    
    // Вывод статистики по модулям
    std::cout << "[MockBus] Subscribers by module:" << std::endl;
    for (const auto& [module_id, handler] : m_subscribers) {
        std::cout << "[MockBus]   Module 0x" << std::hex << (int)module_id << std::dec;
        switch(module_id) {
            case Protocol::ModuleID::CVS:
                std::cout << " (CVS - Central Computing)";
                break;
            case Protocol::ModuleID::SES:
                std::cout << " (SES - Power Supply)";
                break;
            case Protocol::ModuleID::DS:
                std::cout << " (DS - Drive System)";
                break;
            case Protocol::ModuleID::OS:
                std::cout << " (OS - Orientation System)";
                break;
            case Protocol::ModuleID::VIS:
                std::cout << " (VIS - Vision System)";
                break;
            default:
                std::cout << " (Unknown)";
        }
        std::cout << std::endl;
    }
    std::cout << "[MockBus] =================================" << std::endl;
}

void MockBus::processingThreadFunction() {
    std::cout << "[MockBus] Processing thread started (freq: " 
              << Timing::BUS_FREQUENCY_HZ << " Hz)" << std::endl;
    
    m_timer.reset();
    uint32_t cycle_count = 0;
    
    while (m_running) {
        // Process all queued messages in this cycle
        int messages_processed = 0;
        const int MAX_MESSAGES_PER_CYCLE = 10; // Ограничение для предотвращения блокировки
        
        while (messages_processed < MAX_MESSAGES_PER_CYCLE) {
            Protocol::Message message;
            bool has_message = false;
            
            {
                std::lock_guard<std::mutex> lock(m_queue_mutex);
                if (!m_message_queue.empty()) {
                    message = m_message_queue.front();
                    m_message_queue.pop();
                    has_message = true;
                }
            }
            
            if (has_message) {
                // Симуляция задержки передачи по UART
                // При 115200 бод, передача ~10 байт занимает ~1мс
                auto message_size = message.serialize().size();
                if (message_size > 50) {
                    // Для больших сообщений добавляем микрозадержку
                    Timing::delay_us(message_size * 10); // ~10us на байт
                }
                
                deliverMessage(message);
                messages_processed++;
            } else {
                break; // Очередь пуста
            }
        }
        
        // Периодическая диагностика
        cycle_count++;
        if (cycle_count % 10000 == 0) { // Каждые 10 секунд при 1000 Hz
            size_t queue_size = 0;
            {
                std::lock_guard<std::mutex> lock(m_queue_mutex);
                queue_size = m_message_queue.size();
            }
            
            if (queue_size > 0) {
                std::cout << "[MockBus] Queue status: " << queue_size 
                          << " messages pending" << std::endl;
            }
        }
        
        // Wait for next bus cycle (high frequency for fast bus)
        m_timer.waitForNextCycle();
        
        // Проверка на перегрузку
        if (m_timer.isOverrun() && cycle_count % 1000 == 0) {
            std::cout << "[MockBus] WARNING: Bus timing overrun detected!" << std::endl;
        }
    }
    
    std::cout << "[MockBus] Processing thread stopped" << std::endl;
}

void MockBus::deliverMessage(const Protocol::Message& message) {
    std::function<void(const Protocol::Message&)> handler;
    
    // Find subscriber
    {
        std::lock_guard<std::mutex> lock(m_subscribers_mutex);
        auto it = m_subscribers.find(message.getTargetModule());
        if (it != m_subscribers.end()) {
            handler = it->second;
        }
    }
    
    if (handler) {
        try {
            // Simulate UART receive with deserialization
            auto serialized = message.serialize();
            Protocol::Message received_msg;
            
            if (Protocol::Message::deserialize(serialized, received_msg)) {
                // Успешная доставка
                std::cout << "[MockBus] RX [Module 0x" << std::hex 
                          << (int)message.getTargetModule() << std::dec
                          << " <- CMD:" << (int)message.getCommand() 
                          << " BLOCKS:" << (int)message.getDataBlockCount() 
                          << "]" << std::endl;
                
                // Вызов обработчика модуля
                handler(received_msg);
                m_messages_delivered++;
            } else {
                std::cout << "[MockBus] ERROR: Deserialization failed for message to module 0x" 
                          << std::hex << (int)message.getTargetModule() << std::dec << std::endl;
                m_messages_dropped++;
                m_protocol_errors++;
            }
        } catch (const std::exception& e) {
            std::cout << "[MockBus] ERROR: Exception during delivery: " << e.what() << std::endl;
            m_messages_dropped++;
        } catch (...) {
            std::cout << "[MockBus] ERROR: Unknown exception during delivery" << std::endl;
            m_messages_dropped++;
        }
    } else {
        // Нет подписчика для этого модуля
        if (message.getTargetModule() != Protocol::ModuleID::BROADCAST) {
            std::cout << "[MockBus] WARNING: No subscriber for module 0x" 
                      << std::hex << (int)message.getTargetModule() << std::dec << std::endl;
        }
        m_messages_dropped++;
    }
}

bool MockBus::validateMessage(const Protocol::Message& message) {
    // Проверка стартового и стопового байтов
    if (message.start_byte != Protocol::START_BYTE || 
        message.stop_byte != Protocol::STOP_BYTE) {
        std::cout << "[MockBus] Validation failed: Invalid start/stop bytes" << std::endl;
        return false;
    }
    
    // Проверка соответствия количества блоков данных
    if (message.data_blocks.size() != message.header.fields.data_blocks) {
        std::cout << "[MockBus] Validation failed: Block count mismatch (expected " 
                  << message.header.fields.data_blocks << ", got " 
                  << message.data_blocks.size() << ")" << std::endl;
        return false;
    }
    
    // Проверка максимального количества блоков
    if (message.data_blocks.size() > Protocol::MAX_DATA_BLOCKS) {
        std::cout << "[MockBus] Validation failed: Too many data blocks (" 
                  << message.data_blocks.size() << " > " 
                  << Protocol::MAX_DATA_BLOCKS << ")" << std::endl;
        return false;
    }
    
    // Проверка валидности ID модуля
    if (message.header.fields.device_id > 0x1F) { // 5 бит = max 31
        std::cout << "[MockBus] Validation failed: Invalid device ID" << std::endl;
        return false;
    }
    
    // Проверка валидности кода команды
    if (message.header.fields.command > 0x07) { // 3 бита = max 7
        std::cout << "[MockBus] Validation failed: Invalid command code" << std::endl;
        return false;
    }
    
    return true;
}