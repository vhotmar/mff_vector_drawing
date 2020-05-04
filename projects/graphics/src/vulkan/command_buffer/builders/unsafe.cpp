#include <mff/graphics/vulkan/command_buffer/builders/unsafe.h>

#include <range/v3/all.hpp>
#include <mff/graphics/utils.h>
#include <mff/utils.h>

namespace mff::vulkan {

boost::leaf::result<UniqueUnsafeCommandBufferBuilder> UnsafeCommandBufferBuilder::build_primary(
    Device* device,
    const QueueFamily* family
) {
    LEAF_AUTO(command_pool, device->get_command_pool(family));

    return UnsafeCommandBufferBuilder::build(command_pool, Kind_::Primary{}, {});
}

boost::leaf::result<UniqueUnsafeCommandBufferBuilder> UnsafeCommandBufferBuilder::build(
    CommandPool* pool,
    Kind kind,
    vk::CommandBufferUsageFlags usage
) {
    bool secondary = std::visit(
        overloaded{
            [&](Kind_::Primary) -> bool { return false; },
            [&](Kind_::Secondary) -> bool { return true; }
        },
        kind
    );

    assert(!secondary); // TODO: enable secondary

    auto get_allocation = ([&]() -> boost::leaf::result<UniqueCommandPoolAllocation> {
        LEAF_AUTO(allocations, pool->allocate(1, secondary));
        auto allocation = std::move(allocations.back());

        return allocation;
    });
    LEAF_AUTO(allocation, get_allocation());

    auto cmd_buffer = allocation->get_handle();

    LEAF_CHECK(to_result(cmd_buffer.begin(vk::CommandBufferBeginInfo(usage, nullptr))));

    struct enable_UnsafeCommandBufferBuilder : public UnsafeCommandBufferBuilder {};
    UniqueUnsafeCommandBufferBuilder result = std::make_unique<enable_UnsafeCommandBufferBuilder>();

    result->command_buffer_ = cmd_buffer;
    result->allocation_ = std::move(allocation);

    return result;
}

const Device* UnsafeCommandBufferBuilder::get_device() const {
    return allocation_->get_pool()->get_device();
}

void UnsafeCommandBufferBuilder::pipeline_barrier(UnsafeCommandBufferBuilderPipelineBarrier command) {
    command_buffer_.pipelineBarrier(
        command.src_stage_mask,
        command.dst_stage_mask,
        command.dependency_flags,
        command.memory_barriers.size(),
        command.memory_barriers.data(),
        command.buffer_barriers.size(),
        command.buffer_barriers.data(),
        command.image_barriers.size(),
        command.image_barriers.data());
}

void UnsafeCommandBufferBuilder::copy_image(
    const Image* source,
    vk::ImageLayout source_layout,
    const Image* destination,
    vk::ImageLayout destination_layout,
    const std::vector<UnsafeCommandBufferBuilderImageCopy>& regions
) {
    auto source_image = source->get_inner_image();
    auto destination_image = destination->get_inner_image();

    auto updated = regions
        | ranges::views::filter([](auto copy) { return copy.layer_count != 0; })
        | ranges::views::transform(
            [&](const auto& copy) {
                return vk::ImageCopy(
                    vk::ImageSubresourceLayers(
                        copy.aspect.to_vulkan(),
                        copy.source_mip_level,
                        copy.source_base_array_layer + source_image.get_first_layer(),
                        copy.layer_count
                    ),
                    to_offset(copy.source_offset),
                    vk::ImageSubresourceLayers(
                        copy.aspect.to_vulkan(),
                        copy.source_mip_level,
                        copy.source_base_array_layer + source_image.get_first_layer(),
                        copy.layer_count
                    ),
                    to_offset(copy.destination_offset),
                    to_extent(copy.extent));
            }
        )
        | ranges::to<std::vector>();

    if (updated.empty()) return;

    command_buffer_.copyImage(
        source_image.get_image()->get_handle(),
        source_layout,
        destination_image.get_image()->get_handle(),
        destination_layout,
        updated.size(),
        updated.data()
    );

    return;
}

boost::leaf::result<UniqueUnsafeCommandBuffer> UnsafeCommandBufferBuilder::build() {
    LEAF_CHECK(to_result(command_buffer_.end()));

    struct enable_UnsafeCommandBuffer: public UnsafeCommandBuffer {};
    UniqueUnsafeCommandBuffer result = std::make_unique<enable_UnsafeCommandBuffer>();

    result->command_buffer_ = command_buffer_;
    result->allocation_ = std::move(allocation_);

    return result;
}

vk::ImageAspectFlags UnsafeCommandBufferBuilderImageAspect::to_vulkan() const {
    vk::ImageAspectFlags res = {};

    if (color) res |= vk::ImageAspectFlagBits::eColor;
    if (depth) res |= vk::ImageAspectFlagBits::eDepth;
    if (stencil) res |= vk::ImageAspectFlagBits::eStencil;

    return res;
}

bool UnsafeCommandBufferBuilderPipelineBarrier::is_empty() const {
    return src_stage_mask == vk::PipelineStageFlags{} || dst_stage_mask == vk::PipelineStageFlags{};
}

UnsafeCommandBufferBuilderPipelineBarrier UnsafeCommandBufferBuilderPipelineBarrier::merge(
    const UnsafeCommandBufferBuilderPipelineBarrier& other
) const {
    return UnsafeCommandBufferBuilderPipelineBarrier{
        src_stage_mask | other.src_stage_mask,
        dst_stage_mask | other.dst_stage_mask,
        dependency_flags & other.dependency_flags,
        ranges::views::concat(memory_barriers, other.memory_barriers) | ranges::to<std::vector>(),
        ranges::views::concat(buffer_barriers, other.buffer_barriers) | ranges::to<std::vector>(),
        ranges::views::concat(image_barriers, other.image_barriers) | ranges::to<std::vector>(),
    };
}

UnsafeCommandBufferBuilderPipelineBarrier& UnsafeCommandBufferBuilderPipelineBarrier::add_execution_dependency(
    vk::PipelineStageFlags source,
    vk::PipelineStageFlags destination,
    bool by_region
) {
    if (!by_region) {
        dependency_flags = vk::DependencyFlags{};
    }

    src_stage_mask |= source;
    dst_stage_mask |= destination;

    return *this;
}

UnsafeCommandBufferBuilderPipelineBarrier& UnsafeCommandBufferBuilderPipelineBarrier::add_image_memory_barrier(
    const Image* image,
    std::uint32_t mipmaps_from,
    std::uint32_t mipmaps_to,
    std::uint32_t layers_from,
    std::uint32_t layers_to,
    vk::PipelineStageFlags source_stage,
    vk::AccessFlags source_access,
    vk::PipelineStageFlags destination_stage,
    vk::AccessFlags destination_access,
    bool by_region,
    std::optional<std::tuple<std::uint32_t, std::uint32_t>> queue_transfer,
    vk::ImageLayout current_layout,
    vk::ImageLayout new_layout
) {
    add_execution_dependency(source_stage, destination_stage, by_region);

    auto aspect_mask = ([&]() {
        vk::ImageAspectFlags result = {};

        if (image->has_color()) result |= vk::ImageAspectFlagBits::eColor;
        if (image->has_depth()) result |= vk::ImageAspectFlagBits::eDepth;
        if (image->has_stencil()) result |= vk::ImageAspectFlagBits::eStencil;

        return result;
    })();

    auto[src_queue, dst_queue] = ([&]() {
        if (queue_transfer) {
            return queue_transfer.value();
        }

        return std::make_tuple(std::uint32_t{}, std::uint32_t{});
    })();

    image_barriers.push_back(
        vk::ImageMemoryBarrier(
            source_access,
            destination_access,
            current_layout,
            new_layout,
            src_queue,
            dst_queue,
            image->get_inner_image().get_image()->get_handle(),
            vk::ImageSubresourceRange(
                aspect_mask,
                mipmaps_from,
                mipmaps_to - mipmaps_from,
                layers_from,
                layers_to - layers_from
            )));

    return *this;
}

}