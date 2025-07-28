#pragma once
#include "module_manager.hpp"

class DSModule : public IModule {
public:
    void initialize() override;
    void processCommand(const Command& cmd) override;
    void update() override;
    std::string getStatus() const override;

private:
    enum class State { IDLE, MOVING, ERROR_STATE } state;
};