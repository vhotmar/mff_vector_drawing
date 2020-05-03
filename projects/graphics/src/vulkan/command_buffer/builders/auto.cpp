#include <mff/graphics/vulkan/command_buffer/builders/auto.h>

#include <mff/graphics/utils.h>
#include <mff/utils.h>

namespace mff::vulkan {

boost::leaf::result<UniqueAutoCommandBufferBuilder> AutoCommandBufferBuilder::build_primary(
    Device* device,
    const QueueFamily* family
) {
    return AutoCommandBufferBuilder::build_with_flags(device, family, Kind_::Primary{});
}

boost::leaf::result<UniqueAutoCommandBufferBuilder> AutoCommandBufferBuilder::build_with_flags(
    Device* device,
    const QueueFamily* family,
    Kind kind,
    vk::CommandBufferUsageFlags usage
) {
    struct enable_AutoCommandBufferBuilder : public AutoCommandBufferBuilder {};
    UniqueAutoCommandBufferBuilder result = std::make_unique<enable_AutoCommandBufferBuilder>();

    LEAF_AUTO(command_pool, device->get_command_pool(family));
    LEAF_AUTO_TO(result->inner_, SyncCommandBufferBuilder::build(command_pool, kind, usage));

    return result;
}

boost::leaf::result<void> AutoCommandBufferBuilder::copy_image(
    const Image* source,
    std::array<std::int32_t, 3> source_offset,
    std::uint32_t source_base_array_layer,
    std::uint32_t source_mip_level,
    const Image* destination,
    std::array<std::int32_t, 3> destination_offset,
    std::uint32_t destination_base_array_layer,
    std::uint32_t destination_mip_level,
    std::array<std::uint32_t, 3> extent,
    std::uint32_t layer_count
) {
    LEAF_CHECK(inner_->copy_image(
        source,
        vk::ImageLayout::eTransferSrcOptimal,
        destination,
        vk::ImageLayout::eTransferDstOptimal,
        {
            UnsafeCommandBufferBuilderImageCopy{
                UnsafeCommandBufferBuilderImageAspect{
                    source->has_color(),
                    source->has_depth(),
                    source->has_stencil()
                },
                source_mip_level,
                destination_mip_level,
                source_base_array_layer,
                destination_base_array_layer,
                layer_count,
                source_offset,
                destination_offset,
                extent
            }
        }
    ));

    return {};
}

}