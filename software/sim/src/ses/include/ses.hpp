#pragma once
#include "module_manager.hpp"
#include "ses_interface.hpp"


class SESModule : public IModule {
public:
    void initialize() override;
    void processCommand(const Command& cmd) override;
    void update() override;
    std::string getStatus() const override;

private:
    enum class State { POWER_OFF, POWERING_ON, ACTIVE, ERROR_STATE } state;
};