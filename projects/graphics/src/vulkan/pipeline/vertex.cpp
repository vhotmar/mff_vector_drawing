#include <mff/graphics/vulkan/pipeline/vertex.h>

#include <mff/algorithms.h>

namespace mff::vulkan::pipeline {

vk::PipelineVertexInputStateCreateInfo VertexDefinition::to_vulkan() const {
    std::vector<vk::VertexInputBindingDescription> binding_descriptions = mff::map(
        [](const auto& i) { return i.to_vulkan(); },
        buffers
    );

    std::vector<vk::VertexInputAttributeDescription> attribute_descriptions = mff::map(
        [](const auto& i) {
            return vk::VertexInputAttributeDescription(i.location, i.binding, i.info.format, i.info.offset);
        },
        attributes
    );

    return vk::PipelineVertexInputStateCreateInfo(
        {},
        binding_descriptions.size(),
        binding_descriptions.data(),
        attribute_descriptions.size(),
        attribute_descriptions.data());
}

vk::VertexInputBindingDescription BufferItem::to_vulkan() const {
    return vk::VertexInputBindingDescription(binding, stride, input_rate);
}

vk::VertexInputAttributeDescription AttributesItem::to_vulkan() const {
    return vk::VertexInputAttributeDescription(location, binding, info.format, info.offset);
}

}