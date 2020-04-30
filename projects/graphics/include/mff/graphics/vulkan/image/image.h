#pragma once

#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

class ImageDimensions {
public:
    struct Dim1d {
        std::uint32_t width;
        std::uint32_t array_layers;
    };

    struct Dim2d {
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t array_layers;
        bool cubemap_compatible;
    };

    struct Dim3d {
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t depth;
    };

    using type = std::variant<Dim1d, Dim2d, Dim3d>;

private:
    type inner_;

public:
    type& get_inner();
};

}