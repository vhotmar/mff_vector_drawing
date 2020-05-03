#pragma once

#include <array>
#include <memory>

#include <mff/leaf.h>
#include <mff/graphics/vulkan/command_buffer/builders/base.h>
#include <mff/graphics/vulkan/command_buffer/command_buffer.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

class UnsafeCommandBufferBuilder;
using UniqueUnsafeCommandBufferBuilder = std::unique_ptr<UnsafeCommandBufferBuilder>;

struct UnsafeCommandBufferBuilderPipelineBarrier {
    vk::PipelineStageFlags src_stage_mask = {};
    vk::PipelineStageFlags dst_stage_mask = {};
    vk::DependencyFlags dependency_flags = vk::DependencyFlagBits::eByRegion;

    std::vector<vk::ImageMemoryBarrier> image_barriers = {};
};

struct UnsafeCommandBufferBuilderImageAspect {
    bool color;
    bool depth;
    bool stencil;

    vk::ImageAspectFlags to_vulkan() const;
};

struct UnsafeCommandBufferBuilderImageCopy {
    UnsafeCommandBufferBuilderImageAspect aspect;
    std::uint32_t source_mip_level;
    std::uint32_t destination_mip_level;
    std::uint32_t source_base_array_layer;
    std::uint32_t destination_base_array_layer;
    std::uint32_t layer_count;
    std::array<std::int32_t, 3> source_offset;
    std::array<std::int32_t, 3> destination_offset;
    std::array<std::uint32_t, 3> extent;
};

/**
 * Just add some nice wrappers around CommandBuffer build stage
 */
class UnsafeCommandBufferBuilder : public CommandBufferBuilder<UnsafeCommandBufferBuilder> {
public:
    static boost::leaf::result<UniqueUnsafeCommandBufferBuilder> build(
        CommandPool* pool,
        Kind kind,
        vk::CommandBufferUsageFlags usage = {}
    );

    boost::leaf::result<void> copy_image(
        const Image* source,
        vk::ImageLayout source_layout,
        const Image* destination,
        vk::ImageLayout destination_layout,
        const std::vector<UnsafeCommandBufferBuilderImageCopy>& regions
    );

    const Device* get_device() const;

private:
    UnsafeCommandBufferBuilder() = default;

    vk::CommandBuffer command_buffer_;
    UniqueCommandPoolAllocation allocation_;
};

}
