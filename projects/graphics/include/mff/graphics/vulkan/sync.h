#pragma once

#include <memory>
#include <utility>
#include <variant>
#include <vector>

#include <mff/graphics/vulkan/vulkan.h>
#include <mff/graphics/vulkan/instance.h>

namespace mff::vulkan {

namespace SharingMode_ {

/**
 * Resource is owned by one QueueFamily and just this QueueFamily has access to it
 */
struct Exclusive {};

/**
 * Resource is not owned by any concrete QueueFamily.
 */
struct Concurrent {
    std::vector<std::uint32_t> queue_families;
};

}

/**
 * SharingMode specifies how different resources can be accessed from different queues.
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap11.html#VkSharingMode
 */
using SharingMode = std::variant<SharingMode_::Exclusive, SharingMode_::Concurrent>;

SharingMode get_sharing_mode(const std::vector<std::shared_ptr<QueueFamily>>& queue_families = {});

}