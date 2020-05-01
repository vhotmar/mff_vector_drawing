#pragma once

#include <mff/leaf.h>
#include <mff/graphics/vulkan/vulkan.h>
#include <mff/graphics/math.h>

namespace boost::leaf {

template <>
struct is_e_type<vk::Result> : public std::true_type {};

}

namespace mff {


vk::Extent2D to_extent(Vector2ui v);

vk::Offset2D to_offset(Vector2ui v);


template <typename T>
boost::leaf::result<T> to_result(vk::ResultValue<T> vk_result) {
    if (vk_result.result != vk::Result::eSuccess) {
        return boost::leaf::new_error(vk_result.result);
    }

    return std::move(vk_result.value);
}

boost::leaf::result<void> to_result(VkResult vk_result);

namespace utils {

template <typename TContainer>
std::vector<const char*> to_pointer_char_data(const TContainer& data) {
    std::vector<const char*> result;

    for (const auto& item: data)
        result.push_back(&item.front());

    return result;
}

}

};
