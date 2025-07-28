#include "ds.hpp"
#include <iostream>

void DSModule::initialize() {
    state = State::IDLE;
    std::cout << "DS: initialized.\n";
}

void DSModule::processCommand(const Command& cmd) {
    if (cmd.action == "move") {
        std::cout << "DS: moving in direction: " << cmd.args.at("direction") << "\n";
        state = State::MOVING;
    }
}

void DSModule::update() {
    // simulate movement status updates
}

std::string DSModule::getStatus() const {
    switch (state) {
        case State::IDLE: return "IDLE";
        case State::MOVING: return "MOVING";
        case State::ERROR_STATE: return "ERROR_STATE";
    }
    return "UNKNOWN";
}