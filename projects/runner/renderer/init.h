#pragma once

#include <memory>

#include <mff/graphics/window.h>

#include "./renderer.h"
#include "./renderer_context.h"
#include "./renderer_surface.h"
#include "./vulkan_engine.h"
#include "./vulkan_presenter.h"

/**
 * Init everything for rendering
 * - Vulkan Engine and Presenter
 * - Renderer Context, Surface and the Renderer
 */
class RendererInit {
public:
    /**
     * Get the initiated renderer
     * @return
     */
    Renderer* get_renderer();

    /**
     * Get dimensions of framebuffer
     * @return
     */
    mff::Vector2ui get_dimensions();

    /**
     * Present buffer from RendererScreen to real screen
     * @return
     */
    boost::leaf::result<void> present();

    /**
     * Init the renderer
     * @param window
     * @return
     */
    static boost::leaf::result<std::unique_ptr<RendererInit>> build(
        const std::shared_ptr<mff::window::Window>& window
    );

private:
    RendererInit() = default;

    std::unique_ptr<VulkanEngine> engine_;
    std::unique_ptr<VulkanPresenter> presenter_;
    std::unique_ptr<RendererContext> context_;
    std::unique_ptr<RendererSurface> surface_;
    std::unique_ptr<Renderer> renderer_;
};