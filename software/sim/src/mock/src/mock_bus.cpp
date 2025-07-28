#include "mock_bus.hpp"

void MockBus::send(const BusMessage& msg) {
    std::lock_guard<std::mutex> lock(mutex);
    if (receivers.count(msg.destination)) {
        receivers[msg.destination](msg);
    }
}

void MockBus::registerReceiver(const std::string& name, std::function<void(const BusMessage&)> callback) {
    std::lock_guard<std::mutex> lock(mutex);
    receivers[name] = callback;
}
