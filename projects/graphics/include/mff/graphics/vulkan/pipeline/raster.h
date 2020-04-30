#pragma once

#include <optional>
#include <variant>

#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

/**
 * Control of depth bias
 */
struct DepthBias {
    float constant_factor;
    float clamp;
    float slope_factor;
};

class DepthBiasControl {
public:
    struct Disabled {};

    struct Static {
        DepthBias bias;
    };

    using type = std::variant<Disabled, Static>;

private:
    type inner_;

public:
    const type& get_inner() const;
};

/**
 * State of rasterizer
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap26.html#VkPipelineRasterizationStateCreateInfo
 */
struct Rasterization {
    /**
     * If true, then the depth value of the vertices will be clamped to [0.0 ; 1.0]. If false,
     * fragments whose depth is outside of this range will be discarded.
     */
    bool depth_clamp = false;

    /**
     * If true, all the fragments will be discarded. This is usually used when your vertex shader
     * has some side effects and you don't need to run the fragment shader.
     */
    bool rasterizer_discard = false;

    /**
     * This setting can ask the rasterizer to downgrade triangles into lines or points.
     */
    vk::PolygonMode polygon_mode = vk::PolygonMode::eFill;

    /**
     * Specifies whether front faces or back faces should be discarded, or none, or both.
     */
    vk::CullModeFlags cull_mode = vk::CullModeFlags(vk::CullModeFlagBits::eNone);

    /**
     * Specifies which triangle orientation corresponds to the front or the triangle.
     */
    vk::FrontFace front_face = vk::FrontFace::eClockwise;

    /**
     * Width, in pixels, of lines when drawing lines.
     *
     * If std::nullopt then used as dynamic value.
     */
    std::optional<float> line_width = 1.0f;

    DepthBiasControl depth_bias = DepthBiasControl_::Disabled();

    /**
     *
     * @return
     */
    vk::PipelineRasterizationStateCreateInfo to_vulkan() const;
};

}