#pragma once

#include <memory>
#include <vector>

#include <mff/leaf.h>
#include <mff/graphics/vulkan/descriptor/descriptor.h>
#include <mff/graphics/vulkan/device.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

/**
 *
 */
class PipelineLayout {
    std::shared_ptr<Device> device_;
    vk::UniquePipelineLayout handle_;

public:
    boost::leaf::result<std::shared_ptr<PipelineLayout>> build(
        const std::shared_ptr<Device>& device,
        const std::vector<std::vector<DescriptorInfo>>& layouts
    );
};

}