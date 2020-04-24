#pragma once

#include <variant>

#include "../../eigen.h"
#include "../../leaf.h"
#include "../../utils.h"
#include "../../version.h"

#include "./device.h"
#include "./render_pass.h"

namespace mff::vulkan {

/**
 * Vertex input description
 */
namespace vertex {

struct BufferItem {
    std::uint32_t binding;
    std::size_t stride;
    vk::VertexInputRate input_rate;
};

struct AttributesInfo {
    vk::Format format;
    std::uint32_t offset;
};

struct AttributesItem {
    std::uint32_t location;
    std::uint32_t binding;
    AttributesInfo info;
};

/**
 * Vertex input
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap21.html#VkPipelineVertexInputStateCreateInfo
 */
struct DefinitionInfo {
    std::vector<BufferItem> buffers;
    std::vector<AttributesItem> attributes;

    vk::PipelineVertexInputStateCreateInfo to_vulkan() const;
};

}

namespace shader {

class ShaderModule {
private:
    vk::UniqueShaderModule handle_;
    std::shared_ptr<Device> device_;

public:
    vk::ShaderModule get_handle();
};


struct ShaderInterfaceElement {
    std::uint32_t location_start;
    std::uint32_t location_end;
    vk::Format format;
    std::optional<std::string> name;
};

struct InterfaceDefinition {
    std::vector<ShaderInterfaceElement> elements;
};

}

struct GraphicsEntryPoint {
    InterfaceDefinition input;
    InterfaceDefinition output;
    vk::ShaderStageFlag type;
    std::shared_ptr<ShaderModule> module;
    std::string name;
    PipelineLayoutInfo layout;
};


/**
 * Description of how should be the draw operation executed.
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap9.html#pipelines-graphics
 */
class GraphicsPipeline {
private:
    vk::UniquePipeline handle_;
    std::shared_ptr<Device> device_;

    GraphicsPipeline() = default;
};


}