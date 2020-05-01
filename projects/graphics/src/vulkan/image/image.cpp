#include <mff/graphics/vulkan/image/image.h>

#include <mff/graphics/utils.h>
#include <mff/utils.h>

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
            vk::ImageAspectFlagBits::eColor,
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

vk::Format Image::get_format() {
    return get_inner_image().get_image()->get_format();
}

const InnerImage& SwapchainImage::get_inner_image() const {
    return image_;
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

}