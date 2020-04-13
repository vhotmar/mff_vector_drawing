#pragma once

#include <string>

#include "../../leaf.h"
#include "../../vulkan.h"

namespace mff::internal::renderer::vulkan {

struct QueueRaw {
    vk::Queue graphics;
    vk::Queue present;
};

struct DeviceRaw {
    vk::UniqueDevice device;
    QueueRaw queues;
};

boost::leaf::result<DeviceRaw> create_logical_device(vk::PhysicalDevice device, vk::SurfaceKHR surface);

}