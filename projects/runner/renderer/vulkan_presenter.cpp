#include "./vulkan_presenter.h"

boost::leaf::result<std::unique_ptr<VulkanPresenter>> VulkanPresenter::build(VulkanEngine* engine) {
    logger::main->debug("Building VulkanPresenter");
    struct enable_VulkanPresenter : public VulkanPresenter {};
    std::unique_ptr<VulkanPresenter> result = std::make_unique<enable_VulkanPresenter>();

    result->window_ = engine->get_window();
    result->present_queue_ = engine->get_queues().present_queue;
    result->device_ = engine->get_device();
    result->surface_ = engine->get_surface();

    // allocate semaphores used for drawing
    LEAF_AUTO_TO(result->present_end_semaphore_, mff::vulkan::Semaphore::build(engine->get_device()));
    LEAF_AUTO_TO(result->draw_end_semaphore_, mff::vulkan::Semaphore::build(engine->get_device()));

    result->build_swapchain();

    return result;
}

const mff::vulkan::Swapchain* VulkanPresenter::get_swapchain() const {
    return swapchain_.get();
}

mff::Vector2ui VulkanPresenter::get_dimensions() const {
    return swapchain_->get_dimensions();
}

boost::leaf::result<void> VulkanPresenter::build_commands(
    const mff::vulkan::Image* source,
    mff::Vector2ui dimensions
) {
    std::size_t index = 0;
    // we need to do that for all command buffers (for all swapchain screens)
    for (const auto& alloc: command_buffer_allocations_) {
        // for this we can use the incomplete Vulkan Builder
        LEAF_AUTO(builder, mff::vulkan::UnsafeCommandBufferBuilder::from_buffer(alloc->get_handle(), {}));

        auto destination = swapchain_images_[index]->get_image_impl();

        // and what we need to do is to convert the images to correct formats, copy the image
        // and transform the images to original formats

        // TODO: should be automated using SyncCommandBufferBuilder
        builder->pipeline_barrier(
            mff::vulkan::UnsafeCommandBufferBuilderPipelineBarrier()
                .add_image_memory_barrier(
                    destination,
                    0,
                    1,
                    0,
                    1,
                    vk::PipelineStageFlagBits::eBottomOfPipe,
                    {},
                    vk::PipelineStageFlagBits::eTransfer,
                    vk::AccessFlagBits::eTransferWrite,
                    true,
                    std::nullopt,
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eTransferDstOptimal
                )
                .add_image_memory_barrier(
                    source,
                    0,
                    1,
                    0,
                    1,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput,
                    vk::AccessFlagBits::eColorAttachmentWrite,
                    vk::PipelineStageFlagBits::eTransfer,
                    vk::AccessFlagBits::eTransferRead,
                    true,
                    std::nullopt,
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::ImageLayout::eTransferSrcOptimal
                ));

        builder->copy_image(
            source,
            vk::ImageLayout::eTransferSrcOptimal,
            destination,
            vk::ImageLayout::eTransferDstOptimal,
            {mff::vulkan::UnsafeCommandBufferBuilderImageCopy{
                {true, false, false},
                0,
                1,
                0,
                1,
                1,
                {0, 0, 0},
                {0, 0, 0},
                {dimensions[0], dimensions[1], 1}}} // TODO: check dimensions
        );

        builder->pipeline_barrier(
            mff::vulkan::UnsafeCommandBufferBuilderPipelineBarrier()
                .add_image_memory_barrier(
                    destination,
                    0,
                    1,
                    0,
                    1,
                    vk::PipelineStageFlagBits::eTransfer,
                    vk::AccessFlagBits::eTransferWrite,
                    vk::PipelineStageFlagBits::eBottomOfPipe,
                    {},
                    true,
                    std::nullopt,
                    vk::ImageLayout::eTransferDstOptimal,
                    vk::ImageLayout::ePresentSrcKHR
                )
                .add_image_memory_barrier(
                    source,
                    0,
                    1,
                    0,
                    1,
                    vk::PipelineStageFlagBits::eTransfer,
                    {},
                    vk::PipelineStageFlagBits::eColorAttachmentOutput,
                    vk::AccessFlagBits::eColorAttachmentWrite,
                    true,
                    std::nullopt,
                    vk::ImageLayout::eTransferSrcOptimal,
                    vk::ImageLayout::eColorAttachmentOptimal
                ));

        builder->build();
        index++;
    }

    return {};
}

boost::leaf::result<bool> VulkanPresenter::draw() {
    // acquire the swapchain image
    LEAF_AUTO(acquire_result, swapchain_->acquire_next_image_raw(present_end_semaphore_.get(), std::nullopt));
    auto[index, optimal] = acquire_result;

    // if not optimal recreate
    if (!optimal) {
        LEAF_CHECK(build_swapchain());

        return false;
    }

    auto swapchain = swapchain_->get_handle();
    auto present_end = present_end_semaphore_->get_handle();
    auto draw_end = draw_end_semaphore_->get_handle();
    auto buffer = command_buffer_allocations_[index]->get_handle();
    vk::PipelineStageFlags flag = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    // submit built command buffers
    LEAF_CHECK(
        mff::to_result(
            present_queue_->get_handle()
                .submit(
                    {vk::SubmitInfo(
                        1,
                        &present_end,
                        &flag,
                        1,
                        &buffer,
                        1,
                        &draw_end
                    )},
                    {}
                )));

    LEAF_CHECK(mff::to_result(
        present_queue_->get_handle()
            .presentKHR(vk::PresentInfoKHR(1, &draw_end, 1, &swapchain, &index))));

    return true;
}

boost::leaf::result<void> VulkanPresenter::build_swapchain() {
    logger::main->debug("Building VulkanPresenter swapchain");
    LEAF_AUTO(capabilities, surface_->get_capabilities(device_->get_physical_device()));

    LEAF_CHECK_OPTIONAL(
        surface_format,
        capabilities.find_format({{vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}}));
    LEAF_CHECK_OPTIONAL(
        present_mode,
        capabilities.find_present_mode({vk::PresentModeKHR::eMailbox}));
    auto extent = capabilities.normalize_extent(window_->get_inner_size());

    auto image_count = capabilities.min_image_count + 1;

    if (capabilities.max_image_count && image_count > capabilities.max_image_count.value()) {
        image_count = capabilities.max_image_count.value();
    }

    LEAF_AUTO(
        swapchain_result,
        mff::vulkan::Swapchain::build(
            device_,
            surface_,
            image_count,
            surface_format.format,
            extent,
            1,
            capabilities.supported_usage_flags | vk::ImageUsageFlags(vk::ImageUsageFlagBits::eColorAttachment),
            mff::vulkan::SharingMode_::Exclusive{}, // we will use only present queue with swapchain
            capabilities.current_transform,
            vk::CompositeAlphaFlagBitsKHR::eOpaque,
            present_mode,
            true,
            surface_format.colorSpace,
            swapchain_ ? std::make_optional(swapchain_.get()) : std::nullopt
        ));
    std::tie(swapchain_, swapchain_images_) = std::move(swapchain_result);

    LEAF_AUTO(command_pool, device_->get_command_pool(present_queue_->get_queue_family()));
    LEAF_AUTO_TO(command_buffer_allocations_, command_pool->allocate(swapchain_images_.size()));

    return {};
}

vk::Format VulkanPresenter::get_format() const {
    return swapchain_->get_format();
}
