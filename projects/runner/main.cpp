#include <iostream>
#include <vector>
#include <variant>

#include <mff/leaf.h>
#include <mff/graphics/logger.h>
#include <mff/graphics/window.h>

#include "./utils/logger.h"
#include "./canvas.h"
#include "./renderer/init.h"

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

    LEAF_AUTO(render_init, RendererInit::build(window));

    canvas::Canvas canvas(render_init->get_renderer());

    // Renderer renderer(window);

    // LEAF_CHECK(renderer.init());

    bool first = true;
    float scale = -0.4f;

    auto draw = [&]() -> boost::leaf::result<void> {
        // draw commands
        if (!first) {
            scale += 0.01f;

            canvas::Path2D path;
            path.move_to({0, 0});
            path.line_to({0, 1});
            path.quad_to({0.75, 0.75}, {0, 0});
            path.close_path();

            canvas.fill(path, {1.0f, 0.0f, 0.0f, 1.0f});

            path = {};
            path.move_to({ 0, 0 });
            path.line_to({0, -0.5});
            path.line_to({-scale, -0.5});
            path.close_path();

            canvas.fill(path, {scale, 1.0f, 0.0f, 1.0f});

            path = {};
            path.move_to({ 0, 0 });
            path.line_to({0.5, -0.5});
            path.line_to({0.5, 0.5});
            path.close_path();
        }

        LEAF_CHECK(render_init->present());

        first = false;

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

            return mff::window::ExecutionControl::Poll;
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