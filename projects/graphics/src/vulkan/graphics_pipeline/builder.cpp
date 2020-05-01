#include <mff/graphics/vulkan/graphics_pipeline/builder.h>

#include <mff/graphics/utils.h>

namespace mff::vulkan {

boost::leaf::result<std::unique_ptr<GraphicsPipeline>> GraphicsPipelineBuilder::build(const Device* device) {
    // TODO: missing pipeline layout customization + tesselation/geometry stage
    LEAF_AUTO(layout, PipelineLayout::build(device, vertex_shader.layout.make_union(fragment_shader.layout)));
    // TODO: check that the layout conforms the stages

    std::vector<vk::DynamicState> dynamics;

    // TODO: missing specializations
    // TODO: check shader types (do not blindly asume that they are corret)
    std::vector<vk::PipelineShaderStageCreateInfo> stages{
        vk::PipelineShaderStageCreateInfo(
            {},
            vk::ShaderStageFlagBits::eVertex,
            vertex_shader.module->get_handle(),
            vertex_shader.name.c_str()
        ),
        vk::PipelineShaderStageCreateInfo(
            {},
            vk::ShaderStageFlagBits::eFragment,
            fragment_shader.module->get_handle(),
            fragment_shader.name.c_str()
        ),
    };

    // TODO: check if vertex_input bind stride is not too large + attribute offset is not too large + their count is not too large (for device)

    // TODO: check if multi viewport is supported + its upper limit for device
    // TODO: check viewports dimensions - their upper and lower limits

    if (std::holds_alternative<ViewportsState_::Dynamic>(viewport.value())) {
        dynamics.push_back(vk::DynamicState::eViewport);
        dynamics.push_back(vk::DynamicState::eScissor);
    }

    if (!raster.line_width) dynamics.push_back(vk::DynamicState::eLineWidth);

    // TODO: check depth clamp support
    // TODO: check that stancil_front && stencil_back masks are both some or none

    if (std::holds_alternative<DepthBounds_::Dynamic>(depth_stencil.depth_bounds_test)) {
        dynamics.push_back(vk::DynamicState::eDepthBounds);
    }

    if (!depth_stencil.stencil_front.compare_mask && !depth_stencil.stencil_back.compare_mask) {
        dynamics.push_back(vk::DynamicState::eStencilCompareMask);
    }

    if (!depth_stencil.stencil_front.write_mask && !depth_stencil.stencil_back.write_mask) {
        dynamics.push_back(vk::DynamicState::eStencilWriteMask);
    }

    if (!depth_stencil.stencil_front.reference && !depth_stencil.stencil_back.reference) {
        dynamics.push_back(vk::DynamicState::eStencilReference);
    }

    if (!blend.blend_constants) {
        dynamics.push_back(vk::DynamicState::eBlendConstants);
    }

    std::optional<vk::PipelineDynamicStateCreateInfo> dynamic_info = std::nullopt;

    if (!dynamics.empty()) {
        dynamic_info = vk::PipelineDynamicStateCreateInfo({}, dynamics.size(), dynamics.data());
    }

    multisample.rasterizationSamples = render_pass.value()->get_samples();

    auto vertex_input_state = vertex_input.to_vulkan();
    auto viewport_info = to_vulkan(viewport.value());
    auto rasterization_info = raster.to_vulkan();
    auto depth_stencil_info = depth_stencil.to_vulkan();
    auto blend_info = blend.to_vulkan(render_pass.value());

    auto pipeline_info = vk::GraphicsPipelineCreateInfo(
        {},
        stages.size(),
        stages.data(),
        &vertex_input_state,
        &input_assembly,
        nullptr,
        &viewport_info,
        &rasterization_info,
        &multisample,
        &depth_stencil_info,
        &blend_info,
        dynamic_info.has_value() ? &dynamic_info.value() : nullptr,
        layout->get_handle(),
        render_pass.value()->get_render_pass()->get_handle(),
        render_pass.value()->get_index()
    );

    struct enable_GraphicsPipeline : GraphicsPipeline {};
    std::unique_ptr<GraphicsPipeline> result = std::make_unique<enable_GraphicsPipeline>();

    result->device_ = device;
    result->pipeline_layout_ = std::move(layout);
    LEAF_AUTO_TO(result->pipeline_,
                 to_result(device->get_handle().createGraphicsPipelineUnique(nullptr, pipeline_info)));

    return result;
}
}
