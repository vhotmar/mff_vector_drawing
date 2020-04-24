#include "./graphics_pipeline.h"
#include "graphics_pipeline.h"

#include <utility>
#include <mff/algorithms.h>
#include <mff/utils.h>

namespace mff::vulkan {

boost::leaf::result<std::shared_ptr<GraphicsPipeline>> GraphicsPipelineBuilder::build(std::shared_ptr<Device> device) {
    std::vector<vk::DynamicState> dynamics;

    std::vector<vk::PipelineShaderStageCreateInfo> stages{
        vk::PipelineShaderStageCreateInfo(
            {},
            vk::ShaderStageFlagBits::eVertex,
            vertex_shader.module.get_handle(),
            vertex_shader.name.c_str()
        ),
        vk::PipelineShaderStageCreateInfo(
            {},
            vk::ShaderStageFlagBits::eFragment,
            fragment_shader.module.get_handle(),
            fragment_shader.name.c_str()
        ),
    };


    if (std::holds_alternative<ViewportsState_::Dynamic>(viewport.value())) {
        dynamics.push_back(vk::DynamicState::eViewport);
        dynamics.push_back(vk::DynamicState::eScissor);
    }

    if (!raster.line_width) dynamics.push_back(vk::DynamicState::eLineWidth);

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
        multisample,
        depth_stencil_info,
        blend_info,
        dynamic_info.has_value() ? &dynamic_info : nullptr,
        layout,
        render_pass->get_render_pass()->get_handle(),
        render_pass->get_index(),
        0,
        -1
    )

    return boost::leaf::result<std::shared_ptr<GraphicsPipeline>>();
}

}