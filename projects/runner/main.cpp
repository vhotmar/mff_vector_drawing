#include <iostream>

#include <mff/algorithms.h>
#include <mff/graphics/vulkan/device.h>
#include <mff/graphics/vulkan/instance.h>
#include <mff/graphics/vulkan/swapchain.h>
#include <mff/graphics/window.h>

#include "./utils/logger.h"

class Sample {
private:
    std::unique_ptr<std::string> handle_;

    Sample() = default;

public:
    std::string* get_handle() {
        return handle_.get();
    }

    static Sample build(std::string handle) {
        Sample result;

        result.handle_ = std::make_unique<std::string>(handle);


        return result;
    }
};

int main() {
    std::cout << "Happy!" << std::endl;

    auto s1 = Sample::build("mama mia");
    auto s2 = std::make_shared<Sample>(std::move(s1));

    //std::cout << "MamaMia - " << *s1.get_handle() << std::endl;
    std::cout << "MamaMia - " << *s2->get_handle() << std::endl;
}

/*struct QueueFamilyIndices {
    std::optional<mff::vulkan::QueueFamily> graphics_family;
    std::optional<mff::vulkan::QueueFamily> present_family;

    bool is_complete() {
        return graphics_family.has_value() && present_family.has_value();
    }
};

QueueFamilyIndices find_queue_families(
    const std::shared_ptr<mff::vulkan::PhysicalDevice>& physical_device,
    const std::shared_ptr<mff::vulkan::Surface>& surface
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
    const std::shared_ptr<mff::vulkan::PhysicalDevice>& physical_device,
    const std::shared_ptr<mff::vulkan::Surface>& surface,
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

vk::SurfaceFormatKHR choose_swap_surface_format(const mff::vulkan::Capabilities& capabilities) {
    for (const auto& available_format : capabilities.supported_formats) {
        if (available_format.format == vk::Format::eB8G8R8A8Srgb
            && available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return available_format;
        }
    }

    return capabilities.supported_formats.front();
}

vk::PresentModeKHR choose_swap_present_mode(const mff::vulkan::Capabilities& capabilities) {
    for (const auto& available_present_mode : capabilities.present_modes) {
        if (available_present_mode == vk::PresentModeKHR::eMailbox) {
            return available_present_mode;
        }
    }

    return capabilities.present_modes.front();
}

mff::Vector2ui choose_swap_extent(
    const mff::vulkan::Capabilities& capabilities,
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
    mff::window::EventLoop event_loop;

    LEAF_AUTO(window, mff::window::glfw::WindowBuilder()
        .with_size({400, 400})
        .with_title("My app")
        .build(&event_loop));

    LEAF_AUTO(instance, mff::vulkan::Instance::build(std::nullopt, window->get_required_extensions(), {}));
    LEAF_AUTO(surface, mff::vulkan::Surface::build(window, instance));

    auto physical_device = mff::find_if(
        instance->get_physical_devices(),
        [&](auto device) { return is_device_suitable(device, surface, {}); }
    ).value();

    auto queue_infos = [&]() {
        auto indices = find_queue_families(*physical_device, surface);
        return std::vector<mff::vulkan::QueueFamily>{indices.graphics_family.value(), indices.present_family.value()};
    }();

    auto build_device = [&]() {
        std::vector<std::string> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        return mff::vulkan::Device::build(*physical_device, queue_infos, extensions);
    };

    LEAF_AUTO(device_result, build_device());
    auto device = std::move(std::get<0>(device_result));
    auto queues = std::move(std::get<1>(device_result));

    auto build_swapchain = [&]() {
        auto capabilities = surface->get_capabilities(*physical_device).value();

        auto surface_format = choose_swap_surface_format(capabilities);
        auto present_mode = choose_swap_present_mode(capabilities);
        auto extent = choose_swap_extent(capabilities, window->get_inner_size());

        auto image_count = capabilities.min_image_count + 1;

        if (capabilities.max_image_count && image_count > capabilities.max_image_count.value()) {
            image_count = capabilities.max_image_count.value();
        }

        return mff::vulkan::Swapchain::build(
            device,
            surface,
            image_count,
            surface_format.format,
            extent,
            1,
            capabilities.supported_usage_flags | vk::ImageUsageFlags(vk::ImageUsageFlagBits::eColorAttachment),
            mff::vulkan::get_sharing_mode(queue_infos),
            capabilities.current_transform,
            vk::CompositeAlphaFlagBitsKHR::eOpaque,
            present_mode,
            true,
            surface_format.colorSpace
        );
    };

    LEAF_AUTO(swapchain_result, build_swapchain());
    auto swapchain = std::move(swapchain_result);

    LEAF_AUTO(
        render_pass,
        mff::vulkan::RenderPassBuilder()
            .add_attachment(
                "color",
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
                swapchain->get_format(),
                vk::SampleCountFlagBits::e1,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::ePresentSrcKHR
            )
            .add_pass({"color"})
            .build(device)
    );

    //instance.get_physical_devices()[0].

    event_loop.run(
        [&](auto event) {
            if (auto window_event = std::get_if<mff::window::events::WindowEvent>(&event)) {
                if (std::holds_alternative<mff::window::events::window::CloseRequested>(window_event->event)) {
                    logger::main->info("Quitting application");
                    return mff::window::ExecutionControl::Terminate;
                }
            }

            return mff::window::ExecutionControl::Wait;
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
*/
