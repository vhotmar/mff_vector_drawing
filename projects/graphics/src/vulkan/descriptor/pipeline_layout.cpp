#include <mff/graphics/vulkan/descriptor/pipeline_layout.h>

#include <range/v3/all.hpp>
#include <mff/algorithms.h>
#include <mff/graphics/utils.h>

namespace mff::vulkan {

boost::leaf::result<std::unique_ptr<PipelineLayout>> PipelineLayout::build(
    const Device* device,
    const PipelineLayoutInfo& layout_info
) {
    std::vector<std::unique_ptr<DescriptorSetLayout>> layouts;

    for (const auto& info: layout_info.layout_infos) {
        LEAF_AUTO(item, DescriptorSetLayout::build(device, info));

        layouts.push_back(std::move(item));
    }

    auto layout_handles = layouts
        | ranges::views::transform(
            [](const auto& info) {
                return info->get_handle();
            }
        )
        | ranges::to<std::vector>();

    auto info = vk::PipelineLayoutCreateInfo(
        {},
        layout_handles.size(),
        layout_handles.data(),
        0,
        nullptr
    );

    struct enable_PipelineLayout : public PipelineLayout {};
    std::unique_ptr<PipelineLayout> result = std::make_unique<enable_PipelineLayout>();

    result->device_ = device;
    LEAF_AUTO_TO(result->handle_, to_result(device->get_handle().createPipelineLayoutUnique(info)));

    return result;
}

vk::PipelineLayout PipelineLayout::get_handle() const {
    return handle_.get();
}

PipelineLayoutInfo PipelineLayoutInfo::make_union(const PipelineLayoutInfo& other) const {
    auto max_size = std::max(layout_infos.size(), other.layout_infos.size());

    std::vector<DescriptorSetInfo> new_layout_infos;
    new_layout_infos.reserve(max_size);

    for (int i = 0; i < max_size; i++) {
        auto a_contains_i = i < layout_infos.size();
        auto b_contains_i = i < other.layout_infos.size();

        if (a_contains_i && b_contains_i) {
            new_layout_infos.push_back(layout_infos[i].make_union(other.layout_infos[i]));
        } else if (a_contains_i) {
            new_layout_infos.push_back(layout_infos[i]);
        }
        {
            new_layout_infos.push_back(other.layout_infos[i]);
        }
    }

    return PipelineLayoutInfo{new_layout_infos};
}

}