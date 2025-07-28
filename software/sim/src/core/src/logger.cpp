#include "logger.hpp"

namespace logger {
    std::shared_ptr<spdlog::logger> _logger;

    void init() {
        _logger = spdlog::basic_logger_mt("core", "logs/robot_log.txt");
        spdlog::set_level(spdlog::level::debug);
        spdlog::set_pattern("[%H:%M:%S] [%l] %v");
    }

    std::shared_ptr<spdlog::logger> get() {
        return _logger;
    }
}
