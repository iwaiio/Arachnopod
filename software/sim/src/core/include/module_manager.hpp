#pragma once

#include <map>
#include <queue>
#include <memory>
#include <string>
#include "imodule.hpp"
#include "logger.hpp"
#include "render_v.hpp"

class ModuleManager {
public:
    void registerModule(const std::string& name, std::shared_ptr<IModule> module);
    void sendCommand(const Command& cmd);
    void updateAll();

private:
    std::map<std::string, std::shared_ptr<IModule>> modules;
    std::queue<Command> commandQueue;
};
