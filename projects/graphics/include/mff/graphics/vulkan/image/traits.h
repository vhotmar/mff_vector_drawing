#pragma once

#include <mff/graphics/vulkan/device.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

class UnsafeImage {
private:
    vk::UniqueImage image_;
    const Device* device_;
    vk::ImageUsageFlagBits usage_;
    vk::Format format_;
    // dimensions
    std::uint32_t samples_;
    std::uint32_t mipmaps_;

public:
    vk::Format get_format() {
        return format_;
    }
};

struct ImageInner {
    const UnsafeImage* image;
    std::size_t first_layer;
    std::size_t num_layers;
};

class ImageAccess {
    virtual const ImageInner& inner() = 0;

    virtual const vk::Format format() {
        return inner().image->get_format();
    }

};


template <typename T>
class ImageAccessTrait {
public:
    static_assert(sizeof(T) == -1, "You have to have specialization for ImageAccessTrait");
};

namespace ImageAccess {

template <typename T>
ImageInner inner(const T& from) {
    ImageAccessTrait<T> trait;

    return trait.inner(from);
}


template <typename T>
ImageInner format(const T& from) {
    ImageAccessTrait<T> trait;

    if constexpr (boost::hana::is_valid([](auto&& trait) -> decltype(trait.format) {}, trait)) {
        return trait.format(from);
    } else {
        return trait.inner(from).image.format;
    }
}

}

}