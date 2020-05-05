#pragma once

#include <memory>

#include <mff/leaf.h>

#include "./renderer_context.h"
#include "./renderer_surface.h"

class Renderer {
public:
    boost::leaf::result<void> draw(
        const std::vector<Vertex>& vertexes, const std::vector<std::uint32_t>& indices, PushConstants push_constants
    );

    static boost::leaf::result<std::unique_ptr<Renderer>> build(
        RendererSurface* surface,
        mff::vulkan::SharedQueue graphics_queue
    );

    RendererContext* get_context();

private:
    Renderer() = default;

    boost::leaf::result<vma::UniqueBuffer> create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage);
    boost::leaf::result<void> request_vertex_buffer(vk::DeviceSize required_size);
    boost::leaf::result<void> request_index_buffer(vk::DeviceSize required_size);

    RendererSurface* surface_;

    vma::UniqueBuffer vertex_buffer_;
    vma::UniqueBuffer index_buffer_;

    mff::vulkan::UniqueCommandPoolAllocation command_buffer_alloc_;
    mff::vulkan::UniqueFence fence_;

    mff::vulkan::SharedQueue graphics_queue_;
};