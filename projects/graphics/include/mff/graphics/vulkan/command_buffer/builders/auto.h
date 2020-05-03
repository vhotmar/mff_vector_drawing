#pragma once

#include <array>

#include <mff/leaf.h>
#include <mff/graphics/vulkan/command_buffer/builders/base.h>
#include <mff/graphics/vulkan/command_buffer/builders/sync.h>
#include <mff/graphics/vulkan/command_buffer/command_buffer.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

class AutoCommandBufferBuilder;
using UniqueAutoCommandBufferBuilder = std::unique_ptr<AutoCommandBufferBuilder>;

class AutoCommandBufferBuilder : public CommandBufferBuilder<AutoCommandBufferBuilder> {
public:
    boost::leaf::result<void> copy_image(
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
    );

    static boost::leaf::result<UniqueAutoCommandBufferBuilder> build_primary(
        Device* device,
        const QueueFamily* family
    );

    static boost::leaf::result<UniqueAutoCommandBufferBuilder> build_with_flags(
        Device* device,
        const QueueFamily* family,
        Kind kind,
        vk::CommandBufferUsageFlags usage = {}
    );

private:
    AutoCommandBufferBuilder() = default;

    UniqueSyncCommandBufferBuilder inner_ = nullptr;
};

}
