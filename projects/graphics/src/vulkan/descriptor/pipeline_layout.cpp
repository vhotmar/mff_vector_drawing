#include <mff/graphics/vulkan/descriptor/pipeline_layout.h>

#include <mff/algorithms.h>

#include "../../utils.h"

namespace mff::vulkan {

boost::leaf::result<std::shared_ptr<PipelineLayout>> PipelineLayout::build(
    const std::shared_ptr<Device>& device,
    const std::vector<std::vector<DescriptorInfo>>& layout_infos
) {
    std::vector<std::shared_ptr<DescriptorSetLayout>> layouts;

    for (const auto& infos: layout_infos) {
        LEAF_AUTO(item, DescriptorSetLayout::build(device, infos));

        layouts.push_back(std::move(item));
    }

    std::vector<vk::DescriptorSetLayout> layout_handles = mff::map(
        [](const auto& info) { return info->get_handle(); },
        layouts
    );

    auto info = vk::PipelineLayoutCreateInfo({}, layout_handles.size(), layout_handles.data(), 0, nullptr);

    struct enable_PipelineLayout : public PipelineLayout {};
    std::shared_ptr<PipelineLayout> result = std::make_shared<enable_PipelineLayout>();

    result->device_ = device;
    LEAF_AUTO_TO(result->handle_, to_result(device->get_handle().createPipelineLayoutUnique(info)));

    return result;
}
}