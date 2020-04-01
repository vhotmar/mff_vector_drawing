#pragma once

#define SPDLOG_FMT_EXTERNAL 1

#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "debug.h"

namespace logger {

std::shared_ptr<spdlog::logger> setup_system_logging();
std::shared_ptr<spdlog::logger> setup_main_logging();
std::shared_ptr<spdlog::logger> setup_vulkan_logging();

extern std::shared_ptr<spdlog::logger> system;
extern std::shared_ptr<spdlog::logger> main;
extern std::shared_ptr<spdlog::logger> vulkan;

}