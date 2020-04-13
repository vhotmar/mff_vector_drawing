#include "swapchain.h"
#include "queue_family.h"

namespace mff::internal::renderer::vulkan {

boost::leaf::result<swapchain_support_details>
query_swapchain_support(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    swapchain_support_details details;

    LEAF_AUTO_TO(details.capabilities, to_result(device.getSurfaceCapabilitiesKHR(surface)));
    LEAF_AUTO_TO(details.formats, to_result(device.getSurfaceFormatsKHR(surface)));
    LEAF_AUTO_TO(details.present_modes, to_result(device.getSurfacePresentModesKHR(surface)));

    return details;
}

vk::SurfaceFormatKHR choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats) {
    auto is_suitable = [](vk::SurfaceFormatKHR format) -> bool {
        return format.format == vk::Format::eB8G8R8A8Srgb
            && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    };

    for (const auto& available_format : available_formats) {
        if (is_suitable(available_format)) {
            return available_format;
        }
    }

    return available_formats[0];
}

vk::PresentModeKHR choose_swap_present_mode(const std::vector<vk::PresentModeKHR>& available_present_modes) {
    for (const auto& available_present_mode : available_present_modes) {
        if (available_present_mode == vk::PresentModeKHR::eMailbox) {
            return available_present_mode;
        }
    }

    return available_present_modes[0];
}

vk::Extent2D choose_swap_extent(const vk::SurfaceCapabilitiesKHR& capabilities, vk::Extent2D actual_extent) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        vk::Extent2D result_extent(actual_extent);

        result_extent.width = std::max(
            capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, actual_extent.width));
        result_extent.height = std::max(
            capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, actual_extent.height));

        return result_extent;
    }
}

boost::leaf::result<void> create_swapchain(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface) {
    LEAF_AUTO(swapchain_support, query_swapchain_support(physical_device, surface));

    auto surface_format = choose_swap_surface_format(swapchain_support.formats);
    auto present_mode = choose_swap_present_mode(swapchain_support.present_modes);
    auto extent = choose_swap_extent(swapchain_support.capabilities, window_->framebuffer_extent());

    auto image_count = swapchain_support.capabilities.minImageCount + 1;

    if (swapchain_support.capabilities.maxImageCount > 0
        && image_count > swapchain_support.capabilities.maxImageCount) {
        image_count = swapchain_support.capabilities.maxImageCount;
    }

    LEAF_AUTO(indices, find_queue_families(physical_device, surface));
    std::vector<uint32_t> queue_family_indices = {*indices.graphics_family, *indices.present_family};
    bool different_queues = indices.present_family != indices.graphics_family;

    vk::SwapchainCreateInfoKHR swapchain_create_info(
        {},
        surface,
        image_count,
        surface_format.format,
        surface_format.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        different_queues ? vk::SharingMode::eConcurrent
                         : vk::SharingMode::eExclusive, // TODO: check how to use only Concurrent
        different_queues ? 2 : 0,
        different_queues ? queue_family_indices.data() : nullptr,
        swapchain_support.capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        present_mode,
        {},
        *swapchain_
    );

    swapchain_ = VK_TRY(device_->createSwapchainKHRUnique(swapchain_create_info));
    swapchain_images_ = VK_TRY(device_->getSwapchainImagesKHR(*swapchain_));
    swapchain_image_format_ = surface_format.format;
    swapchain_extent_ = extent;

    return {};
}

}
