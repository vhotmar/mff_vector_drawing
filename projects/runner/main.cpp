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

/**
 * Read an SVG file and prerender it (create all information needed for immediate render)
 * @param file_name
 * @param base_transform
 * @return
 */
std::vector<canvas::Canvas::PrerenderedPath> prerender_svg_file(
    const std::string& file_name,
    const canvas::Transform2f base_transform
) {
    auto svg_file = mff::read_file(file_name);
    std::string svg_file_string(svg_file.begin(), svg_file.end());

    auto svg_file_paths = canvas::svg::to_paths(svg_file_string);

    std::vector<canvas::Canvas::PrerenderedPath> prerendered_paths = {};

    for (const auto& item: svg_file_paths) {
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

    return prerendered_paths;
}

/**
 *
 * @param ro
 * @return
 */
boost::leaf::result<void> run(const RunOptions& ro) {
    mff::window::EventLoop event_loop;

    std::uint32_t kWIDTH = ro.width;
    std::uint32_t kHEIGHT = ro.height;

    // init the window
    LEAF_AUTO(
        window, mff::window::glfw::WindowBuilder()
        .with_size({kWIDTH, kHEIGHT})
        .with_title("SVG renderer app")
        .build(&event_loop));

    // Init all utils needed for render
    LEAF_AUTO(render_init, RendererInit::build(window));

    // Vulkan coordinates are (0,0) in center of screen se at first we will move everything to
    // the upper left corner
    canvas::Transform2f base_transform = (canvas::Transform2f::from_translate({-1, -1})
        // then scale everything by framebuffer size (could be by window size...)
        * canvas::Transform2f::from_scale(render_init->get_dimensions().cast<std::float_t>()).inverse())
        // move everything as specified in command arguments
        * canvas::Transform2f::from_translate({ro.translate_x, ro.translate_y})
            // scale everything
        * canvas::Transform2f::from_scale({ro.scale, ro.scale});

    // Init the canvas on which we will render
    canvas::Canvas canvas(render_init->get_renderer());

    auto prerendered_paths = prerender_svg_file(ro.file_name, base_transform);

    // now we will render everything in canvas
    for (const auto& path: prerendered_paths) {
        canvas.drawPrerendered(path);
    }

    auto draw = [&]() -> boost::leaf::result<void> {
        // and now we will just present the canvas to user
        LEAF_CHECK(render_init->present());

        return {};
    };

    // run in event loop
    event_loop.run(
        [&](auto event) {
            // check if we should quit
            if (auto window_event = std::get_if<mff::window::events::WindowEvent>(&event)) {
                if (std::holds_alternative<mff::window::events::window::CloseRequested>(window_event->event)) {
                    logger::main->info("Quitting application");
                    return mff::window::ExecutionControl::Terminate;
                }
            }

            // check if all events polled
            if (std::holds_alternative<mff::window::events::PollingCompleted>(event)) {
                auto res = draw();

                if (!res) return mff::window::ExecutionControl::Terminate;
            }

            return mff::window::ExecutionControl::Wait;
        }
    );

    return {};
}

/**
 * Parse command line options
 * @param argc
 * @param argv
 * @return
 */
std::optional<RunOptions> parse_command_line_options(int argc, char** argv) {
    namespace po = boost::program_options;

    RunOptions result;

    try {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("width,w", po::value<int>(&result.width)->default_value(800), "set width of the window")
            ("height,h", po::value<int>(&result.height)->default_value(600), "set height of the window")
            ("scale,s", po::value<float>(&result.scale)->default_value(1.0f), "set the scale of displayed image")
            (
                "translate_x,tx",
                po::value<float>(&result.translate_x)->default_value(0.0f),
                "set x translation of displayed image"
            )
            (
                "translate_y,ty",
                po::value<float>(&result.translate_y)->default_value(0.0f),
                "set y translation of displayed image"
            )
            ("file,f", po::value<std::string>(&result.file_name)->required(), "the file to display");

        po::positional_options_description p;
        p.add("file", 1);
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return std::nullopt;
        }

        po::notify(vm);

        if (!std::filesystem::exists(result.file_name)) {
            std::cout << fmt::format("Specified file \"{}\" does not exists", result.file_name) << std::endl;
            return std::nullopt;
        }
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return std::nullopt;
    } catch (...) {
        std::cerr << "Unknown error!" << "\n";
        return std::nullopt;
    }

    return result;
}

int main(int argc, char* argv[]) {
    // setup logging
    mff::logger::vulkan = mff::logger::setup_vulkan_logging();
    mff::logger::window = mff::logger::setup_window_logging();

    // parse options
    auto options = parse_command_line_options(argc, argv);

    if (!options) return -1;

    // run everything in boost leaf context
    return boost::leaf::try_handle_all(
        [&]() -> boost::leaf::result<int> {
            LEAF_CHECK(run(options.value()));

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