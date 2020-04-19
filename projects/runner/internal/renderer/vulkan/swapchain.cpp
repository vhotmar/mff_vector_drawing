#include "swapchain.h"

#include <utility>
#include <mff/algorithms.h>
#include <mff/utils.h>
#include "instance.h"

namespace mff::internal::renderer::vulkan {

/*boost::leaf::result<swapchain_support_details>
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
}*/

Surface::Surface(std::shared_ptr<Instance> instance)
    : instance_(instance) {
}

bool Surface::is_supported(const QueueFamily& queue_family) const {
    auto physical_device = queue_family.get_physical_device()->get_handle();

    LEAF_DEFAULT(
        result,
        false,
        to_result(physical_device.getSurfaceSupportKHR(queue_family.get_index(), handle_.get())));

    return result;
}

boost::leaf::result<std::shared_ptr<Surface>> Surface::build(
    std::shared_ptr<window::Window> window,
    std::shared_ptr<Instance> instance
) {
    struct enable_Surface: public Surface {
        enable_Surface(std::shared_ptr<Instance> inst): Surface(inst) {}
    };

    std::shared_ptr<Surface> surface = std::make_shared<enable_Surface>(instance);

    LEAF_AUTO_TO(surface->handle_, window->create_surface(instance->get_handle()));

    return surface;
}

boost::leaf::result<Capabilities> Surface::get_capabilities(const std::shared_ptr<PhysicalDevice>& physical_device) const {
    auto device_handle = physical_device->get_handle();

    LEAF_AUTO(capabilities, to_result(device_handle.getSurfaceCapabilitiesKHR(handle_.get())));
    LEAF_AUTO(formats, to_result(device_handle.getSurfaceFormatsKHR(handle_.get())));
    LEAF_AUTO(present_modes, to_result(device_handle.getSurfacePresentModesKHR(handle_.get())));

    return Capabilities(
        capabilities.minImageCount,
        capabilities.maxImageCount == 0 ? std::nullopt : std::make_optional(capabilities.maxImageCount),
        capabilities.currentExtent.width == UINT32_MAX && capabilities.currentExtent.height == UINT32_MAX
        ? std::nullopt
        : std::make_optional(Vector2ui(capabilities.currentExtent.width, capabilities.currentExtent.height)),
        Vector2ui(capabilities.minImageExtent.width, capabilities.minImageExtent.height),
        Vector2ui(capabilities.maxImageExtent.width, capabilities.maxImageExtent.height),
        capabilities.maxImageArrayLayers,
        capabilities.supportedTransforms,
        capabilities.currentTransform,
        capabilities.supportedCompositeAlpha,
        capabilities.supportedUsageFlags,
        formats,
        present_modes
    );
}

vk::SurfaceKHR Surface::get_handle() const {
    return handle_.get();
}

Swapchain::Swapchain(
    std::shared_ptr<Device> device,
    std::shared_ptr<Surface> surface
)
    : device_(device), surface_(surface) {
}

vk::SwapchainKHR Swapchain::get_handle() const {
    return handle_.get();
}

boost::leaf::result<Swapchain> Swapchain::build(
    std::shared_ptr<Device> device,
    std::shared_ptr<Surface> surface,
    std::uint32_t num_images,
    vk::Format format,
    std::optional<Vector2ui> dimensions,
    std::uint32_t layers,
    vk::ImageUsageFlags usage,
    SharingMode sharing,
    vk::SurfaceTransformFlagBitsKHR transform,
    vk::CompositeAlphaFlagBitsKHR alpha,
    vk::PresentModeKHR mode,
    bool clipped,
    vk::ColorSpaceKHR color_space,
    std::optional<Swapchain> old_swapchain
) {
    LEAF_AUTO(capabilities, surface->get_capabilities(device->get_physical_device()));

    if (num_images < capabilities.min_image_count)
        return boost::leaf::new_error(create_swapchain_error_code::unsupported_min_images_count_error);
    if (capabilities.max_image_count && (num_images > capabilities.max_image_count.value()))
        return boost::leaf::new_error(create_swapchain_error_code::unsupported_max_images_count_error);

    if (!mff::any_of(
        capabilities.supported_formats,
        [&](auto item) { return item.format == format && item.colorSpace == color_space; }
    )
        ) {
        return boost::leaf::new_error(create_swapchain_error_code::unsupported_format);
    }

    Vector2ui dimensions_to_use;

    if (dimensions) {
        auto val = dimensions.value();

        if (val[0] < capabilities.min_image_extent[0] || val[1] < capabilities.min_image_extent[1]) {
            return boost::leaf::new_error(create_swapchain_error_code::unsupported_dimensions);
        }

        if (val[0] > capabilities.max_image_extent[0] || val[1] > capabilities.max_image_extent[1]) {
            return boost::leaf::new_error(create_swapchain_error_code::unsupported_dimensions);
        }

        dimensions_to_use = val;
    } else {
        dimensions_to_use = capabilities.current_extent.value(); // TODO: should use something like LEAF_OPTIONAL_TO
    }

    if (layers < 1 || layers > capabilities.max_image_array_layers) {
        return boost::leaf::new_error(create_swapchain_error_code::unsupported_array_layers);
    }

    if ((usage & capabilities.supported_usage_flags) != usage) {
        return boost::leaf::new_error(create_swapchain_error_code::unsupported_usage_flags);
    }

    if (!mff::contains(device->get_extensions(), VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
        return boost::leaf::new_error(create_swapchain_error_code::missing_extension_khr_swapchin);
    }

    using sh_info = std::tuple<vk::SharingMode, std::uint32_t, std::uint32_t*>;
    auto[sh_mode, sh_count, sh_indices] = std::visit(
        overloaded{
            [](ExclusiveSharingMode e) -> sh_info { return std::make_tuple(vk::SharingMode::eExclusive, 0, nullptr); },
            [](ConcurrentSharingMode c) -> sh_info {
                return std::make_tuple(
                    vk::SharingMode::eConcurrent,
                    c.queue_families.size(),
                    c.queue_families.data());
            },
        }, sharing
    );

    vk::SwapchainCreateInfoKHR swapchain_create_info(
        {},
        surface->get_handle(),
        num_images,
        format,
        color_space,
        vk::Extent2D(dimensions_to_use[0], dimensions_to_use[1]),
        layers,
        usage,
        sh_mode,
        sh_count,
        sh_indices,
        transform,
        alpha,
        mode,
        clipped,
        old_swapchain.has_value() ? old_swapchain.value().get_handle() : nullptr
    );

    Swapchain swapchain(device, surface);

    auto device_handle = device->get_handle();

    LEAF_AUTO_TO(
        swapchain.handle_,
        to_result(device_handle.createSwapchainKHRUnique(swapchain_create_info)));

    LEAF_AUTO(image_handles, to_result(device_handle.getSwapchainImagesKHR(swapchain.handle_.get())));


    return swapchain;
}

Capabilities::Capabilities(
    std::uint32_t min_image_count,
    std::optional<std::uint32_t> max_image_count,
    std::optional<Vector2ui> current_extent,
    Vector2ui min_image_extent,
    Vector2ui max_image_extent,
    std::uint32_t max_image_array_layers,
    vk::SurfaceTransformFlagsKHR supported_transforms,
    vk::SurfaceTransformFlagBitsKHR current_transform,
    vk::CompositeAlphaFlagsKHR supported_composite_alpha,
    vk::ImageUsageFlags supported_usage_flags,
    std::vector<vk::SurfaceFormatKHR> supported_formats,
    std::vector<vk::PresentModeKHR> present_modes
)
    : min_image_count(min_image_count)
    , max_image_count(max_image_count)
    , current_extent(std::move(current_extent))
    , min_image_extent(std::move(min_image_extent))
    , max_image_extent(std::move(max_image_extent))
    , max_image_array_layers(max_image_array_layers)
    , supported_transforms(supported_transforms)
    , current_transform(current_transform)
    , supported_composite_alpha(supported_composite_alpha)
    , supported_usage_flags(supported_usage_flags)
    , supported_formats(std::move(supported_formats))
    , present_modes(present_modes) {

}
}
