#include <mff/graphics/vulkan/framebuffer/framebuffer.h>
#include <mff/utils.h>

namespace mff::vulkan {

FramebufferBuilder FramebufferBuilder::start(const RenderPass* render_pass) {
    FramebufferBuilder builder;

    builder.render_pass_ = render_pass;

    return builder;
}

FramebufferBuilder& FramebufferBuilder::add(const ImageView* image) {
    auto image_dimensions = image->get_dimensions();

    auto new_dimensions = std::visit(
        overloaded{
            [&](FramebufferBuilderDimensions_::AutoIdentical ai) -> FramebufferBuilderDimensions {
                if (ai.dimensions) {
                    // TODO: check compatibility
                    return FramebufferBuilderDimensions_::AutoIdentical{ai.dimensions};
                }

                return FramebufferBuilderDimensions_::AutoIdentical{{{
                    get_width(image_dimensions),
                    get_height(image_dimensions),
                    get_array_layers(image_dimensions)
                }}};
            },
            [&](FramebufferBuilderDimensions_::AutoSmaller ai) -> FramebufferBuilderDimensions {
                if (ai.dimensions) {
                    // TODO: check compatibility
                    return FramebufferBuilderDimensions_::AutoSmaller{{{
                        std::min(ai.dimensions.value()[0], get_width(image_dimensions)),
                        std::min(ai.dimensions.value()[1], get_height(image_dimensions)),
                        std::min(ai.dimensions.value()[2], get_array_layers(image_dimensions)),
                    }}};
                }

                return FramebufferBuilderDimensions_::AutoSmaller{{{
                    get_width(image_dimensions),
                    get_height(image_dimensions),
                    get_array_layers(image_dimensions)
                }}};
            },
            [&](FramebufferBuilderDimensions_::Specific ai) -> FramebufferBuilderDimensions {
                return FramebufferBuilderDimensions_::Specific{{{
                    get_width(image_dimensions),
                    get_height(image_dimensions),
                    get_array_layers(image_dimensions)
                }}};
            }
        },
        dimensions_
    );

    dimensions_ = new_dimensions;
    views_.push_back(image->get_inner_image_view()->get_handle());
    attachments_.push_back(image);

    return *this;
}

boost::leaf::result<UniqueFramebuffer> FramebufferBuilder::build() {
    auto device = render_pass_->get_device();

    using dimensions_result_t = boost::leaf::result<std::array<std::uint32_t, 3>>;
    auto dimensions_result = std::visit(
        overloaded{
            [&](FramebufferBuilderDimensions_::AutoIdentical ai) -> dimensions_result_t {
                if (ai.dimensions) {
                    return ai.dimensions.value();
                }

                return LEAF_NEW_ERROR();
            },
            [&](FramebufferBuilderDimensions_::AutoSmaller ai) -> dimensions_result_t {
                if (ai.dimensions) {
                    return ai.dimensions.value();
                }

                return LEAF_NEW_ERROR();
            },
            [&](FramebufferBuilderDimensions_::Specific ai) -> dimensions_result_t {
                return ai.dimensions;
            }
        },
        dimensions_
    );

    LEAF_AUTO(dimensions, dimensions_result);

    struct enable_Framebuffer: public Framebuffer {};
    UniqueFramebuffer result = std::make_unique<enable_Framebuffer>();

    vk::FramebufferCreateInfo info(
        {},
        render_pass_->get_handle(),
        views_.size(),
        views_.data(),
        dimensions[0],
        dimensions[1],
        dimensions[2]
    );

    result->dimensions_ = dimensions;
    result->device_ = device;
    result->render_pass_ = render_pass_;
    LEAF_AUTO_TO(result->handle_, to_result(device->get_handle().createFramebufferUnique(info)));

    return result;
}

vk::Framebuffer Framebuffer::get_handle() const {
    return handle_.get();
}

}