#pragma once

#include <array>
#include <memory>

#include <mff/leaf.h>
#include <mff/graphics/math.h>
#include <mff/graphics/vulkan/image/image.h>
#include <mff/graphics/vulkan/framebuffer/framebuffer.h>

#include "./renderer_context.h"

/**
 * RenderTarget for our renderer (and then it is going to be copied by presenter to screen).
 *
 * Provides:
 * - Vulkan framebuffer and corresponding attachments
 */
class RendererSurface {
public:
    static boost::leaf::result<std::unique_ptr<RendererSurface>> build(
        RendererContext* renderer,
        mff::Vector2ui dimensions
    );

    /**
     * Get renderer context used by this renderer surface
     * @return
     */
    RendererContext* get_context() const;

    /**
     * Get color image used
     * @return
     */
    const mff::vulkan::Image* get_color_image() const;

    /**
     * Get stencil image used
     * @return
     */
    const mff::vulkan::Image* get_stencil_image() const;

    /**
     * Get the framebuffer
     * @return
     */
    const mff::vulkan::Framebuffer* get_framebuffer() const;

    /**
     * Get width of renderer surface
     * @return
     */
    std::uint32_t get_width() const;

    /**
     * Get height of renderer surface
     * @return
     */
    std::uint32_t get_height() const;

    /**
     * Get dimensions in vector
     * @return
     */
    mff::Vector2ui get_dimensions() const;

private:
    RendererSurface() = default;

    RendererContext* renderer_;
    mff::Vector2ui dimensions_;

    // Framebuffer and corresponding images
    mff::vulkan::UniqueAttachmentImage image_;
    mff::vulkan::UniqueAttachmentImage stencil_;
    mff::vulkan::UniqueFramebuffer framebuffer_;
};
