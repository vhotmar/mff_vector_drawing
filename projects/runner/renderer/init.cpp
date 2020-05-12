#include "./init.h"

boost::leaf::result<std::unique_ptr<RendererInit>> RendererInit::build(
    const std::shared_ptr<mff::window::Window>& window
) {
    logger::main->debug("Building RenderInit");
    struct enable_RenderInit : public RendererInit {};
    std::unique_ptr<RendererInit> result = std::make_unique<enable_RenderInit>();

    LEAF_AUTO_TO(result->engine_, VulkanEngine::build(window));
    LEAF_AUTO_TO(result->presenter_, VulkanPresenter::build(result->engine_.get()));
    LEAF_AUTO_TO(
        result->context_,
        RendererContext::build(result->engine_.get(), result->presenter_->get_format()));

    // TODO: make "rebuild function" from following (should react to resizing)
    LEAF_AUTO_TO(
        result->surface_,
        RendererSurface::build(result->context_.get(), result->presenter_->get_dimensions()));
    LEAF_AUTO_TO(
        result->renderer_,
        Renderer::build(result->surface_.get(), result->engine_->get_queues().graphics_queue));

    // the only special thing here is recording the commands to copy the RendererScreen buffer to
    // the screen from presenter (so user can see the resulting image)
    LEAF_CHECK(
        result
            ->presenter_
            ->build_commands(result->surface_->get_color_image(), result->presenter_->get_dimensions()));

    return result;
}

Renderer* RendererInit::get_renderer() {
    return renderer_.get();
}

mff::Vector2ui RendererInit::get_dimensions() {
    return surface_->get_dimensions();
}

boost::leaf::result<void> RendererInit::present() {
    LEAF_AUTO(fresh, presenter_->draw());

    // if is swapchain invalidated rebuild the commands (so they refer to the new swapchain)
    if (!fresh) {
        LEAF_CHECK(presenter_->build_commands(surface_->get_color_image(), presenter_->get_dimensions()));
    }

    // wait for completion
    engine_->get_device()->get_handle().waitIdle();

    return {};
}