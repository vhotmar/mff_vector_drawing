#include <iostream>
#include <vector>
#include <variant>

#include <mff/leaf.h>
#include <mff/graphics/logger.h>
#include <mff/graphics/window.h>

#include "./utils/logger.h"
#include "./renderer/init.h"
#include "./canvas/svg/path.h"
#include "./canvas/svg/xml.h"
#include "./canvas/canvas.h"
#include "./canvas/path.h"

boost::leaf::result<void> run() {
    mff::logger::vulkan = mff::logger::setup_vulkan_logging();
    mff::logger::window = mff::logger::setup_window_logging();
    mff::window::EventLoop event_loop;

    auto tiger_svg = mff::read_file("./Ghostscript_Tiger.svg");
    std::string tiger_svg_string(tiger_svg.begin(), tiger_svg.end());

    auto tiger_svg_paths = canvas::svg::to_paths(tiger_svg_string);

    std::uint32_t kWIDTH = 1600;
    std::uint32_t kHEIGHT = 1200;

    LEAF_AUTO(
        window, mff::window::glfw::WindowBuilder()
        .with_size({kWIDTH, kHEIGHT})
        .with_title("My app")
        .build(&event_loop));

    LEAF_AUTO(render_init, RendererInit::build(window));

    canvas::Transform2f base_transform = (canvas::Transform2f::from_transpose({-1.0f, -1.0f})
        * canvas::Transform2f::from_scale(render_init->get_dimensions().cast<std::float_t>()).inverse())
        * canvas::Transform2f::from_scale({5.0f, 5.0f});

    auto prerendered_paths = tiger_svg_paths
        | ranges::views::transform(
            [&](const auto& item) {
                auto[path, state] = item;
                path.transform(base_transform);

                return canvas::Canvas::prerenderFill(path, state.fill_color);
            }
        )
        | ranges::to<std::vector>();

    canvas::Canvas canvas(render_init->get_renderer());

    // Renderer renderer(window);

    // LEAF_CHECK(renderer.init());

    bool first = true;
    float scale = -0.4f;

    auto draw = [&]() -> boost::leaf::result<void> {
        // draw commands
        if (!first) {
            scale += 0.01f;

            for (const auto& path: prerendered_paths) {
                canvas.drawPrerendered(path);
            }
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