#include <mff/graphics/vulkan/pipeline/blend.h>

#include <mff/algorithms.h>
#include <mff/graphics/vulkan/render_pass.h>
#include <mff/utils.h>

namespace mff::vulkan {

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
