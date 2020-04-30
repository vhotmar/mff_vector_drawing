#include "logger.h"

namespace logger {

std::shared_ptr<spdlog::logger> setup_system_logging() {
    auto logger = spdlog::stdout_color_mt("system");

    logger->set_level(spdlog::level::trace);

    return logger;
}

std::shared_ptr<spdlog::logger> setup_main_logging() {
    return spdlog::stdout_color_mt("main");
}

std::shared_ptr<spdlog::logger> system = setup_system_logging();
std::shared_ptr<spdlog::logger> main = setup_main_logging();

}