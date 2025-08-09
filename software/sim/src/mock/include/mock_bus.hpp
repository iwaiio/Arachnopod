#pragma once

#include "imodule.hpp"
#include <unordered_map>
#include <thread>
#include <queue>
#include <mutex>

// Enhanced bus implementation with UART-like protocol
class MockBus : public IBus {
private:
    std::unordered_map<Protocol::ModuleID, std::function<void(const Protocol::Message&)>> m_subscribers;
    mutable std::mutex m_subscribers_mutex;
    
    // Message queue for simulation
    std::queue<Protocol::Message> m_message_queue;
    mutable std::mutex m_queue_mutex;
    
    // Bus processing thread
    std::thread m_bus_thread;
    std::atomic<bool> m_running{false};
    Timing::PrecisionTimer m_timer;
    
    // Statistics
    std::atomic<uint64_t> m_messages_sent{0};
    std::atomic<uint64_t> m_messages_delivered{0};
    std::atomic<uint64_t> m_messages_dropped{0};
    std::atomic<uint64_t> m_protocol_errors{0};
    
public:
    MockBus();
    ~MockBus();
    
    // IBus interface implementation
    bool sendMessage(const Protocol::Message& message) override;
    void subscribe(Protocol::ModuleID module_id, 
                   std::function<void(const Protocol::Message&)> handler) override;
    void unsubscribe(Protocol::ModuleID module_id) override;
    
    // Additional methods
    void printStatistics() const;
    void shutdown();

private:
    void processingThreadFunction();
    void deliverMessage(const Protocol::Message& message);
    bool validateMessage(const Protocol::Message& message);
};