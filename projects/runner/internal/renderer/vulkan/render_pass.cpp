#include <utility>

#include <mff/algorithms.h>
#include <mff/optional.h>

#include "render_pass.h"

namespace mff::vulkan {

RenderPass::RenderPass(std::shared_ptr<Device> device)
    : device_(std::move(device)) {
}

boost::leaf::result<std::shared_ptr<RenderPass>> RenderPass::build(
    const std::vector<AttachmentInfo>& attachments,
    const std::vector<SubpassInfo>& subpasses,
    const std::vector<SubpassDependencyInfo>& dependencies,
    const std::shared_ptr<Device>& device
) {
    std::vector<vk::AttachmentReference> vk_references;
    std::vector<std::vector<std::uint32_t>> preserve_attachments;
    std::vector<vk::SubpassDescription> vk_descriptions;
    std::vector<vk::SubpassDependency> vk_dependencies;

    std::vector<vk::AttachmentDescription> vk_attachments = mff::map(
        [](const AttachmentInfo& attachment) {
            return vk::AttachmentDescription(
                {},
                attachment.format,
                vk::SampleCountFlagBits::e1, // TODO: samples conversion
                attachment.load,
                attachment.store,
                attachment.stencil_load,
                attachment.stencil_store,
                attachment.initial_layout,
                attachment.final_layout
            );
        },
        attachments
    );

    std::size_t index = 0;

    for (const auto& info: subpasses) {
        auto append_reference = [&](AttachmentInfo_t info) {
            vk_references.emplace_back(std::get<0>(info), std::get<1>(info));

            return &*std::next(std::begin(vk_references), vk_references.size() - 1);
        };

        auto append_references = [&](const AttachmentInfos_t& offsets) {
            auto current_offset = vk_references.size();

            std::for_each(
                std::begin(offsets),
                std::end(offsets),
                [&](const AttachmentInfo_t& info) {
                    append_reference(info);
                }
            );

            return std::make_tuple(
                offsets.size(),
                offsets.size() == 0 ? nullptr : &*std::next(
                    std::begin(vk_references),
                    current_offset
                ));
        };

        auto[input_count, input_ptr] = append_references(info.input_attachments);
        auto[color_count, color_ptr] = append_references(info.color_attachments);
        auto[resolve_count, resolve_ptr] = append_references(info.resolve_attachments);
        auto depth_ptr = optional::map(info.depth_stencil_attachment, [&](auto a) { return append_reference(a); });
        preserve_attachments.push_back(info.preserve_attachments);


        vk_descriptions.push_back(
            vk::SubpassDescription(
                {},
                vk::PipelineBindPoint::eGraphics,
                input_count,
                input_ptr,
                color_count,
                color_ptr,
                resolve_ptr,
                depth_ptr.has_value() ? depth_ptr.value() : nullptr,
                preserve_attachments[index].size(),
                preserve_attachments[index].empty() ? nullptr : preserve_attachments[index].data()));
    }

    for (const auto& dependency: dependencies) {
        vk_dependencies.push_back(
            vk::SubpassDependency(
                dependency.source_subpass,
                dependency.destination_subpass,
                dependency.source_stages,
                dependency.destination_stages,
                dependency.source_access,
                dependency.destination_access,
                dependency.by_region ? vk::DependencyFlagBits::eByRegion : vk::DependencyFlags{}
            ));
    }

    vk::RenderPassCreateInfo info(
        {},
        vk_attachments.size(),
        vk_attachments.data(),
        vk_descriptions.size(),
        vk_descriptions.data(),
        vk_dependencies.size(),
        vk_dependencies.data()
    );

    struct enable_RenderPass : public RenderPass {
        enable_RenderPass(std::shared_ptr<Device> device)
            : RenderPass(std::move(device)) {
        }
    };

    std::shared_ptr<RenderPass> render_pass = std::make_shared<enable_RenderPass>(device);

    LEAF_AUTO_TO(render_pass->handle_, to_result(device->get_handle().createRenderPassUnique(info)));

    return render_pass;
}

RenderPassBuilder::RenderPassBuilder() {
}

std::optional<std::size_t> RenderPassBuilder::get_attachment_offset(const Id_t& id) {
    return mff::optional::map(
        mff::find_if(attachments_, [&](const auto& attachment) { return attachment.id == id; }),
        [&](auto it) { return std::distance(std::cbegin(attachments_), it); }
    );
}

RenderPassBuilder& RenderPassBuilder::add_attachment(
    const Id_t& id,
    vk::AttachmentLoadOp load,
    vk::AttachmentStoreOp store,
    vk::Format format,
    std::uint32_t samples,
    std::optional<vk::ImageLayout> initial_layout,
    std::optional<vk::ImageLayout> final_layout
) {
    // if (get_attachment_offset())
    attachments_.push_back(
        PartialAttachmentInfo{
            id,
            load,
            store,
            format,
            samples,
            initial_layout,
            final_layout
        }
    );

    return *this;
}

RenderPassBuilder& RenderPassBuilder::add_pass(
    const Ids_t& color,
    const std::optional<Id_t>& depth_stencil,
    const Ids_t& input,
    const Ids_t& resolve
) {
    auto to_offset = [&](auto id) { return get_attachment_offset(id).value(); };

    passes_.push_back(
        PartialSubpassInfo{
            mff::map(to_offset, color),
            mff::optional::map(depth_stencil, to_offset),
            mff::map(to_offset, input),
            mff::map(to_offset, resolve),
        }
    );

    return *this;
}

boost::leaf::result<std::shared_ptr<RenderPass>> RenderPassBuilder::build(const std::shared_ptr<Device>& device) {
    std::vector<AttachmentInfo> attachments;
    std::vector<SubpassInfo> passes;
    std::vector<SubpassDependencyInfo> dependencies;

    auto is_depth = [&](Offset_t index) -> bool {
        return mff::contains_if(
            passes_,
            [&](auto pass) {
                return pass.depth_stencil_attachment.has_value() ?
                       pass.depth_stencil_attachment.value() == index : false;
            }
        );
    };

    auto is_color = [&](Offset_t index) -> bool {
        return mff::contains_if(passes_, [&](auto pass) { return mff::contains(pass.color_attachments, index); });
    };

    auto is_resolve = [&](Offset_t index) -> bool {
        return mff::contains_if(passes_, [&](auto pass) { return mff::contains(pass.resolve_attachments, index); });
    };

    auto is_input = [&](Offset_t index) -> bool {
        return mff::contains_if(passes_, [&](auto pass) { return mff::contains(pass.input_attachments, index); });
    };

    Offset_t index = 0;
    for (const auto& attachment: attachments_) {
        auto get_initial_layout = [&]() {
            if (attachment.initial_layout.has_value()) return attachment.initial_layout.value();
            if (is_depth(index)) return vk::ImageLayout::eDepthStencilAttachmentOptimal;
            if (is_color(index)) return vk::ImageLayout::eColorAttachmentOptimal;
            if (is_resolve(index)) return vk::ImageLayout::eTransferDstOptimal;
            if (is_input(index)) return vk::ImageLayout::eShaderReadOnlyOptimal;
            return vk::ImageLayout::eUndefined;
        };

        auto get_final_layout = [&]() {
            if (attachment.initial_layout.has_value()) return attachment.initial_layout.value();
            if (is_input(index)) return vk::ImageLayout::eShaderReadOnlyOptimal;
            if (is_resolve(index)) return vk::ImageLayout::eTransferDstOptimal;
            if (is_color(index)) return vk::ImageLayout::eColorAttachmentOptimal;
            if (is_depth(index)) return vk::ImageLayout::eDepthStencilAttachmentOptimal;
            return vk::ImageLayout::eUndefined;
        };

        attachments.push_back(
            AttachmentInfo{
                attachment.format,
                attachment.samples,
                attachment.load,
                attachment.store,
                attachment.load,
                attachment.store,
                get_initial_layout(),
                get_final_layout()
            }
        );

        index++;
    }

    index = 0;
    for (const auto& pass: passes_) {
        std::set<std::uint32_t> uniq;

        auto add_image_layout = [&](vk::ImageLayout layout) {
            return [&](Offset_t id) {
                uniq.insert(id); // does not really make sense here, but...

                return std::make_tuple(
                    id,
                    layout
                );
            };
        };

        passes.push_back(
            SubpassInfo{
                mff::map(add_image_layout(vk::ImageLayout::eColorAttachmentOptimal), pass.color_attachments),
                mff::optional::map(
                    pass.depth_stencil_attachment,
                    add_image_layout(vk::ImageLayout::eTransferDstOptimal)),
                mff::map(add_image_layout(vk::ImageLayout::eShaderReadOnlyOptimal), pass.input_attachments),
                mff::map(add_image_layout(vk::ImageLayout::eTransferDstOptimal), pass.resolve_attachments),
                std::vector<uint32_t>(uniq.begin(), uniq.end())
            }
        );

        dependencies.push_back(
            SubpassDependencyInfo{
                index,
                index + 1,
                vk::PipelineStageFlagBits::eAllGraphics,
                vk::PipelineStageFlagBits::eAllGraphics,
                {},
                vk::AccessFlagBits::eColorAttachmentWrite,
                true
            }
        );

        index++;
    }

    return RenderPass::build(attachments, passes, dependencies, device);
}

}