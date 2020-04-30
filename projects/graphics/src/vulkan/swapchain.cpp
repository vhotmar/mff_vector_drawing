#include <mff/graphics/vulkan/swapchain.h>

#include <utility>

#include <mff/algorithms.h>
#include <mff/utils.h>

#include "../utils.h"

namespace mff::vulkan {

bool Surface::is_supported(const QueueFamily* queue_family) const {
    auto physical_device = queue_family->get_physical_device()->get_handle();

    LEAF_DEFAULT(
        result,
        false,
        to_result(physical_device.getSurfaceSupportKHR(queue_family->get_index(), handle_.get())));

    return result;
}

boost::leaf::result<std::unique_ptr<Surface>> Surface::build(
    std::shared_ptr<window::Window> window,
    const Instance* instance
) {
    struct enable_Surface : public Surface {};
    std::unique_ptr<Surface> surface = std::make_unique<enable_Surface>();

    surface->instance_ = instance;
    LEAF_AUTO_TO(surface->handle_, window->create_surface(instance->get_handle()));

    return surface;
}

boost::leaf::result<Capabilities> Surface::get_capabilities(
    const PhysicalDevice* physical_device
) const {
    auto device_handle = physical_device->get_handle();

    LEAF_AUTO(capabilities, to_result(device_handle.getSurfaceCapabilitiesKHR(handle_.get())));
    LEAF_AUTO(formats, to_result(device_handle.getSurfaceFormatsKHR(handle_.get())));
    LEAF_AUTO(present_modes, to_result(device_handle.getSurfacePresentModesKHR(handle_.get())));

    return Capabilities(
        capabilities.minImageCount,
        capabilities.maxImageCount == 0 ? std::nullopt : std::make_optional(capabilities.maxImageCount),
        capabilities.currentExtent.width == std::numeric_limits<std::uint32_t>::max()
            && capabilities.currentExtent.height == std::numeric_limits<std::uint32_t>::max()
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

vk::SwapchainKHR Swapchain::get_handle() const {
    return handle_.get();
}

boost::leaf::result<std::shared_ptr<Swapchain>> Swapchain::build(
    const Device* device,
    const Surface* surface,
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
    // TODO: enable chcking only in debug mode
    // at first we will nsure that the options given by user are correct
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
        if (!capabilities.current_extent.has_value()) {
            return boost::leaf::new_error(create_swapchain_error_code::unspecified_dimensions);
        }

        dimensions_to_use = capabilities.current_extent.value();
    }

    if (layers < 1 || layers > capabilities.max_image_array_layers) {
        return boost::leaf::new_error(create_swapchain_error_code::unsupported_array_layers);
    }

    if ((usage & capabilities.supported_usage_flags) != usage) {
        return boost::leaf::new_error(create_swapchain_error_code::unsupported_usage_flags);
    }

    if ((transform & capabilities.supported_transforms) != transform) {
        return boost::leaf::new_error(create_swapchain_error_code::unsupported_surface_transform);
    }

    if ((alpha & capabilities.supported_composite_alpha) != alpha) {
        return boost::leaf::new_error(create_swapchain_error_code::unsupported_composite_alpha);
    }

    if (!mff::contains(capabilities.present_modes, mode)) {
        return boost::leaf::new_error(create_swapchain_error_code::unsupported_present_mode);
    }

    if (!mff::contains(device->get_extensions(), VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
        return boost::leaf::new_error(create_swapchain_error_code::missing_extension_khr_swapchain);
    }

    using sh_info = std::tuple<vk::SharingMode, std::uint32_t, std::uint32_t*>;
    auto[sh_mode, sh_count, sh_indices] = std::visit(
        overloaded{
            [](SharingMode_::Exclusive e) -> sh_info {
                return std::make_tuple(
                    vk::SharingMode::eExclusive,
                    0,
                    nullptr
                );
            },
            [](SharingMode_::Concurrent c) -> sh_info {
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

    struct enable_Swapchain : public Swapchain {};
    std::shared_ptr<Swapchain> swapchain = std::make_shared<enable_Swapchain>();

    swapchain->device_ = device;
    swapchain->surface_ = surface;

    auto device_handle = device->get_handle();

    LEAF_AUTO_TO(
        swapchain->handle_,
        to_result(device_handle.createSwapchainKHRUnique(swapchain_create_info)));

    LEAF_AUTO(image_handles, to_result(device_handle.getSwapchainImagesKHR(swapchain->handle_.get())));
    swapchain->format_ = format;

    // TODO: create images

    return swapchain;
}

vk::Format Swapchain::get_format() const {
    return format_;
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
    , present_modes(std::move(present_modes)) {

}
}
