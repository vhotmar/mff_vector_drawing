#pragma once

#include <mff/leaf.h>
#include <mff/graphics/vulkan/device.h>
#include <mff/graphics/vulkan/vulkan.h>

boost::leaf::result<vk::UniqueShaderModule> create_shader_module(
    const mff::vulkan::Device* device,
    const std::vector<char>& code
);

boost::leaf::result<vk::UniqueShaderModule> create_shader_module(
    const mff::vulkan::Device* device,
    const std::string& path
);
