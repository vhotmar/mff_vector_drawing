#include "./renderer_surface.h"

boost::leaf::result<std::unique_ptr<RendererSurface>> RendererSurface::build(
    RendererContext* renderer,
    mff::Vector2ui dimensions
) {
    logger::main->debug("Building RendererSurface");
    struct enable_RendererSurface : public RendererSurface {};
    std::unique_ptr<RendererSurface> result = std::make_unique<enable_RendererSurface>();

    result->renderer_ = renderer;
    result->dimensions_ = dimensions;

    // init images
    LEAF_AUTO_TO(
        result->image_,
        mff::vulkan::AttachmentImage::build(
            result->renderer_->get_device(),
            mff::to_array(dimensions),
            renderer->get_color_attachment_format(),
            vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment
                | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc,
            vk::SampleCountFlagBits::e1
        ));

    LEAF_AUTO_TO(
        result->stencil_,
        mff::vulkan::AttachmentImage::build(
            result->renderer_->get_device(),
            mff::to_array(dimensions),
            renderer->get_stencil_attachment_format(),
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst
                | vk::ImageUsageFlagBits::eTransferSrc,
            vk::SampleCountFlagBits::e1
        ));

    // init framebuffer
    LEAF_AUTO_TO(
        result->framebuffer_,
        mff::vulkan::FramebufferBuilder::start(result->renderer_->get_renderpass())
            .add(result->image_->get_image_view_impl())
            .add(result->stencil_->get_image_view_impl())
            .build());

    return result;
}

RendererContext* RendererSurface::get_context() const {
    return renderer_;
}

const mff::vulkan::Image* RendererSurface::get_color_image() const {
    return image_->get_image_impl();
}

const mff::vulkan::Image* RendererSurface::get_stencil_image() const {
    return stencil_->get_image_impl();
}

const mff::vulkan::Framebuffer* RendererSurface::get_framebuffer() const {
    return framebuffer_.get();
}

std::uint32_t RendererSurface::get_width() const {
    return dimensions_[0];
}

std::uint32_t RendererSurface::get_height() const {
    return dimensions_[1];
}

mff::Vector2ui RendererSurface::get_dimensions() const {
    return dimensions_;
}