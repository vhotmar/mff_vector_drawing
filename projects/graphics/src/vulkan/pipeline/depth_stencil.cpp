#include <mff/graphics/vulkan/pipeline/depth_stencil.h>

#include <mff/utils.h>

namespace mff::vulkan {

bool Stencil::always_keep() const {
    if (compare == vk::CompareOp::eAlways)
        return pass_op == vk::StencilOp::eKeep && depth_fail_op == vk::StencilOp::eKeep;
    if (compare == vk::CompareOp::eNever) return fail_op == vk::StencilOp::eKeep;

    return pass_op == vk::StencilOp::eKeep && fail_op == vk::StencilOp::eKeep
        && depth_fail_op == vk::StencilOp::eKeep;
}

vk::StencilOpState Stencil::to_vulkan() const {
    return vk::StencilOpState(
        fail_op,
        pass_op,
        depth_fail_op,
        compare,
        compare_mask.value_or(std::numeric_limits<std::uint32_t>::max()), // TODO: ALL
        write_mask.value_or(std::numeric_limits<std::uint32_t>::max()), // TODO: ALL
        reference.value_or(0)
    );
}


vk::PipelineDepthStencilStateCreateInfo DepthStencil::to_vulkan() const {
    using db_info = std::tuple<bool, float, float>;
    auto[db_enabled, db_start, db_end] = std::visit(
        overloaded{
            [](DepthBounds_::Disabled d) -> db_info {
                return std::make_tuple(false, 0.0f, 0.0f);
            },
            [](DepthBounds_::Dynamic d) -> db_info {
                return std::make_tuple(true, 0.0f, 1.0f);
            },
            [](DepthBounds_::Fixed f) -> db_info {
                return std::make_tuple(true, f.from, f.to);
            }
        },
        depth_bounds_test
    );

    return vk::PipelineDepthStencilStateCreateInfo(
        {},
        !depth_write && depth_compare == vk::CompareOp::eAlways,
        depth_write,
        depth_compare,
        db_enabled,
        stencil_front.always_keep() && stencil_back.always_keep(),
        stencil_front.to_vulkan(),
        stencil_back.to_vulkan(),
        db_start,
        db_end
    );
}

}