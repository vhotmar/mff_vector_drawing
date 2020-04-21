#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <vector>

#include "../../leaf.h"
#include "../../vulkan.h"

#include "./device.h"

namespace mff::vulkan {

using Offset_t = std::size_t;
using Offsets_t = std::vector<std::size_t>;
using AttachmentInfo_t = std::tuple<Offset_t, vk::ImageLayout>;
using AttachmentInfos_t = std::vector<AttachmentInfo_t>;
using Id_t = std::string;
using Ids_t = std::vector<Id_t>;

struct AttachmentInfo {
    vk::Format format;
    std::uint32_t samples = 1;
    vk::AttachmentLoadOp load;
    vk::AttachmentStoreOp store;
    vk::AttachmentLoadOp stencil_load;
    vk::AttachmentStoreOp stencil_store;
    vk::ImageLayout initial_layout;
    vk::ImageLayout final_layout;
};

struct SubpassInfo {
    AttachmentInfos_t color_attachments;
    std::optional<AttachmentInfo_t> depth_stencil_attachment;
    AttachmentInfos_t input_attachments;
    AttachmentInfos_t resolve_attachments;
    std::vector<std::uint32_t> preserve_attachments;
};

struct SubpassDependencyInfo {
    std::size_t source_subpass;
    std::size_t destination_subpass;
    vk::PipelineStageFlags source_stages;
    vk::PipelineStageFlags destination_stages;
    vk::AccessFlags source_access;
    vk::AccessFlags destination_access;
    bool by_region;
};

class RenderPass {
private:
    vk::UniqueRenderPass handle_;
    std::shared_ptr<Device> device_;

    RenderPass(std::shared_ptr<Device> device);

public:
    static boost::leaf::result<std::shared_ptr<RenderPass>> build(
        const std::vector<AttachmentInfo>& attachments,
        const std::vector<SubpassInfo>& infos,
        const std::vector<SubpassDependencyInfo>& dependencies,
        const std::shared_ptr<Device>& device
    );
};

class RenderPassBuilder {
public:
    using identificator_t = std::string;
    using identificators_t = std::vector<std::string>;

private:
    struct PartialAttachmentInfo {
        Id_t id;
        vk::AttachmentLoadOp load;
        vk::AttachmentStoreOp store;
        vk::Format format;
        std::uint32_t samples;
        std::optional<vk::ImageLayout> initial_layout = std::nullopt;
        std::optional<vk::ImageLayout> final_layout = std::nullopt;
    };

    struct PartialSubpassInfo {
        Offsets_t color_attachments;
        std::optional<Offset_t> depth_stencil_attachment;
        Offsets_t input_attachments;
        Offsets_t resolve_attachments;
    };

    std::vector<PartialAttachmentInfo> attachments_;
    std::vector<PartialSubpassInfo> passes_;

    std::optional<std::size_t> get_attachment_offset(const Id_t& id);

public:
    RenderPassBuilder();

    // TODO: init_layout, final_layout
    RenderPassBuilder& add_attachment(
        const Id_t& name,
        vk::AttachmentLoadOp load,
        vk::AttachmentStoreOp store,
        vk::Format format,
        std::uint32_t samples = 1,
        std::optional<vk::ImageLayout> initial_layout = std::nullopt,
        std::optional<vk::ImageLayout> final_layout = std::nullopt
    );

    RenderPassBuilder& add_pass(
        const Ids_t& color = {},
        const std::optional<Id_t>& depth_stencil = std::nullopt,
        const Ids_t& input = {},
        const Ids_t& resolve = {}
    );

    boost::leaf::result<std::shared_ptr<RenderPass>> build(const std::shared_ptr<Device>& device);
};

}