#pragma once

#include "protocol.hpp"
#include "timing.hpp"
#include <memory>
#include <functional>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <cstdint>

// ===================================================================
// BASE MODULE INTERFACES WITH NEW PROTOCOL
// ===================================================================

// Bus interface using new protocol
class IBus {
public:
    virtual ~IBus() = default;
    
    virtual bool sendMessage(const Protocol::Message& message) = 0;
    virtual void subscribe(Protocol::ModuleID module_id, 
                          std::function<void(const Protocol::Message&)> handler) = 0;
    virtual void unsubscribe(Protocol::ModuleID module_id) = 0;
};

// Module interface
class IModule {
public:
    virtual ~IModule() = default;
    
    virtual Protocol::ModuleID getModuleID() const = 0;
    virtual std::string getModuleName() const = 0;
    virtual bool initialize(std::shared_ptr<IBus> bus) = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual void handleMessage(const Protocol::Message& message) = 0;
    virtual bool isPowered() const = 0;
    virtual void setPowered(bool powered) = 0;
};

// Base module implementation with threading and power management
class BaseModule : public IModule {
protected:
    Protocol::ModuleID m_module_id;
    std::string m_module_name;
    std::shared_ptr<IBus> m_bus;
    
    // Threading
    std::atomic<bool> m_is_running{false};
    std::atomic<bool> m_is_powered{false};
    std::thread m_worker_thread;
    Timing::PrecisionTimer m_timer;
    
    // Message handling
    std::queue<Protocol::Message> m_message_queue;
    std::mutex m_queue_mutex;
    
    // Response timeout tracking
    std::mutex m_pending_requests_mutex;
    std::unordered_map<uint64_t, uint64_t> m_pending_requests; // request_id -> timestamp
    
public:
    BaseModule(Protocol::ModuleID id, const std::string& name, double frequency_hz);
    virtual ~BaseModule();
    
    // IModule interface
    Protocol::ModuleID getModuleID() const override;
    std::string getModuleName() const override;
    bool initialize(std::shared_ptr<IBus> bus) override;
    bool start() override;
    void stop() override;
    void handleMessage(const Protocol::Message& message) override;
    bool isPowered() const override;
    void setPowered(bool powered) override;

protected:
    // Module lifecycle methods for overriding
    virtual bool onInitialize() { return true; }
    virtual bool onStart() { return true; }
    virtual void onStop() {}
    virtual void onPowerOn() {}
    virtual void onPowerOff() {}
    virtual void onUpdate() {}
    
    // Message handling methods for overriding
    virtual void onPingReceived(const Protocol::Message& message);
    virtual void onDataRequest(const Protocol::Message& message) = 0;
    virtual void onCommandReceived(const Protocol::Message& message) = 0;
    virtual void onResponseReceived(const Protocol::Message& message) {}
    
    // Utility methods
    bool sendMessage(const Protocol::Message& message);
    bool sendResponse(Protocol::ModuleID target, Protocol::CommandCode response_type, 
                     const std::vector<Protocol::DataBlock>& data = {});
    void enqueueMessage(const Protocol::Message& message);
    void processMessageQueue();
    
    // Request tracking
    uint64_t generateRequestId();
    void trackRequest(uint64_t request_id);
    bool checkRequestTimeout(uint64_t request_id);
    void clearExpiredRequests();

private:
    void workerThreadFunction();
    static uint64_t s_next_request_id;
};