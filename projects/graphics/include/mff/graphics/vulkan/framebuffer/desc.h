#pragma once

#include <optional>
#include <tuple>
#include <vector>

#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

/**
 * Alias for AttachmentReferenceIndice
 */
using AttachmentReferenceIndice = std::uint32_t;

/**
 * Alias for description of attachement reference
 */
using AttachmentReference = std::tuple<AttachmentReferenceIndice, vk::ImageLayout>;

/**
 * This describes the attachments which will be used in all Subpasses.
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap7.html#VkAttachmentDescription
 */
struct AttachmentDescription {
    /**
     * Format of image used as attachment
     */
    vk::Format format = vk::Format::eUndefined;

    /**
     * Number of samples of the image
     */
    vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;

    /**
     * Is specifying how the contents of color and depth components of the attachment are treated
     * at the beginning of the subpass where it is first used
     */
    vk::AttachmentLoadOp load = vk::AttachmentLoadOp::eDontCare;

    /**
     * This value is specifying how the contents of color and depth components of the attachment
     * are treated at the end of the subpass where it is last used
     */
    vk::AttachmentStoreOp store = vk::AttachmentStoreOp::eDontCare;

    /**
     * This value is specifying how the contents of stencil components of the attachment are
     * treated at the beginning of the subpass where it is first used.
     */
    vk::AttachmentLoadOp stencil_load = vk::AttachmentLoadOp::eDontCare;

    /**
     * This value is specifying how the contents of stencil components of the attachment are
     * treated at the end of the last subpass where it is used.
     */
    vk::AttachmentStoreOp stencil_store = vk::AttachmentStoreOp::eDontCare;

    /**
     * The layout the attachment image subresource will be in when a render pass instance begins.
     */
    vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined;

    /**
     * The layout the attachment image subresource will be transitioned to when a render pass
     * instance ends.
     */
    vk::ImageLayout final_layout = vk::ImageLayout::eUndefined;

    vk::AttachmentDescription to_vulkan() const;
};


/**
 * Describe one of the subpass in RenderPass
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap7.html#VkSubpassDescription
 */
struct SubpassDescription {
    /**
     * Each element of the input_attachments array corresponds to an input attachment index in a
     * fragment shader, i.e. if a shader declares an image variable decorated with a
     * InputAttachmentIndex value of X, then it uses the attachment provided in
     * input_attachments[X].
     */
    std::vector<AttachmentReference> input_attachments;

    /**
     * Each element of the color_attachments array corresponds to an output location in the shader,
     * i.e. if the shader declares an output variable decorated with a Location value of X, then it
     * uses the attachment provided in color_attachments[X].
     */
    std::vector<AttachmentReference> color_attachments;

    /**
     * If resolve_attachments is not NULL, each of its elements corresponds to a color attachment
     * (the element in pColorAttachments at the same index), and a multisample resolve operation is
     * defined for each attachment. At the end of each subpass, multisample resolve operations read
     * the subpass’s color attachments, and resolve the samples for each pixel within the render
     * area to the same pixel location in the corresponding resolve attachments
     */
    std::vector<AttachmentReference> resolve_attachments;

    /**
     * Corresponds to the depth/stencil attachment. At the end of each subpass, multisample resolve
     * operations read the subpass’s depth/stencil attachment, and resolve the samples for each
     * pixel to the same pixel location in the corresponding resolve attachment.
     */
    std::optional<AttachmentReference> depth_stencil_attachment;

    /**
     *  Array of render pass attachment indices identifying attachments that are not used by this
     *  subpass, but whose contents must be preserved throughout the subpass.
     */
    std::vector<AttachmentReferenceIndice> preserve_attachments;
};

/**
 * Describe the dependency between two subpasses in RenderPass.
 *
 * This is important when ther is dependency between subpasses (obviously) and because the
 * implementation may change the order of subpasses (it can be different then what you specify).
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap7.html#VkSubpassDependency
 */
struct SubpassDependency {
    /**
     * Index of the subpass that writes the data that destination_subpass is going to use.
     */
    std::uint32_t source_subpass;

    /**
     * Index of the subpass that reads the data that source_subpass wrote.
     */
    std::uint32_t destination_subpass;

    /**
     * The pipeline stages that must be finished on the previous subpass before the destination
     * subpass can start
     */
    vk::PipelineStageFlags source_stages;

    /**
     * The pipeline stages of the destination subpass that must wait for the source to be finished.
     * Stages that are earlier of the stages specified here can start before the source is finished.
     */
    vk::PipelineStageFlags destination_stages;

    /**
     * The way the source subpass accesses the attachments on which we depend.
     */
    vk::AccessFlags source_access;

    /**
     * The way the destination subpass accesses the attachments on which we depend.
     */
    vk::AccessFlags destination_access;

    /**
     * Must this pass be wholly evaluated, before using?
     */
    bool by_region;

    vk::SubpassDependency to_vulkan() const;
};

}