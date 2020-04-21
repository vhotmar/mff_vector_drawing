#pragma once

#include "../glfw.h"

namespace mff::window::detail {

void error_callback(int error, const char* description);

/**
 * This is simple wrapper around GLFW context (glfwInit / glfwTerminate)
 *
 * Requirements
 * - double initialization is faulty
 * - we do not want to keep the GLFW context "alive" when not needed
 */
class GLFWContext {
public:
    GLFWContext();

    GLFWContext(const GLFWContext&) = delete;
    GLFWContext& operator=(const GLFWContext&) = delete;

    ~GLFWContext();

    friend std::shared_ptr<GLFWContext> create_glfw_context();
};

std::shared_ptr<GLFWContext> create_glfw_context();

}