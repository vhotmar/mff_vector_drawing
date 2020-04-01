#include <iostream>

#include "./internal/renderer/vulkan/instance.h"
#include "./internal/renderer/vulkan/logical_device.h"
#include "./internal/renderer/vulkan/physical_device.h"
#include "./internal/window/events_debug.h"
#include "./internal/window/event_loop.h"
#include "./internal/window/window.h"
#include "./utils/logger.h"

#include <vk_mem_alloc.h>

int main() {
    namespace mwin = mff::internal::window;
    namespace vulkan = mff::internal::renderer::vulkan;

    mwin::EventLoop event_loop;

    auto window = mwin::WindowBuilder()
        .with_size({400, 400})
        .with_title("My app")
        .build(&event_loop);

    auto instance = vulkan::create_instance("mff_vector_drawing").value();
    auto surface = window->create_surface(instance.instance.get(), nullptr).value();
    auto physical_device = vulkan::get_physical_device(instance.instance.get(), surface.get()).value();
    auto logical_device = vulkan::create_logical_device(physical_device, surface.get()).value();

    event_loop.run([&](auto event) {
        if (auto window_event = std::get_if<mwin::events::WindowEvent>(&event)) {
            if (std::holds_alternative<mwin::events::window::CloseRequested>(window_event->event)) {
                logger::main->info("Quitting application");
                return mwin::ExecutionControl::Terminate;
            }
        }

        return mwin::ExecutionControl::Wait;
    });
}

