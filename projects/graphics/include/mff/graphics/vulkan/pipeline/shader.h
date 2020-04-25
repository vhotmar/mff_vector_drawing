#pragma once

#include <optional>
#include <vector>

#include <mff/graphics/vulkan/descriptor/pipeline_layout.h>
#include <mff/graphics/vulkan/device.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

struct ShaderInterfaceElementInfo {
    std::uint32_t location_from;
    std::uint32_t location_to;
    vk::Format format;
    std::optional<std::string> name;
};

struct ShaderInterfaceInfo {
    std::vector<ShaderInterfaceElementInfo> elements;
};

class ShaderModule {
private:
    vk::UniqueShaderModule handle_;
    std::shared_ptr<Device> device_;

public:
    vk::ShaderModule get_handle() const;
};

struct GraphicsEntryPoint {
    std::shared_ptr<ShaderModule> module;
    std::string name;
    PipelineLayoutInfo layout;
    ShaderInterfaceInfo input;
    ShaderInterfaceInfo output;
    vk::ShaderStageFlagBits type;
    // TODO: type_
};

}
