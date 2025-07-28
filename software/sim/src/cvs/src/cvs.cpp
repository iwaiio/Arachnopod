#include "cvs.hpp"
#include "ses.hpp"
#include "ds.hpp"
#include <thread>
#include <chrono>

void CVS::initialize() {
    manager.registerModule("SES", std::make_shared<SESModule>());
    manager.registerModule("DS", std::make_shared<DSModule>());

    manager.sendCommand({"SES", "enable_power", { {"target", "DS"} }});
}

void CVS::mainLoop() {
    while (true) {
        manager.updateAll();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}