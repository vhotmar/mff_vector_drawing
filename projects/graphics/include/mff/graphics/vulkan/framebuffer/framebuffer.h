#pragma once

#include <array>
#include <variant>

#include <mff/leaf.h>
#include <mff/graphics/vulkan/device.h>
#include <mff/graphics/vulkan/render_pass.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

namespace FramebufferBuilderDimensions_ {

struct AutoIdentical {
    std::optional<std::array<std::uint32_t, 3>> dimensions;
};

struct AutoSmaller {
    std::optional<std::array<std::uint32_t, 3>> dimensions;
};

struct Specific {
    std::array<std::uint32_t, 3> dimensions;
};

}

using FramebufferBuilderDimensions = std::variant<
    FramebufferBuilderDimensions_::AutoIdentical,
    FramebufferBuilderDimensions_::AutoSmaller,
    FramebufferBuilderDimensions_::Specific
>;

class Framebuffer;
class FramebufferBuilder;
using UniqueFramebuffer = std::unique_ptr<Framebuffer>;

class Framebuffer {
    friend class FramebufferBuilder;

public:
    vk::Framebuffer get_handle() const;

private:
    Framebuffer() = default;

    const Device* device_ = nullptr;
    const RenderPass* render_pass_ = nullptr;
    vk::UniqueFramebuffer handle_ = {};
    std::array<std::uint32_t, 3> dimensions_ = {0, 0, 0};

};

class FramebufferBuilder {
public:
    static FramebufferBuilder start(const RenderPass* render_pass);

    FramebufferBuilder& add(const ImageView* image);
    boost::leaf::result<UniqueFramebuffer> build();

private:
    FramebufferBuilder() = default;

    const RenderPass* render_pass_ = nullptr;
    std::vector<vk::ImageView> views_ = {};
    std::vector<const ImageView*> attachments_ = {};
    FramebufferBuilderDimensions dimensions_ = FramebufferBuilderDimensions_::AutoIdentical{std::nullopt};

};

}
