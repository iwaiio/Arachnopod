#pragma once
#include "module_manager.hpp"

class CVS {
public:
    void initialize();
    void mainLoop();

private:
    ModuleManager manager;
};