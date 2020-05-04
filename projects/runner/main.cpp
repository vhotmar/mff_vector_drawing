#include <iostream>
#include <vector>
#include <variant>

#include <mff/leaf.h>
#include <mff/graphics/logger.h>
#include <mff/graphics/window.h>

#include "./utils/logger.h"
#include "./renderer2.h"

boost::leaf::result<void> run() {
    mff::logger::vulkan = mff::logger::setup_vulkan_logging();
    mff::logger::window = mff::logger::setup_window_logging();
    mff::window::EventLoop event_loop;

    std::uint32_t kWIDTH = 800;
    std::uint32_t kHEIGHT = 600;

    LEAF_AUTO(window, mff::window::glfw::WindowBuilder()
        .with_size({kWIDTH, kHEIGHT})
        .with_title("My app")
        .build(&event_loop));

    LEAF_AUTO(engine, VulkanBaseEngine::build(window));
    LEAF_AUTO(presenter, Presenter::build(engine.get()));
    LEAF_AUTO(renderer, RendererContext::build(engine.get(), presenter.get()));
    LEAF_AUTO(renderer_surface, RendererSurface::build(renderer.get(), {kWIDTH, kHEIGHT}));

    LEAF_CHECK(presenter->build_commands(renderer_surface->get_main_image(), {kWIDTH, kHEIGHT}));

    // Renderer renderer(window);

    // LEAF_CHECK(renderer.init());

    auto draw = [&]() -> boost::leaf::result<void> {
        // draw commands

        LEAF_AUTO(fresh, presenter->draw());

        if (!fresh) {
            LEAF_CHECK(presenter->build_commands(renderer_surface->get_main_image(), {kWIDTH, kHEIGHT}));
        }

        engine->get_device()->get_handle().waitIdle();

        return {};
    };

    event_loop.run(
        [&](auto event) {
            if (auto window_event = std::get_if<mff::window::events::WindowEvent>(&event)) {
                if (std::holds_alternative<mff::window::events::window::CloseRequested>(window_event->event)) {
                    logger::main->info("Quitting application");
                    return mff::window::ExecutionControl::Terminate;
                }
            }

            if (std::holds_alternative<mff::window::events::PollingCompleted>(event)) {
                auto res = draw();

                if (!res) return mff::window::ExecutionControl::Terminate;
            }

            return mff::window::ExecutionControl::Wait;
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