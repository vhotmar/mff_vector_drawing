#pragma once

#include <mff/graphics/vulkan/vulkan.h>
#include <utility>

namespace mff::vulkan {

/**
 * Vulkan API version
 */
class Version {
public:
    Version(std::uint8_t maj, std::uint8_t min, std::uint8_t pat);

    std::uint8_t major = 0;
    std::uint8_t minor = 0;
    std::uint8_t patch = 0;

    /**
     * Convert the version to int usable by vulkan
     * @return int casted version
     */
    std::uint32_t to_vulkan() const;
};

}
