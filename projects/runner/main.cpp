#include <iostream>

#include <mff/algorithms.h>

#include "./internal/renderer/vulkan/instance.h"
#include "./internal/renderer/vulkan/device.h"
#include "./internal/renderer/vulkan/swapchain.h"
#include "./internal/window/events_debug.h"
#include "./internal/window/event_loop.h"
#include "./internal/window/window.h"
#include "./utils/logger.h"

#include <vk_mem_alloc.h>

/*std::vector<std::string> get_required_glfw_instance_extensions() {
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
}*/

struct QueueFamilyIndices {
    std::optional<mff::internal::renderer::vulkan::QueueFamily> graphics_family;
    std::optional<mff::internal::renderer::vulkan::QueueFamily> present_family;

    bool is_complete() {
        return graphics_family.has_value() && present_family.has_value();
    }
};

QueueFamilyIndices find_queue_families(
    const std::shared_ptr<mff::internal::renderer::vulkan::PhysicalDevice>& physical_device,
    const std::shared_ptr<mff::internal::renderer::vulkan::Surface>& surface
) {
    QueueFamilyIndices indices;

    auto queue_families = physical_device->get_queue_families();

    for (const auto& family: queue_families) {
        if (family.supports_graphics()) {
            indices.graphics_family = family;
        }

        if (surface->is_supported(family)) {
            indices.present_family = family;
        }

        if (indices.is_complete()) {
            break;
        }
    }

    return indices;
}

bool is_device_suitable(
    const std::shared_ptr<mff::internal::renderer::vulkan::PhysicalDevice>& physical_device,
    const std::shared_ptr<mff::internal::renderer::vulkan::Surface>& surface,
    const std::vector<std::string>& required_extensions
) {
    bool is_discrete = physical_device->get_type() == vk::PhysicalDeviceType::eDiscreteGpu;
    bool has_queue_families = find_queue_families(physical_device, surface).is_complete();

    bool required_extensions_supported = true;

    const auto& extensions = physical_device->get_extensions();
    for (auto required_extension: required_extensions) {
        if (!mff::contains_if(
            extensions,
            [&](auto extension) { return extension.extensionName == required_extension; }
        )) {
            required_extensions_supported = false;
            break;
        }
    }

    bool swapchain_adequate = false;

    // we can't query for swapchain support if the swapchain extension is not supported
    if (required_extensions_supported) {
        auto capabilities_result = surface->get_capabilities(physical_device);

        if (capabilities_result) {
            auto details = capabilities_result.value();
            swapchain_adequate = !details.supported_formats.empty() && !details.present_modes.empty();
        }
    }

    return is_discrete
        && has_queue_families
        && required_extensions_supported
        && swapchain_adequate;
}

vk::SurfaceFormatKHR choose_swap_surface_format(const mff::internal::renderer::vulkan::Capabilities& capabilities) {
    for (const auto& available_format : capabilities.supported_formats) {
        if (available_format.format == vk::Format::eB8G8R8A8Srgb
            && available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return available_format;
        }
    }

    return capabilities.supported_formats.front();
}

vk::PresentModeKHR choose_swap_present_mode(const mff::internal::renderer::vulkan::Capabilities& capabilities) {
    for (const auto& available_present_mode : capabilities.present_modes) {
        if (available_present_mode == vk::PresentModeKHR::eMailbox) {
            return available_present_mode;
        }
    }

    return capabilities.present_modes.front();
}

mff::Vector2ui choose_swap_extent(
    const mff::internal::renderer::vulkan::Capabilities& capabilities,
    mff::Vector2ui actual_extent
) {
    if (capabilities.current_extent) {
        return capabilities.current_extent.value();
    } else {
        return mff::Vector2ui(
            std::max(capabilities.min_image_extent[0], std::min(capabilities.max_image_extent[0], actual_extent[0])),
            std::max(capabilities.min_image_extent[1], std::min(capabilities.max_image_extent[1], actual_extent[1]))
        );
    }
}

boost::leaf::result<void> run() {
    namespace mwin = mff::internal::window;
    namespace vulkan = mff::internal::renderer::vulkan;

    mwin::EventLoop event_loop;

    auto window = mwin::WindowBuilder()
        .with_size({400, 400})
        .with_title("My app")
        .build(&event_loop);

    LEAF_AUTO(instance, vulkan::Instance::build(std::nullopt, window->get_required_extensions(), {}));
    LEAF_AUTO(surface, vulkan::Surface::build(window, instance));
    auto physical_device = mff::find_if(
        instance->get_physical_devices(),
        [&](auto device) { return is_device_suitable(device, surface, {}); }
    ).value();
    auto indices = find_queue_families(*physical_device, surface);
    std::vector<vulkan::QueueFamily> queue_infos{indices.graphics_family.value(), indices.present_family.value()};
    std::vector<std::string> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    LEAF_AUTO(device_result, vulkan::Device::build(*physical_device, queue_infos, extensions));
    auto[device, queues] = std::move(device_result);

    {
        auto capabilities = surface->get_capabilities(*physical_device).value();

        auto surface_format = choose_swap_surface_format(capabilities);
        auto present_mode = choose_swap_present_mode(capabilities);
        auto extent = choose_swap_extent(capabilities, window->get_inner_size());

        auto image_count = capabilities.min_image_count + 1;

        if (capabilities.max_image_count && image_count > capabilities.max_image_count.value()) {
            image_count = capabilities.max_image_count.value();
        }

        LEAF_AUTO(
            swapchain_result,
            vulkan::Swapchain::build(
                device,
                surface,
                image_count,
                surface_format.format,
                extent,
                1,
                capabilities.supported_usage_flags | vk::ImageUsageFlags(vk::ImageUsageFlagBits::eColorAttachment),
                vulkan::get_sharing_mode(queue_infos),
                capabilities.current_transform,
                vk::CompositeAlphaFlagBitsKHR::eOpaque,
                present_mode,
                true,
                surface_format.colorSpace
            ));
    }

    //instance.get_physical_devices()[0].

    event_loop.run(
        [&](auto event) {
            if (auto window_event = std::get_if<mwin::events::WindowEvent>(&event)) {
                if (std::holds_alternative<mwin::events::window::CloseRequested>(window_event->event)) {
                    logger::main->info("Quitting application");
                    return mwin::ExecutionControl::Terminate;
                }
            }

            return mwin::ExecutionControl::Wait;
        }
    );

    return {};
}

int main() {
    return boost::leaf::try_handle_all(
        [&]() -> boost::leaf::result<int> {
            LEAF_CHECK(run());

            return 0;
        },
        [](boost::leaf::error_info const& unmatched) {
            std::cerr <<
                "Unknown failure detected" << std::endl <<
                "Cryptic diagnostic information follows" << std::endl <<
                unmatched;
            return 6;
        }
    );
}

