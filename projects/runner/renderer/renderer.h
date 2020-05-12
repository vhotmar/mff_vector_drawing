#pragma once

#include <memory>

#include <mff/leaf.h>

#include "./renderer_context.h"
#include "./renderer_surface.h"

/**
 * Renderer is class which takes RendererSurface and graphics queue on which to execute commands
 * and present you with commands to do simple rendering
 *
 * The rendering is done using with provided vertices, indices and push constants (color, transform)
 */
class Renderer {
public:
    /**
     * Render triangles with provided vertices, indices and push_constants
     * @param vertexes
     * @param indices
     * @param push_constants
     * @return
     */
    boost::leaf::result<void> draw(
        const std::vector<Vertex>& vertexes, const std::vector<std::uint32_t>& indices, PushConstants push_constants
    );

    /**
     * Build the renderer
     * @param surface surface on which to render
     * @param graphics_queue queue to use for rendering
     * @return
     */
    static boost::leaf::result<std::unique_ptr<Renderer>> build(
        RendererSurface* surface,
        mff::vulkan::SharedQueue graphics_queue
    );

    /**
     * Get the inner context provided by RendererScreen
     * @return
     */
    RendererContext* get_context();

private:
    Renderer() = default;

    /**
     * Helper function to create buffer
     * @param size the size of requested buffer
     * @param usage how is the buffer going to be used
     * @return
     */
    boost::leaf::result<vma::UniqueBuffer> create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage);

    /**
     * Request the vertex buffer to be sized at_least of required_size
     * @param required_size
     * @return
     */
    boost::leaf::result<void> request_vertex_buffer(vk::DeviceSize required_size);

    /**
     * Request the index buffer to be sized at_least of required_size
     * @param required_size
     * @return
     */
    boost::leaf::result<void> request_index_buffer(vk::DeviceSize required_size);

    RendererSurface* surface_;

    vma::UniqueBuffer vertex_buffer_;
    vma::UniqueBuffer index_buffer_;

    mff::vulkan::UniqueCommandPoolAllocation command_buffer_alloc_;
    mff::vulkan::UniqueFence fence_;

    mff::vulkan::SharedQueue graphics_queue_;
};