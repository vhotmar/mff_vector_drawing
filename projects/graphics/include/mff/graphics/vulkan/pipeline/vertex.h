#pragma once

#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan::pipeline {


struct BufferItem {
    std::uint32_t binding;
    std::size_t stride;
    vk::VertexInputRate input_rate;

    vk::VertexInputBindingDescription to_vulkan() const;
};

struct AttributesInfo {
    vk::Format format;
    std::uint32_t offset;
};

struct AttributesItem {
    std::uint32_t location;
    std::uint32_t binding;
    AttributesInfo info;

    vk::VertexInputAttributeDescription to_vulkan() const;
};

/**
 * Vertex input
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap21.html#VkPipelineVertexInputStateCreateInfo
 */
struct VertexDefinition {
    std::vector<BufferItem> buffers;
    std::vector<AttributesItem> attributes;

    vk::PipelineVertexInputStateCreateInfo to_vulkan() const;
};

}
