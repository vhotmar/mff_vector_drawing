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

    RendererContext* get_context() const;
    const mff::vulkan::Image* get_color_image() const;
    const mff::vulkan::Image* get_stencil_image() const;
    const mff::vulkan::Framebuffer* get_framebuffer() const;
    std::uint32_t get_width() const;
    std::uint32_t get_height() const;
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
