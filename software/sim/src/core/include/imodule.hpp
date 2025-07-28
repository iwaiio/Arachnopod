#pragma once

#include "command.hpp"

class IModule {
public:
    virtual void initialize() = 0;
    virtual void processCommand(const Command& cmd) = 0;
    virtual void update() = 0;
    virtual std::string getStatus() const = 0;
    virtual ~IModule() = default;
};