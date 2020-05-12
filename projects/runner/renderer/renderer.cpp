#include "./renderer.h"

boost::leaf::result<void> Renderer::draw(
    const std::vector<Vertex>& vertexes, const std::vector<std::uint32_t>& indices, PushConstants push_constants
) {
    // request buffers to be of size
    request_vertex_buffer(vertexes.size() * sizeof(Vertex));
    request_index_buffer(indices.size() * sizeof(std::uint32_t));

    // copy vertices and indices to corresponding buffers on GPU
    memcpy(vertex_buffer_->get_allocation_info().pMappedData, vertexes.data(), vertexes.size() * sizeof(Vertex));
    memcpy(
        index_buffer_->get_allocation_info().pMappedData,
        indices.data(),
        indices.size() * sizeof(std::uint32_t));

    // get a command buffer and reset it
    vk::CommandBuffer buffer = command_buffer_alloc_->get_handle();
    LEAF_CHECK(mff::to_result(buffer.reset({})));
    LEAF_CHECK(mff::to_result(buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit))));

    std::vector<vk::ClearValue> clear_values = {
        vk::ClearValue(vk::ClearColorValue(std::array<std::uint32_t, 4>{0, 0, 0, 0})),
        vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0))
    };

    // start render oass
    buffer.beginRenderPass(
        vk::RenderPassBeginInfo(
            get_context()->get_renderpass()->get_handle(),
            surface_->get_framebuffer()->get_handle(),
            vk::Rect2D({0, 0}, {surface_->get_width(), surface_->get_height()}),
            clear_values.size(),
            clear_values.data()),
        vk::SubpassContents::eInline
    );

    // set viewport in framebuffer to which to render
    buffer.setViewport(0, {vk::Viewport(0, 0, surface_->get_width(), surface_->get_height(), 0, 1)});
    buffer.setScissor(
        0,
        {vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(surface_->get_width(), surface_->get_height()))}
    );

    // bind the buffers so they can be rendered
    auto vb = vertex_buffer_->get_buffer();
    buffer.bindVertexBuffers(0, {vb}, {0});
    auto ib = index_buffer_->get_buffer();
    buffer.bindIndexBuffer(ib, 0, vk::IndexType::eUint32);

    // update the push constants
    buffer.pushConstants(
        get_context()->get_pipeline_layout(),
        vk::ShaderStageFlagBits::eVertex,
        0,
        sizeof(PushConstants),
        &push_constants
    );

    // bind pipelines
    buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, get_context()->get_over_pipeline());
    buffer.setStencilCompareMask(vk::StencilFaceFlagBits::eFrontAndBack, kSTENCIL_CLIP_BIT);
    // draw the indices
    buffer.drawIndexed(indices.size(), 1, 0, 0, 0);
    buffer.endRenderPass();
    LEAF_CHECK(mff::to_result(buffer.end()));

    // submit the commands and wait for results
    // TODO: do this "asynchronously"
    vk::PipelineStageFlags wait_flag = vk::PipelineStageFlagBits::eAllCommands;
    vk::SubmitInfo submit_info(0, nullptr, &wait_flag, 1, &buffer, 0, nullptr);
    graphics_queue_->get_handle().submit({submit_info}, fence_->get_handle());

    get_context()->get_device()
        ->get_handle()
        .waitForFences({fence_->get_handle()}, true, std::numeric_limits<std::uint64_t>::max());
    get_context()->get_device()->get_handle().resetFences({fence_->get_handle()});

    return {};
}

boost::leaf::result<std::unique_ptr<Renderer>> Renderer::build(
    RendererSurface* surface,
    mff::vulkan::SharedQueue graphics_queue
) {
    logger::main->debug("Building Renderer");
    struct enable_Renderer : public Renderer {};
    std::unique_ptr<Renderer> result = std::make_unique<enable_Renderer>();

    result->surface_ = surface;
    result->graphics_queue_ = graphics_queue;

    auto device = surface->get_context()->get_device();

    // prepare buffers, fence and command buffers
    LEAF_CHECK(result->request_vertex_buffer(1024));
    LEAF_CHECK(result->request_index_buffer(1024));
    LEAF_AUTO_TO(result->fence_, mff::vulkan::Fence::build(device, false)); // TODO: use pool
    LEAF_AUTO(pool, device->get_command_pool(graphics_queue->get_queue_family()));
    LEAF_AUTO(cmd_buffs, pool->allocate(1, false));
    result->command_buffer_alloc_ = std::move(cmd_buffs[0]);

    return result;
}

RendererContext* Renderer::get_context() {
    return surface_->get_context();
}

boost::leaf::result<vma::UniqueBuffer> Renderer::create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage) {
    auto buffer_info = vk::BufferCreateInfo(
        {},
        size,
        usage,
        vk::SharingMode::eExclusive
    );

    VmaAllocationCreateInfo allocation_info = {};
    // TODO: consider using some other type of memory
    // but most probably this is the best we can get, because everytime we upload it with new data
    allocation_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    allocation_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    return get_context()->get_device()->get_allocator()->create_buffer(buffer_info, allocation_info);
}

boost::leaf::result<void> Renderer::request_vertex_buffer(vk::DeviceSize required_size) {
    if (vertex_buffer_ != nullptr && vertex_buffer_->get_size() >= required_size) return {};

    logger::main->debug("Renderer requesting bigger vertex buffer of size {}", required_size);
    LEAF_AUTO_TO(vertex_buffer_, create_buffer(required_size, vk::BufferUsageFlagBits::eVertexBuffer));

    return {};
}


boost::leaf::result<void> Renderer::request_index_buffer(vk::DeviceSize required_size) {
    if (index_buffer_ != nullptr && index_buffer_->get_size() >= required_size) return {};

    logger::main->debug("Renderer requesting bigger index buffer of size {}", required_size);
    LEAF_AUTO_TO(index_buffer_, create_buffer(required_size, vk::BufferUsageFlagBits::eIndexBuffer));

    return {};
}