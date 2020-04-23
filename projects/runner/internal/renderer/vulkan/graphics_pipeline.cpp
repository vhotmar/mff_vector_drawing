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

bool Stencil::always_keep() {
    if (compare == vk::CompareOp::eAlways)
        return pass_op == vk::StencilOp::eKeep && depth_fail_op == vk::StencilOp::eKeep;
    if (compare == vk::CompareOp::eNever) return fail_op == vk::StencilOp::eKeep;

    return pass_op == vk::StencilOp::eKeep && fail_op == vk::StencilOp::eKeep
        && depth_fail_op == vk::StencilOp::eKeep;
}

vk::StencilOpState Stencil::to_vulkan() {
    return vk::StencilOpState(
        fail_op,
        pass_op,
        depth_fail_op,
        compare,
        compare_mask.value_or(std::numeric_limits<std::uint32_t>::max()), // TODO: ALL
        write_mask.value_or(std::numeric_limits<std::uint32_t>::max()), // TODO: ALL
        reference.value_or(0)
    );
}

vk::PipelineDepthStencilStateCreateInfo DepthStencil::to_vulkan() const {
    using db_info = std::tuple<bool, float, float>;
    auto[db_enabled, db_start, db_end] = std::visit(
        overloaded{
            [](DepthBounds_::Disabled d) -> db_info {
                return std::make_tuple(false, 0.0f, 0.0f);
            },
            [](DepthBounds_::Dynamic d) -> db_info {
                return std::make_tuple(true, 0.0f, 1.0f);
            },
            [](DepthBounds_::Fixed f) -> db_info {
                return std::make_tuple(true, f.from, f.to);
            }
        },
        depth_bounds_test
    );

    return vk::PipelineDepthStencilStateCreateInfo(
        {},
        !depth_write && depth_compare == vk::CompareOp::eAlways,
        depth_write,
        depth_compare,
        db_enabled,
        stencil_front.always_keep() && stencil_back.always_keep(),
        stencil_front.to_vulkan(),
        stencil_back.to_vulkan(),
        db_start,
        db_end
    );
}

vk::PipelineViewportStateCreateInfo to_vulkan(const ViewportsState& state) {
    using vp_info = std::tuple<std::vector<vk::Viewport>, std::vector<vk::Rect2D>, std::uint32_t>;
    auto[vp_vp, vp_sc, vp_num] = std::visit(
        overloaded{
            [](ViewportsState_::Fixed f) -> vp_info {
                return std::make_tuple(
                    mff::map(
                        [](auto it) { return std::get<0>(it).to_vulkan(); },
                        f.data
                    ),
                    mff::map(
                        [](auto it) { return std::get<1>(it).to_vulkan(); },
                        f.data
                    ),
                    f.data.size()
                );
            },
            [](ViewportsState_::Dynamic d) -> vp_info {
                return vp_info({}, {}, d.num);
            },
        },
        state
    );

    return vk::PipelineViewportStateCreateInfo(
        {},
        vp_num,
        vp_num == 0 ? nullptr : vp_vp.data(),
        vp_num,
        vp_num == 0 ? nullptr : vp_sc.data()
    );
}

vk::Viewport Viewport::to_vulkan() const {
    return vk::Viewport(
        origin[0],
        origin[1],
        dimensions[0],
        dimensions[1],
        depth_range_from,
        depth_range_to
    );
}

vk::Rect2D Scissor::to_vulkan() const {
    return vk::Rect2D(
        to_offset(origin),
        to_extent(dimensions)
    );
}

vk::PipelineRasterizationStateCreateInfo Rasterization::to_vulkan() const {
    using dp_info = std::tuple<bool, float, float, float>;
    auto[db_enable, db_const, db_clamp, db_slope] = std::visit(
        overloaded{
            [](DepthBiasControl_::Disabled d) -> dp_info {
                return std::make_tuple(true, 0.0f, 0.0f, 0.0f);
            },
            [](DepthBiasControl_::Static s) -> dp_info {
                return std::make_tuple(true, s.bias.constant_factor, s.bias.clamp, s.bias.slope_factor);
            },
        },
        depth_bias
    );

    return vk::PipelineRasterizationStateCreateInfo(
        {},
        depth_clamp,
        rasterizer_discard,
        polygon_mode,
        cull_mode,
        front_face,
        db_enable,
        db_const,
        db_clamp,
        db_slope,
        line_width.value_or(1.0)
    );
}

vk::PipelineColorBlendAttachmentState AttachmentBlend::to_vulkan() const {
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

AttachmentBlend AttachmentBlend::pass_through() {
    return {
        false,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eZero,
        vk::BlendFactor::eOne,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eZero,
        vk::BlendFactor::eOne,
        true,
        true,
        true,
        true,
    };
}


AttachmentBlend AttachmentBlend::alpha_blending() {
    return {
        true,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eSrcAlpha,
        vk::BlendFactor::eOneMinusSrcAlpha,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eSrcAlpha,
        vk::BlendFactor::eOneMinusSrcAlpha,
        true,
        true,
        true,
        true,
    };
}

vk::PipelineVertexInputStateCreateInfo vertex::DefinitionInfo::to_vulkan() const {
    std::vector<vk::VertexInputBindingDescription> binding_descriptions = mff::map(
        [](const auto& i) {
            return vk::VertexInputBindingDescription(i.binding, i.stride, i.input_rate);
        },
        buffers
    );

    std::vector<vk::VertexInputAttributeDescription> attribute_descriptions = mff::map(
        [](const auto& i) {
            return vk::VertexInputAttributeDescription(i.location, i.binding, i.info.format, i.info.offset);
        },
        attributes
    );

    return vk::PipelineVertexInputStateCreateInfo(
        {},
        binding_descriptions.size(),
        binding_descriptions.data(),
        attribute_descriptions.size(),
        attribute_descriptions.data());
}

vk::PipelineColorBlendStateCreateInfo Blend::to_vulkan(std::shared_ptr<Subpass> pass) const {
    using ba_info = std::vector<vk::PipelineColorBlendAttachmentState>;
    auto blend_attachment_infos = std::visit(
        overloaded{
            [](AttachmentsBlend_::Individual i) -> ba_info {
                return mff::map([](auto b) { return b.to_vulkan(); }, i.blends);
            },
            [&](AttachmentsBlend_::Collective c) -> ba_info {
                return ba_info(pass->get_color_attachments_count(), c.blend.to_vulkan());
            }
        },
        attachments
    );

    return vk::PipelineColorBlendStateCreateInfo(
        {},
        logic_op.has_value(),
        logic_op.value_or(vk::LogicOp::eClear),
        blend_attachment_infos.size(),
        blend_attachment_infos.data(),
        blend_constants.has_value()
        ? to_array(blend_constants.value())
        : std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}
    );
}
}