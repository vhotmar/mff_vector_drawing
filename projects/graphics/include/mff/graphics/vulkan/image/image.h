#pragma once

#include <variant>

#include <mff/graphics/memory.h>
#include <mff/graphics/vulkan/device.h>
#include <mff/graphics/vulkan/sync/sync.h>
#include <mff/graphics/vulkan/swapchain.h>
#include <mff/graphics/vulkan/vulkan.h>
#include <mff/utils.h>

namespace vma {

class Image;
using UniqueImage = std::unique_ptr<Image>;

}

namespace mff::vulkan {

namespace ImageDimensions_ {

struct Dim1d {
    std::uint32_t width;
    std::uint32_t array_layers;
};

struct Dim2d {
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t array_layers;
    bool cubemap_compatible;
};

struct Dim3d {
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t depth;
};

}

using ImageDimensions = std::variant<ImageDimensions_::Dim1d, ImageDimensions_::Dim2d, ImageDimensions_::Dim3d>;

std::uint32_t get_width(const ImageDimensions& id);
std::uint32_t get_height(const ImageDimensions& id);
std::uint32_t get_depth(const ImageDimensions& id);
std::uint32_t get_array_layers(const ImageDimensions& id);

class UnsafeImage;

using UniqueUnsafeImage = std::unique_ptr<UnsafeImage>;

class UnsafeImage {
public:
    vk::Format get_format() const;

    vk::Image get_handle() const;
    const Device* get_device() const;
    vk::ImageUsageFlags get_usage() const;
    ImageDimensions get_dimensions() const;

    static boost::leaf::result<UniqueUnsafeImage> build(
        const Device* device,
        vk::ImageUsageFlags usage,
        vk::Format format,
        ImageDimensions dimensions,
        vk::SampleCountFlagBits samples,
        std::uint32_t mipmaps,
        SharingMode sharing,
        bool linear_tiling
    );

    static boost::leaf::result<UniqueUnsafeImage> from_existing(
        const Device* device,
        vk::Image handle,
        vk::ImageUsageFlags usage,
        vk::Format format,
        ImageDimensions dimensions,
        vk::SampleCountFlagBits samples,
        std::uint32_t mipmaps
    );

private:
    UnsafeImage() = default;

    const Device* device_ = nullptr;
    /**
     * Used for destroying assets if this UnsafeImage owns the asset
     */
    vk::Image handle_ = nullptr;
    vma::UniqueImage image_ = nullptr;
    vk::ImageUsageFlags usage_ = {};
    vk::Format format_ = {};
    ImageDimensions dimensions_ = ImageDimensions_::Dim1d{};
    vk::SampleCountFlagBits samples_ = vk::SampleCountFlagBits::e1;
    std::uint32_t mipmaps_ = 1;
    vk::FormatFeatureFlags format_features_ = {};
};

class UnsafeImageView;

using UniqueUnsafeImageView = std::unique_ptr<UnsafeImageView>;

class UnsafeImageView {
public:
    vk::ImageView get_handle() const;

    static boost::leaf::result<UniqueUnsafeImageView> build(
        const UnsafeImage* image,
        vk::ImageViewType type,
        std::uint32_t mipmap_from,
        std::uint32_t mipmap_to,
        std::uint32_t array_layer_from,
        std::uint32_t array_layer_to
    );

private:
    UnsafeImageView() = default;

    vk::UniqueImageView handle_ = {};
    const Device* device_ = nullptr;
    vk::ImageUsageFlags usage_;
    bool identity_swizzle_ = true;
    vk::Format format_;
};

class InnerImage {
public:
    const UnsafeImage* get_image() const;

    InnerImage(
        const UnsafeImage* image,
        std::size_t first_layer,
        std::size_t num_layers,
        std::size_t first_mipmap_level,
        std::size_t num_mipmap_levels
    );

    std::size_t get_first_layer() const;

private:
    const UnsafeImage* image_;
    std::size_t first_layer_;
    std::size_t num_layers_;
    std::size_t first_mipmap_level_;
    std::size_t num_mipmap_levels_;
};

class Image {
public:
    virtual const InnerImage& get_inner_image() const = 0;

    virtual vk::Format get_format() const;
    virtual bool has_color() const;
    virtual bool has_depth() const;
    virtual bool has_stencil() const;
    virtual ImageDimensions get_dimensions() const;

protected:
    Image() = default;
};

class ImageView {
public:
    virtual const UnsafeImageView* get_inner_image_view() const = 0;
    virtual ImageDimensions get_dimensions() const;
};

class Swapchain;
class SwapchainImage;
using UniqueSwapchainImage = std::unique_ptr<SwapchainImage>;

class SwapchainImage {
public:
    class ImageImpl: public Image {
        const InnerImage& get_inner_image() const override;
    };

    class ImageViewImpl: public ImageView {
        const UnsafeImageView* get_inner_image_view() const override;
    };

    const ImageImpl* get_image_impl();
    const ImageViewImpl* get_image_view_impl();

    static boost::leaf::result<UniqueSwapchainImage> build(const Swapchain* swapchain, InnerImage image);

private:
    SwapchainImage() = default;

    const Swapchain* swapchain_ = nullptr;
    InnerImage image_ = {nullptr, 0, 1, 0, 1};
    UniqueUnsafeImageView view_ = nullptr;
    std::unique_ptr<ImageImpl> image_impl_;
    std::unique_ptr<ImageViewImpl> image_view_impl;
};

}