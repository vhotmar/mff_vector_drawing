#include <mff/graphics/logger.h>
#include <mff/graphics/window/glfw_window.h>

#include "../utils.h"

namespace mff::window::glfw {

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
    // we keep the context initialized when window is initialized
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

WindowBuilder& WindowBuilder::with_title(std::string title) {
    title_ = std::move(title);

    return *this;
}

WindowBuilder& WindowBuilder::with_size(Vector2ui size) {
    size_ = size;

    return *this;
}

Vector2ui WindowBuilder::get_size() {
    if (!size_) {
        auto monitor = glfwGetPrimaryMonitor();
        auto mode = glfwGetVideoMode(monitor);

        return Vector2ui(mode->width, mode->height);
    }

    return size_.value();
}

boost::leaf::result<std::shared_ptr<mff::window::Window>> WindowBuilder::build(EventLoop* loop) {
    auto context = detail::GLFWContext::create();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, resizable_);

    auto size = get_size();

    logger::window->trace("GLFW creating new window");

    detail::UniqueGLFWwindow win(glfwCreateWindow(size[0], size[1], title_.c_str(), nullptr, nullptr));

    logger::window->trace("GLFW new window created");

    struct enable_Window : public Window {};
    auto window = std::make_shared<enable_Window>();

    window->handle_ = std::move(win);
    window->context_ = context;
    window->event_loop_ = loop;

    glfwSetWindowUserPointer(window->handle_.get(), window.get());

    window->setup_callbacks();

    return std::shared_ptr<mff::window::Window>(window);
}

namespace detail {

void DestroyGLFWwindow::operator()(GLFWwindow* ptr) {
    logger::window->trace("GLFW destroying window");

    glfwDestroyWindow(ptr);

    logger::window->trace("GLFW window destroyed");
}

void error_callback(int error, const char* description) {
    logger::window->error("Error ({0}): \"{1}\"", error, description);
}

GLFWContext::GLFWContext() {
    glfwSetErrorCallback(error_callback);

    logger::window->trace("GLFW error callback set");

    glfwInit();

    logger::window->trace("GLFW initialized");
}

GLFWContext::~GLFWContext() {
    glfwTerminate();
    logger::window->trace("GLFW terminated");
}

std::shared_ptr<GLFWContext> GLFWContext::create() {
    // basically what this does is that there will exists exactly one
    // context at any given time

    // we will create static (global) weak_ptr, which will hold the
    // reference to the context
    static std::weak_ptr<GLFWContext> instance;

    auto res = instance.lock();

    // if there is no current context, then create it
    if (!res) {
        struct enable_GLFWContext : public GLFWContext {};
        res = std::shared_ptr<GLFWContext>(new enable_GLFWContext());

        // and assign it to the weak_ptr (so when the returned shared_ptr
        // go out of scope the context is going to be destroyed
        instance = res;
    }

    return res;
}

void window_close_callback(GLFWwindow* glfw_window) {
    auto win = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfw_window));

    win->dispatch(events::Event(events::WindowEvent{events::window::CloseRequested{}}));
}

void framebuffer_size_callback(GLFWwindow* glfw_window, int width, int height) {
    auto win = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfw_window));

    win->dispatch(events::Event(events::WindowEvent{events::window::CloseRequested{}}));
}

}

}