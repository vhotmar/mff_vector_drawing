#include "./context.h"
#include "../../utils/logger.h"

namespace mff::window::detail {

void error_callback(int error, const char* description) {
    logger::system->error("Error ({0}): \"{1}\"", error, description);
}

GLFWContext::GLFWContext() {
    glfwSetErrorCallback(error_callback);

    logger::system->trace("GLFW error callback set");

    glfwInit();

    logger::system->trace("GLFW initialized");
}

GLFWContext::~GLFWContext() {
    glfwTerminate();
    logger::system->trace("GLFW terminated");
}

std::shared_ptr<GLFWContext> create_glfw_context() {
    // basically what this does is that there will exists exactly one
    // context at any given time

    // we will create static (global) weak_ptr, which will hold the
    // reference to the context
    static std::weak_ptr<GLFWContext> instance;

    auto res = instance.lock();

    // if there is no current context, then create it
    if (!res) {
        res = std::shared_ptr<GLFWContext>(new GLFWContext());

        // and assign it to the weak_ptr (so when the returned shared_ptr
        // go out of scope the context is going to be destroyed
        instance = res;
    }

    return res;
}

}