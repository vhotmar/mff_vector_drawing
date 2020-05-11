#define _USE_MATH_DEFINES

#include <cmath>

#include <iostream>
#include <vector>
#include <variant>
#include <filesystem>

#include <boost/program_options.hpp>
#include <mff/leaf.h>
#include <mff/graphics/logger.h>
#include <mff/graphics/window.h>

#include "./utils/logger.h"
#include "./renderer/init.h"
#include "./canvas/svg/path.h"
#include "./canvas/svg/xml.h"
#include "./canvas/canvas.h"
#include "./canvas/path.h"

struct RunOptions {
    std::string file_name;
    int width;
    int height;
    float scale;
    float translate_x;
    float translate_y;
};

boost::leaf::result<void> run(const RunOptions& ro) {
    mff::logger::vulkan = mff::logger::setup_vulkan_logging();
    mff::logger::window = mff::logger::setup_window_logging();
    mff::window::EventLoop event_loop;

    auto tiger_svg = mff::read_file(ro.file_name);
    std::string tiger_svg_string(tiger_svg.begin(), tiger_svg.end());

    auto tiger_svg_paths = canvas::svg::to_paths(tiger_svg_string);

    std::uint32_t kWIDTH = ro.width;
    std::uint32_t kHEIGHT = ro.height;

    LEAF_AUTO(
        window, mff::window::glfw::WindowBuilder()
        .with_size({kWIDTH, kHEIGHT})
        .with_title("SVG renderer app")
        .build(&event_loop));

    LEAF_AUTO(render_init, RendererInit::build(window));

    canvas::Transform2f base_transform = (canvas::Transform2f::from_translate({-1, -1})
        * canvas::Transform2f::from_scale(render_init->get_dimensions().cast<std::float_t>()).inverse())
        * canvas::Transform2f::from_translate({ro.translate_x, ro.translate_y})
        * canvas::Transform2f::from_scale({ro.scale, ro.scale});

    std::vector<canvas::Canvas::PrerenderedPath> prerendered_paths = {};

    for (const auto& item: tiger_svg_paths) {
        auto[path, state] = item;

        auto prerender_fill = [&]() {
            auto[path, state] = item;

            if (state.fill)
                prerendered_paths.push_back(canvas::Canvas::prerenderFill(path, {state.fill_color, base_transform}));
        };

        auto prerender_stroke = [&]() {
            auto[path, state] = item;

            if (state.stroke)
                prerendered_paths.push_back(
                    canvas::Canvas::prerenderStroke(
                        path,
                        {state.stroke_color, {state.stroke_width, state.line_cap, state.line_join},
                            base_transform}
                    ));
        };

        if (state.paint_first == canvas::svg::DrawStatePaintFirst::Fill) {
            prerender_fill();
            prerender_stroke();
        } else {
            prerender_stroke();
            prerender_fill();
        }
    }

    canvas::Canvas canvas(render_init->get_renderer());

    bool first = true;
    float scale = -0.4f;


    auto draw = [&]() -> boost::leaf::result<void> {
        // draw commands
        if (!first) {
            scale += 0.01f;
        } else {
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
    namespace po = boost::program_options;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("width,w", po::value<int>()->default_value(800), "set width of the window")
        ("height,h", po::value<int>()->default_value(600), "set height of the window")
        ("scale,s", po::value<float>()->default_value(1.0f), "set the scale of displayed image")
        ("translate_x,tx", po::value<float>()->default_value(0.0f), "set x translation of displayed image")
        ("translate_y,ty", po::value<float>()->default_value(0.0f), "set y translation of displayed image")
        ("file,f", po::value<std::string>(), "the file to display");

    po::positional_options_description p;
    p.add("file", 1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }

    if (!vm.count("file")) {
        std::cout << "Please specify name of file you want to render" << std::endl;
        std::cout << desc << std::endl;

        return -1;
    }

    std::string file_name(vm["file"].as<std::string>());

    if (!std::filesystem::exists(file_name)) {
        std::cout << fmt::format("Specified file \"{}\" does not exists", file_name) << std::endl;
        return -1;
    }

    RunOptions ro = {
        file_name,
        vm["width"].as<int>(),
        vm["height"].as<int>(),
        vm["scale"].as<float>(),
        vm["translate_x"].as<float>(),
        vm["translate_x"].as<float>()
    };

    return boost::leaf::try_handle_all(
        [&]() -> boost::leaf::result<int> {
            LEAF_CHECK(run(ro));

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