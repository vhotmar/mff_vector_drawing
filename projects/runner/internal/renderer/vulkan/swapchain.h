#pragma once

#include <string>
#include <vector>

#include "../../eigen.h"
#include "../../leaf.h"
#include "../../vulkan.h"
#include "./instance.h"
#include "./device.h"
#include "../../window/window.h"
#include "./sync.h"

namespace mff::internal::renderer::vulkan {

class Capabilities {
public:
    std::uint32_t min_image_count;
    std::optional<std::uint32_t> max_image_count;
    std::optional<Vector2ui> current_extent;
    Vector2ui min_image_extent;
    Vector2ui max_image_extent;
    std::uint32_t max_image_array_layers;
    vk::SurfaceTransformFlagsKHR supported_transforms;
    vk::SurfaceTransformFlagBitsKHR current_transform;
    vk::CompositeAlphaFlagsKHR supported_composite_alpha;
    vk::ImageUsageFlags supported_usage_flags;
    std::vector<vk::SurfaceFormatKHR> supported_formats;
    std::vector<vk::PresentModeKHR> present_modes;

    Capabilities(
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
    );
};

class Surface {
private:
    std::shared_ptr<Instance> instance_;
    vk::UniqueSurfaceKHR handle_;

    Surface(std::shared_ptr<Instance> instance);

public:
    vk::SurfaceKHR get_handle() const;

    bool is_supported(const QueueFamily& queue_family) const;

    boost::leaf::result<Capabilities> get_capabilities(const std::shared_ptr<PhysicalDevice>& physical_device) const;

    static boost::leaf::result<std::shared_ptr<Surface>> build(
        std::shared_ptr<window::Window> window,
        std::shared_ptr<Instance> instance
    );
};

enum class create_swapchain_error_code {
    unsupported_min_images_count_error,
    unsupported_max_images_count_error,
    unsupported_format,
    unsupported_array_layers,
    unsupported_dimensions,
    unsupported_usage_flags,
    missing_extension_khr_swapchin
};

class Swapchain {
private:
    vk::UniqueSwapchainKHR handle_;
    std::shared_ptr<Device> device_;
    std::shared_ptr<Surface> surface_;

public:
    Swapchain(
        std::shared_ptr<Device> device,
        std::shared_ptr<Surface> surface
    );

    vk::SwapchainKHR get_handle() const;

    static boost::leaf::result<Swapchain> build(
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
        std::optional<Swapchain> old_swapchain = std::nullopt
    );
};

}

namespace boost::leaf {

template <>
struct is_e_type<mff::internal::renderer::vulkan::create_swapchain_error_code> : public std::true_type {};

}
