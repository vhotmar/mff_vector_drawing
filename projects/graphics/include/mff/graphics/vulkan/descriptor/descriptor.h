#pragma once

#include <optional>
#include <variant>

#include <mff/leaf.h>
#include <mff/graphics/math.h>
#include <mff/graphics/vulkan/device.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

/**
 * Definition of single descriptor
 */
struct DescriptorInfo {
    vk::DescriptorType type; // TODO: add wrapper
    std::uint32_t array_count = 0;
    vk::ShaderStageFlags stages;
    bool readonly;

    vk::DescriptorSetLayoutBinding to_vulkan(std::uint32_t binding) const;

    std::optional<DescriptorInfo> make_union(const DescriptorInfo& other) const;
};

struct DescriptorSetInfo {
    std::vector<std::optional<DescriptorInfo>> descriptors;

    DescriptorSetInfo make_union(const DescriptorSetInfo& other) const;
};

/**
 * Describes to the Vulkan implementation the layout of all descriptors within a descriptor set.
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap13.html#descriptorsets-setlayout
 */
class DescriptorSetLayout {
private:
    const Device* device_;
    vk::UniqueDescriptorSetLayout handle_;

    DescriptorSetLayout() = default;

public:
    vk::DescriptorSetLayout get_handle() const;

    /**
     * Create DescriptorSetLayout given the specified the DescriptorInfos
     *
     * @param device Device on which to create this DescriptorSetLayout
     * @param infos Infos describing the layout
     * @return
     */
    static boost::leaf::result<std::unique_ptr<DescriptorSetLayout>> build(
        const Device* device,
        const DescriptorSetInfo& set_info
    );
};

}