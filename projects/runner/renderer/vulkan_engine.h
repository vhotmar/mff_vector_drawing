#pragma once

#include <optional>
#include <vector>

#include <mff/graphics/vulkan/device.h>
#include <mff/graphics/vulkan/instance.h>

//////////////////////
/// Queue families ///
//////////////////////

/**
 * Indicates which queue families are available and where
 * https://vulkan-tutorial.com/
 */
struct QueueFamilyIndices {
    std::optional<const mff::vulkan::QueueFamily*> graphics_family = std::nullopt;
    std::optional<const mff::vulkan::QueueFamily*> present_family = std::nullopt;
    std::optional<const mff::vulkan::QueueFamily*> transfer_family = std::nullopt;
    std::optional<const mff::vulkan::QueueFamily*> compute_family = std::nullopt;

    /**
     * Check whether all families are present
     * @return
     */
    bool is_complete();
    std::vector<const mff::vulkan::QueueFamily*> to_vector() const;
};

/**
 * Collection of all queues
 */
struct Queues {
    mff::vulkan::SharedQueue graphics_queue = nullptr;
    mff::vulkan::SharedQueue present_queue = nullptr;
    mff::vulkan::SharedQueue transfer_queue = nullptr;
    mff::vulkan::SharedQueue compute_queue = nullptr;

    static Queues from_vector(const std::vector<mff::vulkan::SharedQueue>& queues_vec);
};

/**
 * This class provides us with resources which are not specific to our vector graphics renderer
 * - Vulkan instance
 * - Vulkan surface
 * - Vulkan device (+ allocator)
 * - Vulkan queues
 */
class VulkanEngine {
public:
    /**
     * Build all specified Vulkan handles
     * @param window
     * @return
     */
    static boost::leaf::result<std::unique_ptr<VulkanEngine>> build(
        const std::shared_ptr<mff::window::Window>& window
    );

    /**
     * Get Vulkan Instance
     * @return
     */
    const mff::vulkan::Instance* get_instance() const;

    /**
     * Get Vulkan Surface (corresponding to window specified in build function)
     * @return
     */
    const mff::vulkan::Surface* get_surface() const;

    /**
     * Get the used Vulkan Device
     * @return
     */
    mff::vulkan::Device* get_device();

    /**
     * Get the used Vulkan Device (const version)
     * @return
     */
    const mff::vulkan::Device* get_device() const;

    /**
     * Get Allocator
     * @return
     */
    const vma::Allocator* get_allocator() const;

    /**
     * Get used window
     * @return
     */
    const std::shared_ptr<mff::window::Window>& get_window() const;

    /**
     * Get all queues provided by Vulkan Device
     * @return
     */
    const Queues& get_queues() const;

private:
    VulkanEngine() = default;

    /**
     * Window on which we will initialize the vulkan context + draw (no multi window for now)
     */
    std::shared_ptr<mff::window::Window> window_ = nullptr;

    mff::vulkan::UniqueInstance instance_ = nullptr;
    mff::vulkan::UniqueSurface surface_ = nullptr;
    const mff::vulkan::PhysicalDevice* physical_device_ = nullptr;
    mff::vulkan::UniqueDevice device_ = nullptr;

    Queues queues_ = {};

    std::vector<std::uint32_t> queue_family_indices_;
};