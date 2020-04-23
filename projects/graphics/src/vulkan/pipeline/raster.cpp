#include <mff/graphics/vulkan/pipeline/raster.h>

#include <mff/utils.h>

namespace mff::vulkan {

vk::PipelineRasterizationStateCreateInfo Rasterization::to_vulkan() const {
    using dp_info = std::tuple<bool, float, float, float>;
    auto[db_enable, db_const, db_clamp, db_slope] = std::visit(
        overloaded{
            [](DepthBiasControl_::Disabled d) -> dp_info {
                return std::make_tuple(true, 0.0f, 0.0f, 0.0f);
            },
            [](DepthBiasControl_::Static s) -> dp_info {
                return std::make_tuple(true, s.bias.constant_factor, s.bias.clamp, s.bias.slope_factor);
            },
        },
        depth_bias
    );

    return vk::PipelineRasterizationStateCreateInfo(
        {},
        depth_clamp,
        rasterizer_discard,
        polygon_mode,
        cull_mode,
        front_face,
        db_enable,
        db_const,
        db_clamp,
        db_slope,
        line_width.value_or(1.0)
    );
}

}