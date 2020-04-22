#pragma once

#include <variant>

#include "../../eigen.h"
#include "../../leaf.h"
#include "../../utils.h"
#include "../../vulkan.h"

#include "./device.h"
#include "./render_pass.h"

namespace mff::vulkan {

/**
 * Describes how stencil should be used
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap27.html#VkStencilOpState
 */
struct Stencil {
    /**
     * The comparison to perform between the existing stencil value in the stencil buffer, and
     * the reference value (given by `reference`).
     */
    vk::CompareOp compare = vk::CompareOp::eNever;

    /**
     * Value specifying the action performed on samples that pass both the depth and stencil tests.
     */
    vk::StencilOp pass_op = vk::StencilOp::eKeep;

    /**
     * Value specifying the action performed on samples that fail the stencil test.
     */
    vk::StencilOp fail_op = vk::StencilOp::eKeep;

    /**
     * Value specifying the action performed on samples that pass the stencil test and fail the
     * depth test.
     */
    vk::StencilOp depth_fail_op = vk::StencilOp::eKeep;

    /**
     * Selects the bits of the unsigned integer stencil values participating in the stencil test.
     *
     * When std::nullopt then needs to be set dynamically
     */
    std::optional<std::uint32_t> compare_mask = std::numeric_limits<std::uint32_t>::max();

    /**
     * Selects the bits of the unsigned integer stencil values updated by the stencil test in the
     * stencil framebuffer attachment.
     *
     * When std::nullopt then needs to be set dynamically
     */
    std::optional<std::uint32_t> write_mask = std::numeric_limits<std::uint32_t>::max();

    /**
     * An integer reference value that is used in the unsigned stencil comparison.
     *
     * When std::nullopt then needs to be set dynamically
     */
    std::optional<std::uint32_t> reference = std::numeric_limits<std::uint32_t>::max();

    /**
     * Does stencil op always result in keep?
     */
    bool always_keep();

    vk::StencilOpState to_vulkan();
};

namespace DepthBounds_ {

/**
 * There is no depth testing
 */
struct Disabled {};

/**
 * The depth testing range is fixed
 */
struct Fixed {
    float from;
    float to;
};

/**
 * The depth testing is dynamic and will be set during render
 */
struct Dynamic {};

}

/**
 * Describe depth bound testing part of DepthStencil. It allows you to ask the GPU to exclude
 * fragments that are outside of a certain range.
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap27.html#VkPipelineDepthStencilStateCreateInfo
 */
using DepthBounds = std::variant<DepthBounds_::Disabled, DepthBounds_::Fixed, DepthBounds_::Dynamic>;

/**
 * Configuration of depth and stencil tests.
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap27.html#fragops
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap27.html#VkPipelineDepthStencilStateCreateInfo
 */
struct DepthStencil {
    /**
     * Comparison to use between depth of current fragment and value in current depth buffer.
     */
    vk::CompareOp depth_compare = vk::CompareOp::eAlways;

    /**
     * If the depth comparison was succesful should it be written to depth buffer?
     */
    bool depth_write = false;

    /**
     * Allows you to ask the GPU to exclude fragments that are outside of a certain range.
     */
    DepthBounds depth_bounds_test = DepthBounds_::Disabled{};

    /**
     * Stencil operations to use for points, lines and triangles whose front is facing the user.
     */
    Stencil stencil_front;

    /**
     * Stencil operations to use for points, lines and triangles whose back is facing the user.
     */
    Stencil stencil_back;

    vk::PipelineDepthStencilStateCreateInfo to_vulkan() const;
};

namespace vertex {

struct BufferItem {
    std::uint32_t binding;
    std::size_t stride;
    vk::VertexInputRate input_rate;
};

struct AttributesInfo {
    std::uint32_t offset;
    vk::Format format;
};

struct AttributesItem {
    std::uint32_t location;
    std::uint32_t binding;
    AttributesInfo info;
};

struct DefinitionInfo {
    std::vector<BufferItem> buffers;
    std::vector<AttributesItem> attributes;
};

}

namespace shader {

class ShaderModule {
private:
    vk::UniqueShaderModule handle_;
    std::shared_ptr<Device> device_;

public:
    vk::ShaderModule get_handle();
};

struct OutputDefinition {};
struct InputDefinition {};
struct GraphicsEntryPoint {
    InputDefinition input;
    OutputDefinition output;
    int type;
    ShaderModule module;
    std::string name;
};

}

/**
 * Description of how should be the draw operation executed.
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap9.html#pipelines-graphics
 */
class GraphicsPipeline {
private:
    vk::UniquePipeline handle_;
    std::shared_ptr<Device> device_;

    GraphicsPipeline() = default;
};


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

struct DepthBias {
    float constant_factor;
    float clamp;
    float slope_factor;
};

namespace DepthBiasControl_ {

struct Disabled {};

struct Static {
    DepthBias bias;
};

}

using DepthBiasControl = std::variant<DepthBiasControl_::Disabled, DepthBiasControl_::Static>;

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

    vk::PipelineRasterizationStateCreateInfo to_vulkan() const;
};

/**
 * Describe how the blending system should behave for individual attachments
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap28.html#VkPipelineColorBlendAttachmentState
 */
struct AttachmentBlend {
    bool enabled;

    vk::BlendOp color_op;
    vk::BlendFactor color_source;
    vk::BlendFactor color_destination;

    vk::BlendOp alpha_op;
    vk::BlendFactor alpha_source;
    vk::BlendFactor alpha_destination;

    bool mask_red;
    bool mask_green;
    bool mask_blue;
    bool mask_alpha;

    vk::PipelineColorBlendAttachmentState to_vulkan() {
        vk::ColorComponentFlags flags;

        if (mask_red) {
            flags |= vk::ColorComponentFlagBits::eR;
        }

        if (mask_alpha) {
            flags |= vk::ColorComponentFlagBits::eA;
        }

        if (mask_green) {
            flags |= vk::ColorComponentFlagBits::eG;
        }

        if (mask_blue) {
            flags |= vk::ColorComponentFlagBits::eB;
        }

        return vk::PipelineColorBlendAttachmentState(
            enabled,
            color_source,
            color_destination,
            color_op,
            alpha_source,
            alpha_destination,
            alpha_op,
            flags
        );
    }
};

struct AttachmentsBlendCollective {
    AttachmentBlend blend;
};

struct AttachemntsBlendIndividual {
    std::vector<AttachmentBlend> blends;
};

using AttachmentsBlend = std::variant<AttachmentsBlendCollective, AttachemntsBlendIndividual>;

struct Blend {
    std::optional<vk::LogicOp> logic_op;
    AttachmentsBlend attachments;
    std::optional<Vector4f> blend_constants;
};

class GraphicsPipelineBuilder {
private:
    vertex::DefinitionInfo vertex_input;
    shader::GraphicsEntryPoint vertex_shader;
    shader::GraphicsEntryPoint fragment_shader;
    vk::PipelineInputAssemblyStateCreateInfo input_assembly;
    vk::PrimitiveTopology input_assembly_topology;
    std::optional<ViewportsState> viewport;
    Rasterization raster;
    vk::PipelineMultisampleStateCreateInfo multisample;
    DepthStencil depth_stencil;
    Blend blend;
    std::optional<Subpass> render_pass;

public:
    boost::leaf::result<std::shared_ptr<GraphicsPipeline>> build(std::shared_ptr<Device> device);
};

}