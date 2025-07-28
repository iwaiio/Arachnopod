#pragma once

#include <string>
#include <queue>
#include <unordered_map>
#include <functional>
#include <mutex>

struct BusMessage {
    std::string source;
    std::string destination;
    std::string payload;
};

class MockBus {
public:
    void send(const BusMessage& msg);
    void registerReceiver(const std::string& name, std::function<void(const BusMessage&)> callback);

private:
    std::unordered_map<std::string, std::function<void(const BusMessage&)>> receivers;
    std::mutex mutex;
};
