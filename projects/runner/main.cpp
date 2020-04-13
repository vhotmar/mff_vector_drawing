#include <iostream>

#include "./internal/renderer/vulkan/instance.h"
#include "./internal/window/events_debug.h"
#include "./internal/window/event_loop.h"
#include "./internal/window/window.h"
#include "./utils/logger.h"

#include <vk_mem_alloc.h>

boost::leaf::result<void> run() {
    namespace mwin = mff::internal::window;
    namespace vulkan = mff::internal::renderer::vulkan;

    mwin::EventLoop event_loop;

    auto window = mwin::WindowBuilder()
        .with_size({400, 400})
        .with_title("My app")
        .build(&event_loop);

    LEAF_AUTO(instance, vulkan::Instance::build(std::nullopt, window->get_required_extensions(), {}));
    LEAF_AUTO(surface, window->create_surface(instance.get_handle(), nullptr));

    instance.get_physical_devices()[0].

        event_loop.run(
        [&](auto event) {
            if (auto window_event = std::get_if<mwin::events::WindowEvent>(&event)) {
                if (std::holds_alternative<mwin::events::window::CloseRequested>(window_event->event)) {
                    logger::main->info("Quitting application");
                    return mwin::ExecutionControl::Terminate;
                }
            }

            return mwin::ExecutionControl::Wait;
        }
    );

    return {};
}

int main() {
    return boost::leaf::try_handle_all(
        [&]() -> boost::leaf::result<int> {
            LEAF_CHECK(run());

            return 0;
        },
        [](boost::leaf::error_info const& unmatched) {
            std::cerr <<
                "Unknown failure detected" << std::endl <<
                "Cryptic diagnostic information follows" << std::endl <<
                unmatched;
            return 6;
        }
    );
}

