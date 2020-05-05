#include "./vulkan_shaders.h"


boost::leaf::result<vk::UniqueShaderModule> create_shader_module(
    const mff::vulkan::Device* device,
    const std::vector<char>& code
) {
    vk::ShaderModuleCreateInfo create_info(
        {},
        code.size() * sizeof(char),
        reinterpret_cast<const std::uint32_t*>(code.data()));

    return mff::to_result(device->get_handle().createShaderModuleUnique(create_info));
}

boost::leaf::result<vk::UniqueShaderModule> create_shader_module(
    const mff::vulkan::Device* device,
    const std::string& path
) {
    return create_shader_module(device, mff::read_file(path));
}