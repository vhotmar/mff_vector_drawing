#include <mff/graphics/vulkan/descriptor/descriptor.h>

#include "../../utils.h"

namespace mff::vulkan {

vk::DescriptorSetLayoutBinding DescriptorInfo::to_vulkan(std::uint32_t binding) const {
    return vk::DescriptorSetLayoutBinding(binding, type, array_count, stages, nullptr);
}

std::optional<DescriptorInfo> DescriptorInfo::make_union(const DescriptorInfo& other) const {
    if (type == other.type) {
        return std::nullopt;
    }

    return DescriptorInfo{
        type,
        std::max(array_count, other.array_count),
        stages | other.stages,
        readonly && other.readonly
    };
}

boost::leaf::result<std::shared_ptr<DescriptorSetLayout>> DescriptorSetLayout::build(
    const std::shared_ptr<Device>& device,
    const DescriptorSetInfo& set_info
) {
    std::vector<vk::DescriptorSetLayoutBinding> bindings;

    for (const auto& info: set_info.descriptors) {
        if (!info) continue;
        bindings.push_back(info.value().to_vulkan(bindings.size()));
    }

    auto info = vk::DescriptorSetLayoutCreateInfo({}, bindings.size(), bindings.data());

    struct enable_DescriptorSetLayout : public DescriptorSetLayout {};
    std::shared_ptr<DescriptorSetLayout> desc = std::make_shared<enable_DescriptorSetLayout>();

    LEAF_AUTO_TO(desc->handle_, to_result(device->get_handle().createDescriptorSetLayoutUnique(info)));
    desc->device_ = device;

    return desc;
}

DescriptorSetInfo DescriptorSetInfo::make_union(const DescriptorSetInfo& other) const {
    auto get_item = [](const auto& vector, std::size_t index) -> std::optional<DescriptorInfo> {
        if (index >= vector.size())return std::nullopt;

        return vector[index];
    };

    auto get_merged = [&](std::size_t index) -> std::optional<DescriptorInfo> {
        auto a = get_item(descriptors, index);
        auto b = get_item(other.descriptors, index);

        if (a && b) {
            return a.value().make_union(b.value());
        } else if (a) {
            return a;
        } else if (b) {
            return b;
        }

        return std::nullopt;
    };

    auto final_size = std::max(other.descriptors.size(), descriptors.size());
    std::vector<std::optional<DescriptorInfo>> infos;

    for (std::size_t i = 0; i < final_size; i++) {
        infos.push_back(get_merged(i));
    }

    return DescriptorSetInfo{infos};
}

vk::DescriptorSetLayout DescriptorSetLayout::get_handle() const {
    return handle_.get();
}
}