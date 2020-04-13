#pragma once

#include <optional>

#include "../leaf.h"
#include "./events.h"
#include "./event_loop.h"
#include "./context.h"

namespace mff::internal::window {

namespace detail {

struct DestroyGLFWwindow {
    void operator()(GLFWwindow* ptr);
};

using glfw_window = std::unique_ptr<GLFWwindow, DestroyGLFWwindow>;

void window_close_callback(GLFWwindow* glfw_window);
void framebuffer_size_callback(GLFWwindow* glfw_window, int width, int height);

}

class Window {
public:
    Window(std::shared_ptr<detail::GLFWContext> context, detail::glfw_window handle, EventLoop* loop);

    boost::leaf::result<vk::UniqueSurfaceKHR>
    create_surface(vk::Instance instance, const VkAllocationCallbacks* allocator);

    std::vector<std::string> get_required_extensions();

    void dispatch(events::Event event);

private:
    void setup_callbacks();

private:
    // Order dependent!!!
    std::shared_ptr<detail::GLFWContext> context_;
    detail::glfw_window handle_;
    EventLoop* event_loop_;
};

struct WindowSize {
    int width;
    int height;

    WindowSize(int width, int height);
};

class WindowBuilder {
public:
    WindowBuilder();

    WindowBuilder& with_title(std::string title);
    WindowBuilder& with_size(WindowSize size);
    std::unique_ptr<Window> build(EventLoop* loop);

private:
    WindowSize get_size();

private:
    bool resizable_ = false;
    std::optional<WindowSize> size_;
    std::string title_;
};

};