#include <mff/graphics/vulkan/image/image.h>

#include <mff/graphics/utils.h>
#include <mff/utils.h>
#include <mff/graphics/vulkan/format.h>

namespace mff::vulkan {

boost::leaf::result<UniqueUnsafeImage> UnsafeImage::build(
    const Device* device,
    vk::ImageUsageFlags usage,
    vk::Format format,
    ImageDimensions dimensions,
    vk::SampleCountFlagBits samples,
    std::uint32_t mipmaps,
    SharingMode sharing,
    bool linear_tiling
) {
    using decoded = std::tuple<vk::ImageType, vk::Extent3D, std::uint32_t, vk::ImageCreateFlags>;
    auto[ty, extent, array_layers, flags] = std::visit(
        overloaded{
            [&](ImageDimensions_::Dim1d d1) -> decoded {
                return std::make_tuple(
                    vk::ImageType::e1D,
                    vk::Extent3D(d1.width, 1, 1),
                    d1.array_layers,
                    vk::ImageCreateFlags{}
                );
            },
            [&](ImageDimensions_::Dim2d d2) -> decoded {
                return std::make_tuple(
                    vk::ImageType::e2D,
                    vk::Extent3D(d2.width, d2.height, 1),
                    d2.array_layers,
                    d2.cubemap_compatible ? vk::ImageCreateFlagBits::eCubeCompatible : vk::ImageCreateFlags{}
                );
            },
            [&](ImageDimensions_::Dim3d d3) -> decoded {
                return std::make_tuple(
                    vk::ImageType::e3D,
                    vk::Extent3D(d3.width, d3.height, d3.depth),
                    1,
                    vk::ImageCreateFlags{}
                );
            },
        },
        dimensions
    );

    using sh_info = std::tuple<vk::SharingMode, std::vector<std::uint32_t>>;
    auto[sh_mode, sh_indices] = std::visit(
        overloaded{
            [](SharingMode_::Exclusive e) -> sh_info {
                return std::make_tuple(
                    vk::SharingMode::eExclusive,
                    std::vector<std::uint32_t>{}
                );
            },
            [](SharingMode_::Concurrent c) -> sh_info {
                return std::make_tuple(
                    vk::SharingMode::eConcurrent,
                    c.queue_families
                );
            },
        },
        sharing
    );

    vk::ImageCreateInfo info(
        {},
        ty,
        format,
        extent,
        mipmaps,
        array_layers,
        samples,
        linear_tiling ? vk::ImageTiling::eLinear : vk::ImageTiling::eOptimal,
        usage,
        sh_mode,
        sh_indices.size(),
        sh_indices.data(),
        vk::ImageLayout::eUndefined
    );

    VmaAllocationCreateInfo allocation_info = {};
    allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    struct enable_UnsafeImage : public UnsafeImage {};
    UniqueUnsafeImage result = std::make_unique<enable_UnsafeImage>();

    LEAF_AUTO_TO(result->image_, device->get_allocator()->create_image(info, allocation_info));
    result->handle_ = result->image_->get_image();

    result->device_ = device;
    result->usage_ = usage;
    result->format_ = format;
    result->dimensions_ = dimensions;
    result->samples_ = samples;
    result->mipmaps_ = mipmaps;

    auto format_properties = device->get_physical_device()->get_handle().getFormatProperties(format);
    result->format_features_ = linear_tiling ? format_properties.linearTilingFeatures
                                             : format_properties.optimalTilingFeatures;

    return result;
}

boost::leaf::result<UniqueUnsafeImage> UnsafeImage::from_existing(
    const Device* device,
    vk::Image handle,
    vk::ImageUsageFlags usage,
    vk::Format format,
    ImageDimensions dimensions,
    vk::SampleCountFlagBits samples,
    std::uint32_t mipmaps
) {
    struct enable_UnsafeImage : public UnsafeImage {};
    UniqueUnsafeImage result = std::make_unique<enable_UnsafeImage>();

    result->image_ = nullptr; // we do not own it
    result->handle_ = handle;
    result->device_ = device;
    result->usage_ = usage;
    result->format_ = format;
    result->dimensions_ = dimensions;
    result->samples_ = samples;
    result->mipmaps_ = mipmaps;

    auto format_properties = device->get_physical_device()->get_handle().getFormatProperties(format);
    result->format_features_ = format_properties.optimalTilingFeatures;

    return result;
}

vk::Format UnsafeImage::get_format() const {
    return format_;
}

vk::Image UnsafeImage::get_handle() const {
    return handle_;
}

const Device* UnsafeImage::get_device() const {
    return device_;
}

vk::ImageUsageFlags UnsafeImage::get_usage() const {
    return usage_;
}

ImageDimensions UnsafeImage::get_dimensions() const {
    return dimensions_;
}

std::uintptr_t UnsafeImage::get_key() const {
    return reinterpret_cast<std::uintptr_t>((VkImage) handle_);
}

vk::ImageView UnsafeImageView::get_handle() const {
    return handle_.get();
}

boost::leaf::result<UniqueUnsafeImageView> UnsafeImageView::build(
    const UnsafeImage* image,
    vk::ImageViewType type,
    std::uint32_t mipmap_from,
    std::uint32_t mipmap_to,
    std::uint32_t array_layer_from,
    std::uint32_t array_layer_to
) {
    struct enable_UnsafeImageView : public UnsafeImageView {};
    UniqueUnsafeImageView result = std::make_unique<enable_UnsafeImageView>();

    auto aspect_mask = ([&]() {
        auto format = image->get_format();
        vk::ImageAspectFlags result = {};

        if (is_float(format) || is_uint(format) || is_sint(format)) result |= vk::ImageAspectFlagBits::eColor;
        if (is_depth(format) || is_depthstencil(format)) result |= vk::ImageAspectFlagBits::eDepth;
        if (is_stencil(format) || is_depthstencil(format)) result |= vk::ImageAspectFlagBits::eStencil;

        return result;
    })();

    vk::ImageViewCreateInfo info(
        {},
        image->get_handle(),
        type,
        image->get_format(),
        vk::ComponentMapping(
            vk::ComponentSwizzle::eR,
            vk::ComponentSwizzle::eG,
            vk::ComponentSwizzle::eB,
            vk::ComponentSwizzle::eA
        ),
        vk::ImageSubresourceRange(
            aspect_mask,
            mipmap_from,
            mipmap_to - mipmap_from,
            array_layer_from,
            array_layer_to - array_layer_from
        )
    );

    LEAF_AUTO_TO(result->handle_, to_result(image->get_device()->get_handle().createImageViewUnique(info)));

    result->device_ = image->get_device();
    result->format_ = image->get_format();
    result->usage_ = image->get_usage();
    result->identity_swizzle_ = true;


    return result;
}

vk::Format Image::get_format() const {
    return get_inner_image().get_image()->get_format();
}

bool Image::has_color() const {
    auto format = get_format();

    return is_float(format) || is_uint(format) || is_sint(format);
}

bool Image::has_depth() const {
    auto format = get_format();

    return is_depth(format) || is_depthstencil(format);
}


bool Image::has_stencil() const {
    auto format = get_format();

    return is_stencil(format) || is_depthstencil(format);
}

ImageDimensions Image::get_dimensions() const {
    return get_inner_image().get_image()->get_dimensions();
}

boost::leaf::result<UniqueSwapchainImage> SwapchainImage::build(const Swapchain* swapchain, InnerImage image) {
    struct enable_SwapchainImage : public SwapchainImage {};
    UniqueSwapchainImage result = std::make_unique<enable_SwapchainImage>();

    result->image_ = image;
    result->swapchain_ = swapchain;

    LEAF_AUTO_TO(result->view_, UnsafeImageView::build(image.get_image(), vk::ImageViewType::e2D, 0, 1, 0, 1));

    return result;
}

const UnsafeImage* InnerImage::get_image() const {
    return image_;
}

InnerImage::InnerImage(
    const UnsafeImage* image,
    std::size_t first_layer,
    std::size_t num_layers,
    std::size_t first_mipmap_level,
    std::size_t num_mipmap_levels
)
    : image_(image)
    , first_layer_(first_layer)
    , num_layers_(num_layers)
    , first_mipmap_level_(first_mipmap_level)
    , num_mipmap_levels_(num_mipmap_levels) {

}

std::size_t InnerImage::get_first_layer() const {
    return first_layer_;
}


std::uint32_t get_width(const ImageDimensions& id) {
    return std::visit(
        overloaded{
            [&](ImageDimensions_::Dim1d it) { return it.width; },
            [&](ImageDimensions_::Dim2d it) { return it.width; },
            [&](ImageDimensions_::Dim3d it) { return it.width; },
        },
        id
    );
}

std::uint32_t get_height(const ImageDimensions& id) {
    return std::visit(
        overloaded{
            [&](ImageDimensions_::Dim1d it) { return (std::uint32_t) 1; },
            [&](ImageDimensions_::Dim2d it) { return it.height; },
            [&](ImageDimensions_::Dim3d it) { return it.height; },
        },
        id
    );
}

std::uint32_t get_depth(const ImageDimensions& id) {
    return std::visit(
        overloaded{
            [&](ImageDimensions_::Dim1d it) { return (std::uint32_t) 1; },
            [&](ImageDimensions_::Dim2d it) { return (std::uint32_t) 1; },
            [&](ImageDimensions_::Dim3d it) { return it.depth; },
        },
        id
    );
}

std::uint32_t get_array_layers(const ImageDimensions& id) {
    return std::visit(
        overloaded{
            [&](ImageDimensions_::Dim1d it) { return (std::uint32_t) 1; },
            [&](ImageDimensions_::Dim2d it) { return (std::uint32_t) 1; },
            [&](ImageDimensions_::Dim3d it) { return (std::uint32_t) 1; },
        },
        id
    );
}

const InnerImage& AttachmentImage::ImageImpl::get_inner_image() const {
    return image_->inner_image_;
}

AttachmentImage::ImageImpl::ImageImpl(const AttachmentImage* image)
    : image_(image) {
}

const UnsafeImageView* AttachmentImage::ImageViewImpl::get_inner_image_view() const {
    return image_->view_.get();
}

ImageDimensions AttachmentImage::ImageViewImpl::get_dimensions() const {
    return image_->image_->get_dimensions();
}

AttachmentImage::ImageViewImpl::ImageViewImpl(const AttachmentImage* image)
    : image_(image) {
}

boost::leaf::result<UniqueAttachmentImage> AttachmentImage::build(
    const Device* device,
    std::array<std::uint32_t, 2> dimensions,
    vk::Format format
) {
    return AttachmentImage::build(device, dimensions, format, vk::ImageUsageFlags{}, vk::SampleCountFlagBits::e1);
}

boost::leaf::result<UniqueAttachmentImage> AttachmentImage::build(
    const Device* device,
    std::array<std::uint32_t, 2> dimensions,
    vk::Format format,
    vk::ImageUsageFlags usage,
    vk::SampleCountFlagBits samples
) {
    struct enable_AttachmentImage : public AttachmentImage {};
    UniqueAttachmentImage result = std::make_unique<enable_AttachmentImage>();

    bool depth = (is_depth(format) || is_depthstencil(format) || is_stencil(format));

    if (depth) {
        usage = (usage | vk::ImageUsageFlagBits::eDepthStencilAttachment) & (~vk::ImageUsageFlagBits::eColorAttachment);
    } else {
        usage = (usage | vk::ImageUsageFlagBits::eColorAttachment) & (~vk::ImageUsageFlagBits::eDepthStencilAttachment);
    }

    LEAF_AUTO_TO(
        result->image_,
        UnsafeImage::build(
            device,
            usage,
            format,
            ImageDimensions_::Dim2d{dimensions[0], dimensions[1], 1, false},
            samples,
            1,
            SharingMode_::Exclusive{},
            false
        ));

    LEAF_AUTO_TO(result->view_, UnsafeImageView::build(result->image_.get(), vk::ImageViewType::e2D, 0, 1, 0, 1));

    result->inner_image_ = InnerImage(result->image_.get(), 0, 1, 0, 1);
    result->format_ = format;

    result->image_impl_ = std::make_unique<ImageImpl>(result.get());
    result->image_view_impl_ = std::make_unique<ImageViewImpl>(result.get());

    return result;
}

const Image* AttachmentImage::get_image_impl() const { return image_impl_.get(); }
const ImageView* AttachmentImage::get_image_view_impl() const { return image_view_impl_.get(); }

}