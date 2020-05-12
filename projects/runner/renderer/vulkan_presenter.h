#pragma once

#include <memory>

#include <mff/leaf.h>
#include <mff/graphics/vulkan/command_buffer/builders/unsafe.h>

#include "./vulkan_engine.h"
#include "../utils/logger.h"

/**
 * Most of the time we do not really want to deal with drawing to swapchain (or in this case
 * specifically) So we will create an basic abstraction.
 *
 * It will provide you with two basic methods:
 * - build_commands (which should be used to indicate which image should be presented on screen)
 * - draw (drawing command)
 */
class VulkanPresenter {
public:
    /**
     * Build the presenter using VulkanEngine (which contains screen to present to + all other
     * needed Vulkan handles
     * @param engine
     * @return
     */
    static boost::leaf::result<std::unique_ptr<VulkanPresenter>> build(VulkanEngine* engine);

    /**
     * Get the built swapchain
     * @return
     */
    const mff::vulkan::Swapchain* get_swapchain() const;

    /**
     * Get presenter dimensions
     * @return
     */
    mff::Vector2ui get_dimensions() const;

    /**
     * Get format used by presenter
     * @return
     */
    vk::Format get_format() const;

    /**
     * Set new image source for the presenter to present
     * @param source image to present
     * @param dimensions dimensions of image to use
     * @return
     */
    boost::leaf::result<void> build_commands(
        const mff::vulkan::Image* source,
        mff::Vector2ui dimensions
    );

    /**
     * Copy resources from specified image to screen
     * @return is current swapchain optimal?
     */
    boost::leaf::result<bool> draw();

private:
    VulkanPresenter() = default;

    boost::leaf::result<void> build_swapchain();

    std::shared_ptr<mff::window::Window> window_ = nullptr;
    mff::vulkan::SharedQueue present_queue_ = nullptr;
    mff::vulkan::Device* device_ = nullptr;
    const mff::vulkan::Surface* surface_ = nullptr;
    mff::vulkan::UniqueSemaphore present_end_semaphore_ = nullptr;
    mff::vulkan::UniqueSemaphore draw_end_semaphore_ = nullptr;
    mff::vulkan::UniqueSwapchain swapchain_ = nullptr;
    std::vector<mff::vulkan::UniqueSwapchainImage> swapchain_images_ = {};
    std::vector<mff::vulkan::UniqueCommandPoolAllocation> command_buffer_allocations_ = {};
};