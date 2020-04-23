#pragma once

#include <variant>
#include <vector>

#include <mff/graphics/math.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

/**
 * State of viewport
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap25.html#VkViewport
 */
struct Viewport {
    /**
     * Coordinates of top-left hand corner of viewport
     */
    Vector2f origin;

    /**
     * Viewport width and height
     */
    Vector2f dimensions;

    /**
     * Minimum and maximum range of depth values for viewport.
     */
    float depth_range_from;
    float depth_range_to;

    /**
     * Convert this object to vulkan representation
     * @return vulkan representation
     */
    vk::Viewport to_vulkan() const;
};

/**
 * State of scissor box
 */
struct Scissor {
    /**
     * Coordinates in pixels of the top-left hand corner of the box.
     */
    Vector2ui origin;

    /**
     * Dimensions in pixels of the box.
     */
    Vector2ui dimensions;

    /**
     * Convert this object to vulkan representation
     * @return vulkan representation
     */
    vk::Rect2D to_vulkan() const;
};

namespace ViewportsState_ {

/**
 * Everything should be static / predefined
 */
struct Fixed {
    std::vector<std::tuple<Viewport, Scissor>> data;
};

/**
 * Vieports and scissors are defined during draw
 */
struct Dynamic {
    std::uint32_t num;
};

}

/**
 * List of viewports and scissors that are used when creating a graphics pipeline object.
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap25.html#VkPipelineViewportStateCreateInfo
 */
using ViewportsState = std::variant<ViewportsState_::Fixed, ViewportsState_::Dynamic>;

/**
 * Convert viewport state to vulkan representation
 * @param state vieport stat to be converted
 * @return vulkan representation
 */
vk::PipelineViewportStateCreateInfo to_vulkan(const ViewportsState& state);

}