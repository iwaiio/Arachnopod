#pragma once

#include <string>
#include <map>

struct Command {
    std::string target;
    std::string action;
    std::map<std::string, std::string> args;
};
