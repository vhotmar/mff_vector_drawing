#include <mff/graphics/vulkan/pipeline/viewport.h>

#include <mff/algorithms.h>
#include <mff/utils.h>

#include <mff/graphics/utils.h>

namespace mff::vulkan {

vk::PipelineViewportStateCreateInfo to_vulkan(const ViewportsState& state) {
    using vp_info = std::tuple<std::vector<vk::Viewport>, std::vector<vk::Rect2D>, std::uint32_t>;
    auto[vp_vp, vp_sc, vp_num] = std::visit(
        overloaded{
            [](ViewportsState_::Fixed f) -> vp_info {
                return std::make_tuple(
                    mff::map(
                        [](auto it) { return std::get<0>(it).to_vulkan(); },
                        f.data
                    ),
                    mff::map(
                        [](auto it) { return std::get<1>(it).to_vulkan(); },
                        f.data
                    ),
                    f.data.size()
                );
            },
            [](ViewportsState_::Dynamic d) -> vp_info {
                return vp_info({}, {}, d.num);
            },
        },
        state
    );

    return vk::PipelineViewportStateCreateInfo(
        {},
        vp_num,
        vp_num == 0 ? nullptr : vp_vp.data(),
        vp_num,
        vp_num == 0 ? nullptr : vp_sc.data()
    );
}

vk::Viewport Viewport::to_vulkan() const {
    return vk::Viewport(
        origin[0],
        origin[1],
        dimensions[0],
        dimensions[1],
        depth_range_from,
        depth_range_to
    );
}

vk::Rect2D Scissor::to_vulkan() const {
    return vk::Rect2D(
        to_offset(origin),
        to_extent(dimensions)
    );
}

}