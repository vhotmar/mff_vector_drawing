#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <mff/leaf.h>
#include <mff/graphics/vulkan/device.h>
#include <mff/graphics/vulkan/framebuffer/desc.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

class RenderPass;

/**
 * Class identifying concrete subpass within RenderPass.
 *
 * This does not have vulkan equivalent (it is simple construct to identify concrete subpass).
 */
class Subpass {
private:
    const RenderPass* render_pass_ = nullptr;
    SubpassDescription description_ = {};
    std::uint32_t subpass_id_ = -1;

    friend class RenderPass;

public:
    /**
     *
     * @return
     */
    std::uint32_t get_color_attachments_count() const;

    vk::SampleCountFlagBits get_samples() const;

    const RenderPass* get_render_pass() const;

    std::uint32_t get_index() const;
};

/**
 * Defines the layout of subpasses
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap7.html#renderpass
 */
class RenderPass {
private:
    vk::UniqueRenderPass handle_;
    const Device* device_;
    std::vector<AttachmentDescription> attachments_;
    std::vector<std::unique_ptr<Subpass>> subpasses_;

public:
    /**
     * Builds new render pass
     * @param attachments
     * @param infos
     * @param dependencies
     * @param device
     * @return RenderPass build from the parameters
     */
    static boost::leaf::result<std::unique_ptr<RenderPass>> build(
        const Device* device,
        const std::vector<AttachmentDescription>& attachments,
        const std::vector<SubpassDescription>& subpasses,
        const std::vector<SubpassDependency>& dependencies
    );

    vk::RenderPass get_handle() const;

    std::optional<const Subpass*> get_subpass(std::uint32_t index) const;
};

/**
 * Utility class which builds concrete RenderPass. Subpasses created in this builder are treated
 * sequentially. For any other configuration you need to build the RenderPass by hand.
 */
class RenderPassBuilder {
private:
    using Id = std::string;
    struct PartialAttachmentInfo {
        Id id;
        vk::AttachmentLoadOp load;
        vk::AttachmentStoreOp store;
        vk::Format format;
        vk::SampleCountFlagBits samples;
        std::optional<vk::ImageLayout> initial_layout = std::nullopt;
        std::optional<vk::ImageLayout> final_layout = std::nullopt;
    };

    struct PartialSubpassInfo {
        std::vector<Id> color_attachments;
        std::optional<Id> depth_stencil_attachment;
        std::vector<Id> input_attachments;
        std::vector<Id> resolve_attachments;
    };

    std::vector<PartialAttachmentInfo> attachments_;
    std::vector<PartialSubpassInfo> passes_;

    std::optional<std::size_t> get_attachment_offset(const Id& id);

public:
    /**
     * Add RenderPass attachment.
     *
     * @param name the internal id of the attachment which you can us in add_pass.
     * @param load How is the attachment and stencil treated at the beginning of subpass.
     * @param store How is the attachment and stencil treated at the end of subpass.
     * @param format Format of the attachment.
     * @param samples Number of samples.
     * @param initial_layout The layout of the attachment at the beginning.
     * @param final_layout The layout of the attachment at the end.
     * @return RenderPassBuilder for chaining
     */
    RenderPassBuilder& add_attachment(
        const Id& name,
        vk::AttachmentLoadOp load,
        vk::AttachmentStoreOp store,
        vk::Format format,
        vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1,
        std::optional<vk::ImageLayout> initial_layout = std::nullopt,
        std::optional<vk::ImageLayout> final_layout = std::nullopt
    );

    /**
     * Add RenderPass Subpass.
     *
     * @param color The color attachments referenced by ID.
     * @param depth_stencil The depth attachment referenced by ID.
     * @param input The input attachment referenced by ID.
     * @param resolve The resolve attachment referenced by ID.
     * @return RenderPassBuilder for chaining
     */
    RenderPassBuilder& add_pass(
        const std::vector<Id>& color = {},
        const std::optional<Id>& depth_stencil = std::nullopt,
        const std::vector<Id>& input = {},
        const std::vector<Id>& resolve = {}
    );

    /**
     * Builds the RenderPass on specified device
     *
     * @param device The device where RenderPass should be built
     * @return result with finalized RenderPass
     */
    boost::leaf::result<std::unique_ptr<RenderPass>> build(const Device* device);
};


/**
 * Errors that can happen during render pass building (via RenderPassBuilder)
 */
enum class build_render_pass_error_code {
    /**
     * Initial or final layout could not be inferred and was not set manually
     */
        missing_initial_or_final_layout,

    /**
     * Invalid id was used when adding subpass definition
     */
        invalid_id_in_subpass_definition
};

}

namespace boost::leaf {

template <>
struct is_e_type<mff::vulkan::build_render_pass_error_code> : public std::true_type {};

}
