#pragma once

#include <memory>
#include <string>
#include <utility>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../utils/vulkan.h"
#include "../utils/logger.h"
#include "../utils/expected.h"

namespace glfw {

#define REQUIRE_CONTEXT(var)                           \
    if (!var)                                          \
    {                                                  \
        return tl::make_unexpected("Context is empty");\
    }

////////////////////////////////
//////////// Context ///////////
////////////////////////////////

namespace detail {

void error_callback(int error, const char* description)
{
    logger::system->error("Error ({0}): \"{1}\"", error, description);
}

class context_detail {
public:
    context_detail() {
        glfwSetErrorCallback(error_callback);

        logger::system->trace("GLFW error callback set");

        glfwInit();

        logger::system->trace("GLFW initialized");
    }

    context_detail(const context_detail&) = delete;

    context_detail& operator=(const context_detail&) = delete;

    ~context_detail() {
        glfwTerminate();
        logger::system->trace("GLFW terminated");
    }

    friend std::shared_ptr<context_detail> create_context_detail();
};

std::shared_ptr<context_detail> create_context_detail() {
    // basically what this does is that there will exists exactly one
    // context at any given time

    // we will create static (global) weak_ptr, which will hold the
    // reference to the context
    static std::weak_ptr<context_detail> instance;

    auto res = instance.lock();

    // if there is no current context, then create it
    if (!res) {
        res = std::shared_ptr<context_detail>(new context_detail());

        // and assign it to the weak_ptr (so when the returned shared_ptr
        // go out of scope the context is going to be destroyed
        instance = res;
    }

    return res;
}

}

using context = detail::context_detail;

std::shared_ptr<context> create_context() {
    return detail::create_context_detail();
}


////////////////////////////////
/////// Context functions //////
////////////////////////////////

enum class WindowHint {
    // ...
    RedBits = GLFW_RED_BITS,
    Resizable = GLFW_RESIZABLE,
    // ...
    ClientApi = GLFW_CLIENT_API,
};

tl::expected<bool, std::string> vulkan_supported(const std::shared_ptr<context>& c) {
    REQUIRE_CONTEXT(c);

    return glfwVulkanSupported();
}

tl::expected<void, std::string> window_hint(const std::shared_ptr<context>& c, WindowHint hint, int value) {
    REQUIRE_CONTEXT(c);

    glfwWindowHint(static_cast<int>(hint), value);

    return {};
}

tl::expected<std::vector<std::string>, std::string> get_required_instance_extensions(const std::shared_ptr<context>& c) {
    REQUIRE_CONTEXT(c);

    unsigned int glfwExtensionCount;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    return std::vector<std::string>(glfwExtensions, glfwExtensions + glfwExtensionCount);
}

tl::expected<void, std::string> poll_events(const std::shared_ptr<context>& c) {
    REQUIRE_CONTEXT(c);

    glfwPollEvents();

    return {};
}

////////////////////////////////
//////////// Window ////////////
////////////////////////////////

namespace detail {

struct destroy_GLFWwindow {
    void operator()(GLFWwindow* ptr) {
        glfwDestroyWindow(ptr);
    }
};

using window_unique = std::unique_ptr<GLFWwindow, destroy_GLFWwindow>;

}

class window {
public:
    window(
        std::shared_ptr<context> context,
        int width,
        int height,
        const std::string& title,
        GLFWmonitor* monitor,
        GLFWwindow* share
    )
        : context_(std::move(context))
        , width_(width)
        , height_(height)
        , title_(title)
        , window_(glfwCreateWindow(width, height, title.c_str(), monitor, share)) {}

    bool should_close() {
        return glfwWindowShouldClose(window_.get());
    }

    tl::expected<vk::UniqueSurfaceKHR, std::string> create_surface(vk::Instance instance, const VkAllocationCallbacks* allocator) {
        VkSurfaceKHR surface;

        auto result = glfwCreateWindowSurface(instance, window_.get(), allocator, &surface);

        if (result != VK_SUCCESS) {
            return tl::make_unexpected("Can't create a surface");
        }

        vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> _deleter(instance);
        return vk::UniqueSurfaceKHR(surface, _deleter);
    }

    int initial_width() {
        return width_;
    }

    int initial_height() {
        return height_;
    }

    vk::Extent2D framebuffer_extent() {
        int width, height;
        glfwGetFramebufferSize(window_.get(), &width, &height);

        return vk::Extent2D{
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
    }

    void set_user_pointer(void* pointer) {
        glfwSetWindowUserPointer(window_.get(), this);
    }

    void set_framebuffer_size_callback(GLFWframebuffersizefun fn) {
        glfwSetFramebufferSizeCallback(window_.get(), fn);
    }

    template<typename TOStream>
    friend TOStream &operator<<(TOStream &os, const window &win)
    {
        utils::debug::format::DebugStruct debug(os, "window");

        return debug.field("width", win.width_).field("height", win.height_).field("title", win.title_).finish();
    }

private:
    // we have guarantee by 12.6.2 that it will go out of scope in order
    int width_;
    int height_;
    std::string title_;
    std::shared_ptr<context> context_;
    detail::window_unique window_;
};

tl::expected<std::unique_ptr<window>, std::string> create_window(
    const std::shared_ptr<context>& context,
    int width,
    int height,
    const std::string& title,
    GLFWmonitor* monitor,
    GLFWwindow* share
) {
    REQUIRE_CONTEXT(context);

    return std::make_unique<window>(context, width, height, title, monitor, share);
};

tl::expected<void, std::string> wait_events(const std::shared_ptr<context>& context) {
    REQUIRE_CONTEXT(context);

    glfwPollEvents();

    return {};
}

}