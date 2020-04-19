#pragma once

#include <variant>

#include "../../vulkan.h"
#include "./instance.h"

namespace mff::internal::renderer::vulkan {

struct ExclusiveSharingMode {};
struct ConcurrentSharingMode {
    std::vector<std::uint32_t> queue_families;
};

using SharingMode = std::variant<ExclusiveSharingMode, ConcurrentSharingMode>;

SharingMode get_sharing_mode(const std::vector<QueueFamily>& queue_families = {});

}