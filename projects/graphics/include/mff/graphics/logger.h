#pragma once

#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace mff::logger {

std::shared_ptr<spdlog::logger> setup_window_logging();
std::shared_ptr<spdlog::logger> setup_vulkan_logging();

extern std::shared_ptr<spdlog::logger> window;
extern std::shared_ptr<spdlog::logger> vulkan;

}