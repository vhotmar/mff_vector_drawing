#include <mff/graphics/vulkan/render_pass.h>

#include <utility>

#include <mff/algorithms.h>
#include <mff/optional.h>

#include "../utils.h"

namespace mff::vulkan {

boost::leaf::result<std::unique_ptr<RenderPass>> RenderPass::build(
    const Device* device,
    const std::vector<AttachmentDescription>& attachments,
    const std::vector<SubpassDescription>& subpasses,
    const std::vector<SubpassDependency>& dependencies
) {
    // to build the RenderPass we need to create References between the individual Subpasses and
    // Attachments
    std::vector<vk::AttachmentReference> vk_references;
    std::vector<std::vector<std::uint32_t>> preserve_attachments;
    std::vector<vk::SubpassDescription> vk_descriptions;
    std::vector<vk::SubpassDependency> vk_dependencies;

    // convert attachments to their vulkan representation
    std::vector<vk::AttachmentDescription> vk_attachments = mff::map(
        [](const AttachmentDescription& attachment) {
            return attachment.to_vulkan();
        },
        attachments
    );

    std::size_t index = 0;

    for (const auto& info: subpasses) {
        // for each Subpass we need to build the references (for each input, color, depth etc.
        // attachment), so we will create helper function which will add the reference to global
        // references store and return pointer to added element
        auto append_reference = [&](AttachmentReference info) {
            vk_references.emplace_back(std::get<0>(info), std::get<1>(info));

            return &*std::next(std::begin(vk_references), vk_references.size() - 1);
        };

        // same for multiple descriptions
        auto append_references = [&](const std::vector<AttachmentReference>& offsets) {
            auto current_offset = vk_references.size();

            std::for_each(
                std::begin(offsets),
                std::end(offsets),
                [&](const AttachmentReference& info) {
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
        vk_dependencies.push_back(dependency.to_vulkan());
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

    struct enable_RenderPass : public RenderPass {};
    std::unique_ptr<RenderPass> render_pass = std::make_unique<enable_RenderPass>();

    render_pass->device_ = device;
    render_pass->attachments_ = attachments;
    render_pass->subpasses_.reserve(subpasses.size());

    index = 0;
    for (const auto& subpass_description: subpasses) {
        struct enable_Subpass : public Subpass {};
        std::unique_ptr<Subpass> subpass = std::make_unique<enable_Subpass>();

        subpass->render_pass_ = render_pass.get();
        subpass->description_ = subpass_description;
        subpass->subpass_id_ = index++;

        render_pass->subpasses_.push_back(std::move(subpass));
    }

    LEAF_AUTO_TO(render_pass->handle_, to_result(device->get_handle().createRenderPassUnique(info)));

    return render_pass;
}

vk::RenderPass RenderPass::get_handle() const {
    return handle_.get();
}

std::optional<const Subpass*> RenderPass::get_subpass(std::uint32_t index) const {
    return subpasses_[index].get();
}

std::optional<std::size_t> RenderPassBuilder::get_attachment_offset(const Id& id) {
    return mff::optional::map(
        mff::find_if(attachments_, [&](const auto& attachment) { return attachment.id == id; }),
        [&](auto it) { return std::distance(std::cbegin(attachments_), it); }
    );
}

RenderPassBuilder& RenderPassBuilder::add_attachment(
    const Id& id,
    vk::AttachmentLoadOp load,
    vk::AttachmentStoreOp store,
    vk::Format format,
    vk::SampleCountFlagBits samples,
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
    const std::vector<Id>& color,
    const std::optional<Id>& depth_stencil,
    const std::vector<Id>& input,
    const std::vector<Id>& resolve
) {
    passes_.push_back(
        PartialSubpassInfo{
            color,
            depth_stencil,
            input,
            resolve
        }
    );

    return *this;
}

boost::leaf::result<std::unique_ptr<RenderPass>> RenderPassBuilder::build(const Device* device) {
    std::vector<AttachmentDescription> attachments;
    std::vector<SubpassDescription> passes;
    std::vector<SubpassDependency> dependencies;

    // helpers to decide if is attachment ever used like depth/color/resolve/input
    auto is_depth = [&](Id id) -> bool {
        return mff::contains_if(
            passes_,
            [&](auto pass) {
                return pass.depth_stencil_attachment.has_value() ?
                       pass.depth_stencil_attachment.value() == id : false;
            }
        );
    };

    auto is_color = [&](Id id) -> bool {
        return mff::contains_if(passes_, [&](auto pass) { return mff::contains(pass.color_attachments, id); });
    };

    auto is_resolve = [&](Id id) -> bool {
        return mff::contains_if(passes_, [&](auto pass) { return mff::contains(pass.resolve_attachments, id); });
    };

    auto is_input = [&](Id id) -> bool {
        return mff::contains_if(passes_, [&](auto pass) { return mff::contains(pass.input_attachments, id); });
    };

    for (const auto& attachment: attachments_) {
        std::optional<vk::ImageLayout> initial_layout = std::nullopt;
        std::optional<vk::ImageLayout> final_layout = std::nullopt;

        if (is_depth(attachment.id)) {
            if (!initial_layout) initial_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            final_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        }

        if (is_color(attachment.id)) {
            if (!initial_layout) initial_layout = vk::ImageLayout::eColorAttachmentOptimal;
            final_layout = vk::ImageLayout::eColorAttachmentOptimal;
        }

        if (is_resolve(attachment.id)) {
            if (!initial_layout) initial_layout = vk::ImageLayout::eTransferDstOptimal;
            final_layout = vk::ImageLayout::eTransferDstOptimal;
        }

        if (is_input(attachment.id)) {
            if (!initial_layout) initial_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
            final_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
        }

        if (attachment.initial_layout) {
            initial_layout = attachment.initial_layout;
        }

        if (attachment.final_layout) {
            final_layout = attachment.final_layout;
        }

        if (!initial_layout || !final_layout) {
            return boost::leaf::new_error(build_render_pass_error_code::missing_initial_or_final_layout);
        }

        attachments.push_back(
            AttachmentDescription{
                attachment.format,
                attachment.samples,
                attachment.load,
                attachment.store,
                attachment.load,
                attachment.store,
                initial_layout.value(),
                final_layout.value()
            }
        );
    }

    std::uint32_t index = 0;
    for (const auto& pass: passes_) {
        std::set<std::uint32_t> uniq;

        auto get_reference = [&](const Id& id, vk::ImageLayout layout) -> std::optional<AttachmentReference> {
            auto offset = get_attachment_offset(id);
            if (!offset) return std::nullopt;

            uniq.insert(offset.value());

            return std::make_tuple(offset.value(), layout);
        };

        auto get_references = [&](
            const std::vector<Id>& ids,
            vk::ImageLayout layout
        ) -> std::optional<std::vector<AttachmentReference>> {
            std::vector<AttachmentReference> references;
            references.reserve(ids.size());

            for (const auto& id: ids) {
                auto reference = get_reference(id, layout);
                if (!reference) return std::nullopt;

                references.push_back(std::move(reference.value()));
            }

            return references;
        };

        auto colors = get_references(pass.color_attachments, vk::ImageLayout::eColorAttachmentOptimal);
        auto inputs = get_references(pass.input_attachments, vk::ImageLayout::eColorAttachmentOptimal);
        auto resolves = get_references(pass.resolve_attachments, vk::ImageLayout::eColorAttachmentOptimal);

        if (!colors || !inputs || !resolves) {
            return boost::leaf::new_error(build_render_pass_error_code::invalid_id_in_subpass_definition);
        }

        std::optional<AttachmentReference> depth = std::nullopt;

        if (pass.depth_stencil_attachment) {
            depth = get_reference(
                pass.depth_stencil_attachment.value(),
                vk::ImageLayout::eDepthStencilAttachmentOptimal
            );
        }

        passes.push_back(
            SubpassDescription{
                inputs.value(),
                colors.value(),
                resolves.value(),
                depth,
                std::vector<uint32_t>(uniq.begin(), uniq.end())
            }
        );

        dependencies.push_back(
            SubpassDependency{
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

    return RenderPass::build(device, attachments, passes, dependencies);
}

}