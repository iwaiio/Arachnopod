#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace logger {
    void init();  // вызвать один раз
    std::shared_ptr<spdlog::logger> get();
};