#include "module_manager.hpp"

void ModuleManager::registerModule(const std::string& name, std::shared_ptr<IModule> module) {
    modules[name] = std::move(module);
}

void ModuleManager::sendCommand(const Command& cmd) {
    commandQueue.push(cmd);
}

void ModuleManager::updateAll() {
    while (!commandQueue.empty()) {
        const Command& cmd = commandQueue.front();
        if (modules.count(cmd.target)) {
            modules[cmd.target]->processCommand(cmd);
        }
        commandQueue.pop();
    }
    for (auto& [name, module] : modules) {
        module->update();
    }
}