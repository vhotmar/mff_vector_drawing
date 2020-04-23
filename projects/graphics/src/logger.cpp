#include <mff/graphics/logger.h>

namespace mff::logger {

std::shared_ptr<spdlog::logger> setup_window_logging() {
    auto logger = spdlog::stdout_color_mt("window");

    logger->set_level(spdlog::level::trace);

    return logger;
}

std::shared_ptr<spdlog::logger> setup_vulkan_logging() {
    auto logger = spdlog::stdout_color_mt("vulkan");

    logger->set_level(spdlog::level::trace);

    return logger;
}

}