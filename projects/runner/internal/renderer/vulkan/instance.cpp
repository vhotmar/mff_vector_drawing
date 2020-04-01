#include "instance.h"

#include <fmt/format.h>
#include <mff/algorithms.h>
#include <set>
#include <string>
#include <vector>

#include "../../constants.h"
#include "../../window/context.h"
#include "./debug.h"
#include "./dispatcher.h"
#include "./layers.h"
#include "../utils.h"

namespace mff::internal::renderer::vulkan {

std::vector<std::string> get_required_glfw_instance_extensions() {
    auto context = window::detail::create_glfw_context();

    unsigned int glfwExtensionCount;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    return std::vector<std::string>(glfwExtensions, glfwExtensions + glfwExtensionCount);
}

std::vector<std::string> get_required_mff_instance_extensions() {
    std::vector<std::string> validation_extensions = {};

    if (constants::kVULKAN_DEBUG) {
        validation_extensions = {VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    }

    std::vector<std::string> extensions = {};

    extensions.insert(
        std::end(extensions),
        std::begin(validation_extensions),
        std::end(validation_extensions)
    );

    return extensions;
}

std::set<std::string> get_required_instance_extensions() {
    auto glfw_extensions = get_required_glfw_instance_extensions();
    auto mff_extensions = get_required_mff_instance_extensions();

    std::set<std::string> extensions;

    extensions.insert(std::begin(glfw_extensions), std::end(glfw_extensions));
    extensions.insert(std::begin(mff_extensions), std::end(mff_extensions));

    return extensions;
}

tl::expected<InstanceRaw, std::string> create_instance(const std::string& name) {
    init_dispatcher();

    vk::ApplicationInfo application_info(
        name.c_str(),
        VK_MAKE_VERSION(1, 0, 0),
        "mff",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_2
    );

    auto required_extensions = get_required_instance_extensions();
    auto instance_extensions = VK_TRY(vk::enumerateInstanceExtensionProperties());

    // check whether we can use the extensions
    for (const auto& extension: required_extensions) {
        if (!mff::contains_if(instance_extensions, [&](auto item) { return item.extensionName == extension; })) {
            return tl::make_unexpected(fmt::format("Could not find extension: \"{}\"", extension));
        }
    }

    auto required_layers = get_required_mff_layers();
    auto instance_layers = VK_TRY(vk::enumerateInstanceLayerProperties());

    // check whether we can use the layers
    for (const auto& layer: required_layers) {
        if (!mff::contains_if(instance_layers, [&](auto item) { return item.layerName == layer; })) {
            return tl::make_unexpected(fmt::format("Could not find layer: \"{}\"", layer));
        }
    }

    auto required_extensions_c = utils::to_pointer_char_data(required_extensions);
    auto layers_c = utils::to_pointer_char_data(required_layers);

    vk::InstanceCreateInfo create_instance_info(
        {},
        &application_info,
        layers_c.size(),
        layers_c.data(),
        required_extensions_c.size(),
        required_extensions_c.data()
    );

    vk::UniqueInstance instance;

    if (constants::kVULKAN_DEBUG) {
        vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT> chain = {
            create_instance_info,
            get_debug_utils_create_info()
        };

        instance = VK_TRY(vk::createInstanceUnique(chain.get<vk::InstanceCreateInfo>()));
    } else {
        instance = VK_TRY(vk::createInstanceUnique(create_instance_info));
    }

    init_dispatcher(*instance);

    vk::UniqueDebugUtilsMessengerEXT messenger;

    if (constants::kVULKAN_DEBUG) {
        auto create_info = get_debug_utils_create_info();
        messenger = VK_TRY(instance->createDebugUtilsMessengerEXTUnique(create_info));
    }

    return InstanceRaw{ std::move(instance), std::move(messenger) };
}

}
