#pragma once

#include <optional>
#include <variant>

#include <mff/graphics/vulkan/vulkan.h>
#include <mff/graphics/math.h>

namespace mff::vulkan {

/**
 * Describe how the blending system should behave for individual attachments
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap28.html#VkPipelineColorBlendAttachmentState
 */
struct AttachmentBlend {
    /**
     * If false, blending is ignored and the output is directly written to the attachment.
     */
    bool enabled = false;

    vk::BlendOp color_op = vk::BlendOp::eAdd;
    vk::BlendFactor color_source = vk::BlendFactor::eZero;
    vk::BlendFactor color_destination = vk::BlendFactor::eOne;

    vk::BlendOp alpha_op = vk::BlendOp::eAdd;
    vk::BlendFactor alpha_source = vk::BlendFactor::eZero;
    vk::BlendFactor alpha_destination = vk::BlendFactor::eOne;

    bool mask_red = true;
    bool mask_green = true;
    bool mask_blue = true;
    bool mask_alpha = true;

    static AttachmentBlend pass_through();

    static AttachmentBlend alpha_blending();

    vk::PipelineColorBlendAttachmentState to_vulkan() const;
};

namespace AttachmentsBlend_ {

/**
 * All the framebuffer attachments will use the same blending.
 */
struct Collective {
    AttachmentBlend blend;
};

/**
 * Each attachment will behave differently.
 */
struct Individual {
    std::vector<AttachmentBlend> blends;
};

}

/**
 * Describes how the blending system should behave.
 */
using AttachmentsBlend = std::variant<AttachmentsBlend_::Collective, AttachmentsBlend_::Individual>;

class Subpass;

/**
 * Describes how the color output of the fragment shader is written to the attachment.
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap28.html#VkPipelineColorBlendStateCreateInfo
 */
struct Blend {
    std::optional<vk::LogicOp> logic_op;
    AttachmentsBlend attachments;

    /**
     * The constant color to use for the `Constant*` blending operation.
     *
     * If std::nullopt then considered dynamic and needs to be set during draw call.
     */
    std::optional<Vector4f> blend_constants;

    vk::PipelineColorBlendStateCreateInfo to_vulkan(std::shared_ptr<Subpass> pass) const;
};

}