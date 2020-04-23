#pragma once

#include <optional>

#include <mff/leaf.h>
#include <mff/graphics/math.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace mff::window {

/**
 * Simple abstraction over window
 */
class Window {
public:
    /**
     * @brief Given vulkan instance return vulkan surface (low-level API for now)
     * @param instance the vulkan instance
     * @return result of building the vulkan surface
     */
    virtual boost::leaf::result<vk::UniqueSurfaceKHR> create_surface(vk::Instance instance) = 0;

    /**
     * @brief Get vulkan required extensions
     * @return the vulkan extensions
     */
    virtual std::vector<std::string> get_required_extensions() = 0;

    /**
     * @brief Get current window size
     * @return the window size
     */
    virtual Vector2ui get_inner_size() = 0;
};

};