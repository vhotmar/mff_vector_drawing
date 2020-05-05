#pragma once

#include <memory>

#include <mff/graphics/window.h>

#include "./renderer.h"
#include "./renderer_context.h"
#include "./renderer_surface.h"
#include "./vulkan_engine.h"
#include "./vulkan_presenter.h"

class RendererInit {
public:
    Renderer* get_renderer();
    mff::Vector2ui get_dimensions();

    boost::leaf::result<void> present();

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