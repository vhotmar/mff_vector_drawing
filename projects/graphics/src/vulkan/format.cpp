#include <mff/graphics/vulkan/format.h>

namespace mff::vulkan {

bool is_float(vk::Format format) {
    return mff::contains_if(
        kINFOS,
        [&](const auto& info) { return info.format == format && info.type == FormatType::Float; }
    );
}


bool is_sint(vk::Format format) {
    return mff::contains_if(
        kINFOS,
        [&](const auto& info) { return info.format == format && info.type == FormatType::Sint; }
    );
}


bool is_uint(vk::Format format) {
    return mff::contains_if(
        kINFOS,
        [&](const auto& info) { return info.format == format && info.type == FormatType::Uint; }
    );
}

bool is_depth(vk::Format format) {
    return mff::contains_if(
        kINFOS,
        [&](const auto& info) { return info.format == format && info.type == FormatType::Depth; }
    );
}

bool is_stencil(vk::Format format) {
    return mff::contains_if(
        kINFOS,
        [&](const auto& info) { return info.format == format && info.type == FormatType::Stencil; }
    );
}

bool is_depthstencil(vk::Format format) {
    return mff::contains_if(
        kINFOS,
        [&](const auto& info) { return info.format == format && info.type == FormatType::DepthStencil; }
    );
}

}