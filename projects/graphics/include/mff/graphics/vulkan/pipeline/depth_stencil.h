#pragma once

#include <optional>
#include <variant>

#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

/**
 * Describes how stencil should be used
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap27.html#VkStencilOpState
 */
struct Stencil {
    /**
     * The comparison to perform between the existing stencil value in the stencil buffer, and
     * the reference value (given by `reference`).
     */
    vk::CompareOp compare = vk::CompareOp::eNever;

    /**
     * Value specifying the action performed on samples that pass both the depth and stencil tests.
     */
    vk::StencilOp pass_op = vk::StencilOp::eKeep;

    /**
     * Value specifying the action performed on samples that fail the stencil test.
     */
    vk::StencilOp fail_op = vk::StencilOp::eKeep;

    /**
     * Value specifying the action performed on samples that pass the stencil test and fail the
     * depth test.
     */
    vk::StencilOp depth_fail_op = vk::StencilOp::eKeep;

    /**
     * Selects the bits of the unsigned integer stencil values participating in the stencil test.
     *
     * When std::nullopt then needs to be set dynamically
     */
    std::optional<std::uint32_t> compare_mask = std::numeric_limits<std::uint32_t>::max();

    /**
     * Selects the bits of the unsigned integer stencil values updated by the stencil test in the
     * stencil framebuffer attachment.
     *
     * When std::nullopt then needs to be set dynamically
     */
    std::optional<std::uint32_t> write_mask = std::numeric_limits<std::uint32_t>::max();

    /**
     * An integer reference value that is used in the unsigned stencil comparison.
     *
     * When std::nullopt then needs to be set dynamically
     */
    std::optional<std::uint32_t> reference = std::numeric_limits<std::uint32_t>::max();

    /**
     * Does stencil op always result in keep?
     */
    bool always_keep() const;

    /**
     * Convert this object to vulkan representation
     * @return vulkan representation
     */
    vk::StencilOpState to_vulkan() const;
};

template <typename T>
class VariantTrait {};

template <typename Parent, typename Item>
class VariantEnumItem {
public:
    operator Parent() {
        return Parent(typename Parent::type{*static_cast<Item*>(this)});
    };
};

template <typename T>
class VariantEnum {

    VariantEnum(const typename VariantTrait<T>::type& t);

    const typename VariantTrait<T>::type& get_inner() const;

private:
    typename VariantTrait<T>::type inner_;
};

class SampleEvent : VariantEnum<SampleEvent> {
    struct Open : VariantEnumItem<SampleEvent, Open> {};
    struct Resized : VariantEnumItem<SampleEvent, Open> {};
    struct Moved : VariantEnumItem<SampleEvent, Open> {};

    using type = std::variant<Open, Resized, Moved>;
};

/**
 * Describe depth bound testing part of DepthStencil. It allows you to ask the GPU to exclude
 * fragments that are outside of a certain range.
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap27.html#VkPipelineDepthStencilStateCreateInfo
 */
class DepthBounds : VariantEnum<DepthBounds> {
public:
    /**
     * There is no depth testing
     */
    struct Disabled : VariantItem<DepthBounds, Disabled> {};

    /**
     * The depth testing range is fixed
     */
    struct Fixed : VariantItem<DepthBounds, Fixed> {
        float from;
        float to;
    };

    /**
     * The depth testing is dynamic and will be set during render
     */
    struct Dynamic {};

    using type = std::variant<Disabled, Fixed, Dynamic>;
};

template <>
class VariantTrait<DepthBounds> {
    using type = std::variant<DepthBounds::Disabled, DepthBounds::Fixed, DepthBounds::Dynamic>;
};

/**
 * Configuration of depth and stencil tests.
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap27.html#fragops
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap27.html#VkPipelineDepthStencilStateCreateInfo
 */
struct DepthStencil {
    /**
     * Comparison to use between depth of current fragment and value in current depth buffer.
     */
    vk::CompareOp depth_compare = vk::CompareOp::eAlways;

    /**
     * If the depth comparison was succesful should it be written to depth buffer?
     */
    bool depth_write = false;

    /**
     * Allows you to ask the GPU to exclude fragments that are outside of a certain range.
     */
    DepthBounds depth_bounds_test = DepthBounds::Disabled{};

    /**
     * Stencil operations to use for points, lines and triangles whose front is facing the user.
     */
    Stencil stencil_front;

    /**
     * Stencil operations to use for points, lines and triangles whose back is facing the user.
     */
    Stencil stencil_back;

    /**
     * Convert this object to vulkan representation
     * @return vulkan representation
     */
    vk::PipelineDepthStencilStateCreateInfo to_vulkan() const;
};

}