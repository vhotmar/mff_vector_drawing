#pragma once

#include <memory>

#include "../glfw.h"
#include "../leaf.h"

#include "./event_loop.h"
#include "./events.h"
#include "./window.h"

/**
 * Concrete window implementation using GLFW
 */
namespace mff::window::glfw {

/**
 * Here are some internal parts for working with GLFW (add RAII and callbacks)
 */
namespace detail {

/**
 * GLFW error callback
 * @param error
 * @param description
 */
void error_callback(int error, const char* description);

/**
 * This is simple wrapper around GLFW context (glfwInit / glfwTerminate)
 *
 * Requirements
 * - double initialization is faulty
 * - we do not want to keep the GLFW context "alive" when not needed
 */
class GLFWContext {
private:
    GLFWContext();

    // Can't be copied
    GLFWContext(const GLFWContext&) = delete;

    GLFWContext& operator=(const GLFWContext&) = delete;

    ~GLFWContext();

public:
    static std::shared_ptr<GLFWContext> create();
};

struct DestroyGLFWwindow {
    void operator()(GLFWwindow* ptr);
};

/**
 * Wrap GLFWwindow with unique_ptr so we do not have to worry about destroying the window manually
 */
using UniqueGLFWwindow = std::unique_ptr<GLFWwindow, DestroyGLFWwindow>;

// GLFW callbacks
void window_close_callback(GLFWwindow* glfw_window);

void framebuffer_size_callback(GLFWwindow* glfw_window, int width, int height);

}

class WindowBuilder;

/**
 * Concrete window implementation
 */
class Window : public mff::window::Window {
private:
    Window() = default;

public:
    /**
     * @brief Given vulkan instance return vulkan surface (low-level API for now)
     * @param instance the vulkan instance
     * @return result of building the vulkan surface
     */
    boost::leaf::result<vk::UniqueSurfaceKHR> create_surface(vk::Instance instance);

    /**
     * @brief Get vulkan required extensions
     * @return the vulkan extensions
     */
    std::vector<std::string> get_required_extensions();

    /**
     * @brief Get current window size
     * @return the window size
     */
    Vector2ui get_inner_size();

private:
    void setup_callbacks();

    void dispatch(events::Event event);

    // Order dependent!!!
    std::shared_ptr<detail::GLFWContext> context_;
    detail::UniqueGLFWwindow handle_;
    EventLoop* event_loop_;

    friend class WindowBuilder;

    friend void detail::window_close_callback(GLFWwindow* glfw_window);

    friend void detail::framebuffer_size_callback(GLFWwindow* glfw_window, int width, int height);
};

/**
 * There are (will) be multiple values not needed for window creation - only way to initialize
 * window is through this builder
 */
class WindowBuilder {
public:
    /**
     * @brief Set the title of window
     * @param title title to be set
     * @return builder
     */
    WindowBuilder& with_title(std::string title);

    /**
     * @brief Set the size of window
     * @param size size to be set
     * @return builder
     */
    WindowBuilder& with_size(Vector2ui size);

    /**
     * @brief Build the window which will generate events passed to the event loop
     * @param loop
     * @return result of building the window
     */
    boost::leaf::result<std::shared_ptr<mff::window::Window>> build(EventLoop* loop);

private:
    Vector2ui get_size();

    bool resizable_ = false;
    std::optional<Vector2ui> size_ = std::nullopt;
    std::string title_ = "";
};

}