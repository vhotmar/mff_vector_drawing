#include <mff/graphics/vulkan/version.h>

namespace mff::vulkan {

Version::Version(std::uint8_t maj, std::uint8_t min, std::uint8_t pat)
    : major(maj), minor(min), patch(pat) {
}

std::uint32_t Version::to_vulkan() const {
    return VK_MAKE_VERSION(major, minor, patch);
}

}