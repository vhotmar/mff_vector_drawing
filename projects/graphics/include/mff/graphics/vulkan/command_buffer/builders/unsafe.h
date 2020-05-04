#pragma once

#include <array>
#include <memory>

#include <mff/leaf.h>
#include <mff/graphics/vulkan/command_buffer/builders/base.h>
#include <mff/graphics/vulkan/command_buffer/command_buffer.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

class UnsafeCommandBuffer;
class UnsafeCommandBufferBuilder;
using UniqueUnsafeCommandBuffer = std::unique_ptr<UnsafeCommandBuffer>;
using UniqueUnsafeCommandBufferBuilder = std::unique_ptr<UnsafeCommandBufferBuilder>;

struct UnsafeCommandBufferBuilderPipelineBarrier {
    vk::PipelineStageFlags src_stage_mask = {};
    vk::PipelineStageFlags dst_stage_mask = {};
    vk::DependencyFlags dependency_flags = vk::DependencyFlagBits::eByRegion;

    std::vector<vk::MemoryBarrier> memory_barriers = {};
    std::vector<vk::BufferMemoryBarrier> buffer_barriers = {};
    std::vector<vk::ImageMemoryBarrier> image_barriers = {};

    bool is_empty() const;
    UnsafeCommandBufferBuilderPipelineBarrier merge(const UnsafeCommandBufferBuilderPipelineBarrier& other) const;
    UnsafeCommandBufferBuilderPipelineBarrier& add_execution_dependency(vk::PipelineStageFlags source, vk::PipelineStageFlags destination, bool by_region);

    UnsafeCommandBufferBuilderPipelineBarrier& add_image_memory_barrier(
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
    );
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

class UnsafeCommandBuffer {
    friend class UnsafeCommandBufferBuilder;

public:
    const Device* get_device() const;

private:
    UnsafeCommandBuffer() = default;

    vk::CommandBuffer command_buffer_;
    UniqueCommandPoolAllocation allocation_;
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

    // TODO: should not be here (only in Auto)
    static boost::leaf::result<UniqueUnsafeCommandBufferBuilder> build_primary(
        Device* device,
        const QueueFamily* family
    );

    static boost::leaf::result<UniqueUnsafeCommandBufferBuilder> from_buffer(
        vk::CommandBuffer cmd_buffer,
        vk::CommandBufferUsageFlags usage
    );

    void pipeline_barrier(UnsafeCommandBufferBuilderPipelineBarrier command);

    void copy_image(
        const Image* source,
        vk::ImageLayout source_layout,
        const Image* destination,
        vk::ImageLayout destination_layout,
        const std::vector<UnsafeCommandBufferBuilderImageCopy>& regions
    );

    boost::leaf::result<UniqueUnsafeCommandBuffer> build();

    const Device* get_device() const;

private:
    UnsafeCommandBufferBuilder() = default;

    vk::CommandBuffer command_buffer_;
    UniqueCommandPoolAllocation allocation_;
};

}
