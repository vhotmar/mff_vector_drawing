#include <mff/graphics/utils.h>

namespace mff {

vk::Extent2D to_extent(Vector2ui v) {
    return vk::Extent2D(v[0], v[1]);
}

vk::Extent3D to_extent(std::array<std::uint32_t, 3> arr) {
    return vk::Extent3D(arr[0], arr[1], arr[2]);
}

vk::Offset2D to_offset(Vector2ui v) {
    return vk::Offset2D(v[0], v[1]);
}

vk::Offset3D to_extent(std::array<std::int32_t, 3> arr) {
    return vk::Offset3D(arr[0], arr[1], arr[2]);
}

boost::leaf::result<void> to_result(VkResult vk_result) {
    if (vk_result != VK_SUCCESS) {
        return boost::leaf::new_error(static_cast<vk::Result>(vk_result));
    }

    return {};
}

boost::leaf::result<void> to_result(vk::Result vk_result) {
    if (vk_result != vk::Result::eSuccess) {
        return boost::leaf::new_error(vk_result);
    }

    return {};
}

}