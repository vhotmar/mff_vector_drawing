#include <mff/graphics/vulkan/command_buffer/builders/unsafe.h>

#include <range/v3/all.hpp>
#include <mff/graphics/utils.h>
#include <mff/utils.h>

namespace mff::vulkan {

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

boost::leaf::result<void> UnsafeCommandBufferBuilder::copy_image(
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

    if (updated.empty()) return {};

    command_buffer_.copyImage(
        source_image.get_image()->get_handle(),
        source_layout,
        destination_image.get_image()->get_handle(),
        destination_layout,
        updated.size(),
        updated.data()
    );

    return {};
}

vk::ImageAspectFlags UnsafeCommandBufferBuilderImageAspect::to_vulkan() const {
    auto res = vk::ImageAspectFlags{};

    if (color) res |= vk::ImageAspectFlagBits::eColor;
    if (depth) res |= vk::ImageAspectFlagBits::eDepth;
    if (stencil) res |= vk::ImageAspectFlagBits::eStencil;

    return res;
}
}