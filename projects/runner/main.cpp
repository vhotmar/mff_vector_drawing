#include <iostream>
#include <vector>
#include <variant>
#include <filesystem>

#include <mff/leaf.h>
#include <mff/graphics/logger.h>
#include <mff/graphics/window.h>

#include "./utils/logger.h"
#include "./renderer/init.h"
#include "./canvas/svg/path.h"
#include "./canvas/svg/xml.h"
#include "./canvas/canvas.h"
#include "./canvas/path.h"

boost::leaf::result<void> run(const std::string& file_name) {
    mff::logger::vulkan = mff::logger::setup_vulkan_logging();
    mff::logger::window = mff::logger::setup_window_logging();
    mff::window::EventLoop event_loop;

    auto tiger_svg = mff::read_file(file_name);
    std::string tiger_svg_string(tiger_svg.begin(), tiger_svg.end());

    auto tiger_svg_paths = canvas::svg::to_paths(tiger_svg_string);

    std::uint32_t kWIDTH = 1600;
    std::uint32_t kHEIGHT = 1200;

    LEAF_AUTO(
        window, mff::window::glfw::WindowBuilder()
        .with_size({kWIDTH, kHEIGHT})
        .with_title("SVG renderer app")
        .build(&event_loop));

    LEAF_AUTO(render_init, RendererInit::build(window));

    canvas::Transform2f base_transform = (canvas::Transform2f::from_transpose({0.0f, 0.0f})
        * canvas::Transform2f::from_scale(render_init->get_dimensions().cast<std::float_t>()).inverse())
        * canvas::Transform2f::from_scale({5.0f, 5.0f});

    std::vector<canvas::Canvas::PrerenderedPath> prerendered_paths = {};

    for (const auto& item: tiger_svg_paths) {
        auto[path, state] = item;

        if (state.fill) {
            prerendered_paths.push_back(canvas::Canvas::prerenderFill(path, {state.fill_color, base_transform}));
        }

        if (state.stroke) {
            prerendered_paths.push_back(canvas::Canvas::prerenderStroke(path,
                                                                        {state.stroke_color, {state.stroke_width},
                                                                            base_transform}
            ));
        }
    }

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

int main(int argc, char* argv[]) {
    if (argc == 1) {
        std::cout << "Please specify name of file you want to render" << std::endl;
        return -1;
    }

    if (argc != 2) {
        std::cout << "You have to specify exactly one argument" << std::endl;
        return -1;
    }

    std::string file_name(argv[1]);

    if (!std::filesystem::exists(file_name)) {
        std::cout << fmt::format("Specified file \"{}\" does not exists", file_name) << std::endl;
        return -1;
    }

    return boost::leaf::try_handle_all(
        [&]() -> boost::leaf::result<int> {
            LEAF_CHECK(run(file_name));

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