#include "ses.hpp"
#include <iostream>

void SESModule::initialize() {
    state = State::POWERING_ON;
    std::cout << "SES: powering on.\n";
}

void SESModule::processCommand(const Command& cmd) {
    if (cmd.action == "enable_power") {
        state = State::ACTIVE;
        std::cout << "SES: power enabled for " << cmd.args.at("target") << "\n";
    }
}

void SESModule::update() {
    // simulate power system monitoring
}

std::string SESModule::getStatus() const {
    switch (state) {
        case State::POWER_OFF: return "OFF";
        case State::POWERING_ON: return "BOOT";
        case State::ACTIVE: return "ACTIVE";
        case State::ERROR_STATE: return "ERROR_STATE";
    }
    return "UNKNOWN";
}