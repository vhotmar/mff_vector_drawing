#include "./window.h"

#include <utility>

#include "../../utils/logger.h"

namespace mff::window {

Window::Window(std::shared_ptr<detail::GLFWContext> context, detail::glfw_window handle, EventLoop* loop)
    : context_(std::move(context)), handle_(std::move(handle)), event_loop_(loop) {
    glfwSetWindowUserPointer(handle_.get(), this);

    setup_callbacks();
}

void detail::window_close_callback(GLFWwindow* glfw_window) {
    auto win = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfw_window));

    win->dispatch(events::Event(events::WindowEvent{events::window::CloseRequested{}}));
}

void detail::framebuffer_size_callback(GLFWwindow* glfw_window, int width, int height) {
    auto win = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfw_window));

    win->dispatch(events::Event(events::WindowEvent{events::window::CloseRequested{}}));
}

void Window::setup_callbacks() {
    glfwSetWindowCloseCallback(handle_.get(), detail::window_close_callback);
    glfwSetFramebufferSizeCallback(handle_.get(), detail::framebuffer_size_callback);
}

void Window::dispatch(events::Event event) {
    event_loop_->dispatch(event);
}

boost::leaf::result<vk::UniqueSurfaceKHR> Window::create_surface(vk::Instance instance) {
    VkSurfaceKHR surface;

    auto result = glfwCreateWindowSurface(instance, handle_.get(), nullptr, &surface);

    if (result != VK_SUCCESS) {
        return boost::leaf::new_error(static_cast<vk::Result>(result));
    }

    vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> _deleter(instance);
    return vk::UniqueSurfaceKHR(surface, _deleter);
}

std::vector<std::string> Window::get_required_extensions() {
    auto context = window::detail::create_glfw_context();

    unsigned int glfwExtensionCount;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    return std::vector<std::string>(glfwExtensions, glfwExtensions + glfwExtensionCount);
}

Vector2ui Window::get_inner_size() {
    int width;
    int height;
    glfwGetFramebufferSize(handle_.get(), &width, &height);
    return mff::Vector2ui(width, height);
}

WindowSize::WindowSize(int width_, int height_)
    : width(width_), height(height_) {
}

WindowBuilder::WindowBuilder() = default;

WindowBuilder& WindowBuilder::with_title(std::string title) {
    title_ = std::move(title);

    return *this;
}

WindowBuilder& WindowBuilder::with_size(WindowSize size) {
    size_ = size;

    return *this;
}

WindowSize WindowBuilder::get_size() {
    if (!size_) {
        auto monitor = glfwGetPrimaryMonitor();
        auto mode = glfwGetVideoMode(monitor);

        return WindowSize(mode->width, mode->height);
    }

    return *size_;
}

std::shared_ptr<Window> WindowBuilder::build(EventLoop* loop) {
    auto context = detail::create_glfw_context();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, resizable_);

    auto size = get_size();

    logger::system->trace("GLFW creating new window");

    detail::glfw_window win(glfwCreateWindow(size.width, size.height, title_.c_str(), nullptr, nullptr));

    logger::system->trace("GLFW new window created");

    return std::make_shared<Window>(context, std::move(win), loop);
}

void detail::DestroyGLFWwindow::operator()(GLFWwindow* ptr) {
    logger::system->trace("GLFW destroying window");

    glfwDestroyWindow(ptr);

    logger::system->trace("GLFW window destroyed");
}

}
