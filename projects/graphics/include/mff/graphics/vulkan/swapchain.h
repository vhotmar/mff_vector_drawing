#pragma once

#include <optional>
#include <string>
#include <vector>

#include <mff/leaf.h>
#include <mff/graphics/math.h>
#include <mff/graphics/vulkan/device.h>
#include <mff/graphics/vulkan/instance.h>
#include <mff/graphics/vulkan/image/image.h>
#include <mff/graphics/vulkan/sync.h>
#include <mff/graphics/vulkan/vulkan.h>
#include <mff/graphics/window/window.h>

#include "./sync.h"

namespace mff::vulkan {

class Swapchain;
class SwapchainImage;
class UnsafeImage;
using UniqueSwapchain = std::unique_ptr<Swapchain>;
using UniqueSwapchainImage = std::unique_ptr<SwapchainImage>;
using UniqueUnsafeImage = std::unique_ptr<UnsafeImage>;

/**
 * The surface capabilities regarding PhysicalDevice, Window and platform
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap32.html#_surface_capabilities
 */
class Capabilities {
public:
    /**
     * Minimum number of images that swapchain supports
     */
    std::uint32_t min_image_count;

    /**
     * Maximum number of images that swapchain supports, if it is std::nullopt, than you can
     * specify any number of images (but you are still limited by memory)
     */
    std::optional<std::uint32_t> max_image_count;

    /**
     * Current extent (width and height) of the surface. If none will be determined by swapchain size
     */
    std::optional<Vector2ui> current_extent;

    /**
     * Contains the smallest valid swapchain extent (size) for the surface on the specified device.
     */
    Vector2ui min_image_extent;

    /**
     * Contains the largest valid swapchain extent (size) for the surface on the specified device.
     */
    Vector2ui max_image_extent;

    /**
     * // TODO: layers
     * Maximum number of layers presentable images.
     */
    std::uint32_t max_image_array_layers;

    /**
     * Presentation transforms supported for this device
     */
    vk::SurfaceTransformFlagsKHR supported_transforms;

    /**
     * Current transform used by the surface
     */
    vk::SurfaceTransformFlagBitsKHR current_transform;

    /**
     * Alpha compositing modes supported by the surface
     */
    vk::CompositeAlphaFlagsKHR supported_composite_alpha;

    /**
     * Image usages that are supported for images of the swapchain. Only
     * `vk::ImageUsageFlagBits::eColorAttachment` will be present everytime
     */
    vk::ImageUsageFlags supported_usage_flags;

    /**
     * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap32.html#_surface_format_support
     * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap32.html#VkSurfaceFormatKHR
     */
    std::vector<vk::SurfaceFormatKHR> supported_formats;

    /**
     * List of supported present modes. (FIFO will be always supported)
     * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap32.html#_surface_presentation_mode_support
     * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap32.html#VkPresentModeKHR
     */
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

    static boost::leaf::result<Capabilities> from_raw(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface);

    struct FindFormatOption {
        vk::Format format;
        vk::ColorSpaceKHR color_space;
    };

    std::optional<vk::SurfaceFormatKHR> find_format(
        const std::vector<FindFormatOption>& possibilities,
        bool or_default = true
    ) const;

    std::optional<vk::PresentModeKHR> find_present_mode(
        const std::vector<vk::PresentModeKHR>& possibilities,
        bool or_default = true
    ) const;

    mff::Vector2ui normalize_extent(mff::Vector2ui actual_extent, bool prefer_current = true) const;
};

class Surface;

using UniqueSurface = std::unique_ptr<Surface>;

/**
 * Represents surface on screen (built by Window)
 */
class Surface {
private:
    const Instance* instance_;
    vk::UniqueSurfaceKHR handle_;

    Surface() = default;

public:
    /**
     * @return the vulkan handle
     */
    vk::SurfaceKHR get_handle() const;

    /**
     * Is the specified QueueFamily supported by this surface?
     * @param queue_family QueueFamily which will be checked
     * @return support
     */
    bool is_supported(const QueueFamily* queue_family) const;

    /**
     * @param physical_device PhysicalDevice against we will try to find capabilities
     * @return Capabilities
     */
    boost::leaf::result<Capabilities> get_capabilities(const PhysicalDevice* physical_device) const;

    /**
     * Try to create the Surface from Window and instance
     * @param window
     * @param instance
     * @return
     */
    static boost::leaf::result<UniqueSurface> build(
        const std::shared_ptr<window::Window>& window,
        const Instance* instance
    );
};

enum class create_swapchain_error_code {
    /**
     * Provided invalid images count
     */
    unsupported_min_images_count_error,

    /**
     * Provided invalid images count
     */
    unsupported_max_images_count_error,

    /**
     * Provided unsupported images
     */
    unsupported_format,

    /**
     * Provided invalid image layers count
     */
    unsupported_array_layers,

    /**
     * Provided unsupported dimensions
     */
    unsupported_dimensions,

    /**
     * Provided invalid image usage flags
     */
    unsupported_usage_flags,

    /**
     * KHR extension was not loaded for device
     */
    missing_extension_khr_swapchain,

    /**
     * You have not specified dimensions and they are not already set.
     */
    unspecified_dimensions,

    /**
     * Specified surface transform is not supported
     */
    unsupported_surface_transform,

    /**
     * Specified composite alpha is not supported
     */
    unsupported_composite_alpha,

    /**
     * Unsupported present mode
     */
    unsupported_present_mode
};

/**
 * Contains the swapping system of images (WSI specific that are going to be presented to surface.
 * Basically this is the connection between graphics and screen.
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-khr-extensions/html/chap29.html#_wsi_swapchain
 */
class Swapchain {
public:
    /**
     * @return the actual vulkan swapchain
     */
    vk::SwapchainKHR get_handle() const;

    /**
     * @return the format with which was swapchain created
     */
    vk::Format get_format() const;

    /**
     * Builds a new swapchain and corresponding swapchain images (and their views)
     * @param device Device on which will be swapchain created
     * @param surface Surface onto which will swapchain present images
     * @param num_images Minimum number of images application needs (or the swapchain creation
     *                   will fail
     * @param format Format with which will be the swapchain images created with
     * @param dimensions Size of swapchain images
     * @param layers Number of layers in images
     * @param usage Intended usage of swapchain images
     * @param sharing Sharing mode for swapchain images
     * @param transform The transform used before presenting image
     * @param alpha Alpha compositing mode to use when presenting image
     * @param mode Presenting mode (how are image displayed on screen)
     * @param clipped Discard rendering operations which are not visible
     * @param color_space Color space (how swapchain interprets image data)
     * @param old_swapchain The old swapchain (may be helpful when reusing resources
     * @return created Swapchain object and image views
     */
    static boost::leaf::result<std::tuple<UniqueSwapchain, std::vector<UniqueSwapchainImage>>> build(
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
        std::optional<const Swapchain*> old_swapchain = std::nullopt
    );

private:
    Swapchain() = default;
    vk::UniqueSwapchainKHR handle_;
    const Device* device_;
    const Surface* surface_;
    vk::Format format_;

    std::vector<UniqueUnsafeImage> image_handles_;
};

}

namespace boost::leaf {

template <>
struct is_e_type<mff::vulkan::create_swapchain_error_code> : public std::true_type {};

}
