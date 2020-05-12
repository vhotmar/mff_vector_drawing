#pragma once

#include <memory>

#include <mff/leaf.h>
#include <mff/graphics/vulkan/render_pass.h>

#include "./vulkan_engine.h"
#include "./vulkan_presenter.h"

const std::uint32_t kSTENCIL_CLIP_BIT = 0x1;
const std::uint32_t kSTENCIL_FILL_BIT = 0x2;
const std::uint32_t kSTENCIL_ALL_BIT = 0x3;

/**
 * Vertex ot use
 */
struct Vertex {
    mff::Vector2f pos;

    static vk::VertexInputBindingDescription get_binding_description(std::uint32_t binding = 0);
    static std::array<vk::VertexInputAttributeDescription, 1> get_attribute_descriptions();
};

/**
 * Push constants
 */
struct PushConstants {
    mff::Vector4f color = mff::Vector4f::Ones();
    mff::Matrix2f transform = mff::Matrix2f::Identity();
    mff::Vector2f scale = mff::Vector2f::Zero();
};

/**
 * Class which encapsulates all the common rendering behaviour for our renderer
 *
 * - Vulkan render passes
 * - Vulkan pipelines
 */
class RendererContext {
public:
    /**
     * Build the renderer context
     * @param engine on which vulkan engine
     * @param color_format which color format to use
     * @return
     */
    static boost::leaf::result<std::unique_ptr<RendererContext>> build(
        VulkanEngine* engine,
        vk::Format color_format
    );

    /**
     * Get the main render pass
     * @return
     */
    const mff::vulkan::RenderPass* get_renderpass() const;

    /**
     * Get device used by RendererContext
     * @return
     */
    mff::vulkan::Device* get_device();

    /**
     * Get pipeline layout
     * @return
     */
    vk::PipelineLayout get_pipeline_layout();

    /**
     * Get the color attachment format
     * @return
     */
    vk::Format get_color_attachment_format() const;

    /**
     * Get the stencil attachment format
     * @return
     */
    vk::Format get_stencil_attachment_format() const;

    /**
     * Get pipeliene which will write over data already specified in image (without cleaning first)
     * @return
     */
    vk::Pipeline get_over_pipeline();

private:
    RendererContext() = default;

    // Helper functions
    boost::leaf::result<mff::vulkan::UniqueRenderPass> build_render_pass(
        vk::AttachmentLoadOp load_op,
        vk::AttachmentLoadOp stencil_load_op
    );
    boost::leaf::result<void> build_pipeline_layout();
    boost::leaf::result<void> build_pipelines();

    // Most of our pipelines are the much same except few informations
    struct BuildPipelineInfo {
        vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
        vk::StencilOpState stencil_op = vk::StencilOpState();

        /**
         * Color mask to use
         */
        vk::ColorComponentFlags color_mask = vk::ColorComponentFlagBits::eR
            | vk::ColorComponentFlagBits::eG
            | vk::ColorComponentFlagBits::eB
            | vk::ColorComponentFlagBits::eA;

        /**
         * Number of first of these dynamic states:
         * - Viewport
         * - Scissor
         * - StencilCompareMask
         * - StencilReference
         * - StencilWriteMask
         */
        std::uint32_t dynamics_count = 5;

        /**
         * Which stages to use?
         *
         * 1 - only vertex
         * 2 - vertex + fragment
         */
        std::uint32_t stages_count = 2;

        /**
         * Color blending to use
         */
        vk::BlendOp blend_op = vk::BlendOp::eAdd;

        /**
         * Should we color blending
         */
        bool blend_enabled = true;
    };

    // Helper function
    boost::leaf::result<vk::UniquePipeline> build_pipeline(BuildPipelineInfo info);

    VulkanEngine* engine_;

    /*
     * The main render pass with which we render to surface
     */
    mff::vulkan::UniqueRenderPass render_pass_main_ = nullptr;

    vk::UniquePipelineLayout pipeline_layout_;

    /**
     * Same as render passes - future proofing
     */
    vk::UniquePipeline pipeline_over_;

    // used color format
    vk::Format color_format_;

    // used stencil format
    vk::Format stencil_format_;
};