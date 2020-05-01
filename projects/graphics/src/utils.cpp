#include <mff/graphics/utils.h>

namespace mff {

vk::Extent2D to_extent(Vector2ui v) {
    return vk::Extent2D(v[0], v[1]);
}

vk::Offset2D to_offset(Vector2ui v) {
    return vk::Offset2D(v[0], v[1]);
}

boost::leaf::result<void> to_result(VkResult vk_result) {
    if (vk_result != VK_SUCCESS) {
        return boost::leaf::new_error(static_cast<vk::Result>(vk_result));
    }

    return {};
}

}