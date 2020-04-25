#pragma once

#include <memory>
#include <vector>

#include <mff/leaf.h>
#include <mff/graphics/vulkan/descriptor/descriptor.h>
#include <mff/graphics/vulkan/device.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

struct PipelineLayoutInfo {
    std::vector<DescriptorSetInfo> layout_infos;

    PipelineLayoutInfo make_union(const PipelineLayoutInfo& other) const;
};

/**
 *
 */
class PipelineLayout {
    std::shared_ptr<Device> device_;
    vk::UniquePipelineLayout handle_;

public:
    vk::PipelineLayout get_handle() const;

    static boost::leaf::result<std::shared_ptr<PipelineLayout>> build(
        const std::shared_ptr<Device>& device,
        const PipelineLayoutInfo& layout_info
    );
};

}