#pragma once

#include "../../vulkan.h"

namespace mff::vulkan {

void init_dispatcher();

void init_dispatcher(vk::Instance instance);

void init_dispatcher(vk::Device device);

}