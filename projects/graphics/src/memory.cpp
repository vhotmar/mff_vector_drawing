#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <mff/graphics/memory.h>

namespace vma {

vk::DeviceSize Buffer::get_size() const {
    return info_.size;
}

vk::Buffer Buffer::get_buffer() const {
    return buffer_;
}

Buffer::~Buffer() {
    vmaDestroyBuffer(allocator_->get_handle(), buffer_, allocation_);
}

vk::Image Image::get_image() const {
    return image_;
}

Image::~Image() {
    vmaDestroyImage(allocator_->get_handle(), image_, allocation_);
}

Allocator::~Allocator() {
    vmaDestroyAllocator(handle_);
}

boost::leaf::result<UniqueAllocator> Allocator::build(const mff::vulkan::Device* device) {
    struct enable_Allocator : public Allocator {};
    std::unique_ptr<Allocator> allocator = std::make_unique<enable_Allocator>();


    VmaAllocatorCreateInfo info = {};

    info.instance = device->get_physical_device()->get_instance()->get_handle();
    info.device = device->get_handle();
    info.physicalDevice = device->get_physical_device()->get_handle();

    allocator->device_ = device;

    LEAF_CHECK(mff::to_result(vmaCreateAllocator(&info, &allocator->handle_)));

    return std::move(allocator);
}

const VmaAllocator Allocator::get_handle() const {
    return handle_;
}

boost::leaf::result<UniqueBuffer> Allocator::create_buffer(
    const vk::BufferCreateInfo& buffer_info,
    const VmaAllocationCreateInfo& allocation_info
) const {
    struct enable_Buffer : public Buffer {};
    UniqueBuffer buffer = std::make_unique<enable_Buffer>();

    buffer->info_ = buffer_info;
    buffer->allocator_ = this;

    LEAF_CHECK(
        mff::to_result(
            vmaCreateBuffer(
                handle_,
                reinterpret_cast<const VkBufferCreateInfo*>(&buffer_info),
                &allocation_info,
                reinterpret_cast<VkBuffer*>(&buffer->buffer_),
                &buffer->allocation_,
                &buffer->allocation_info_
            )));

    return std::move(buffer);
};

boost::leaf::result<UniqueImage> Allocator::create_image(
    const vk::ImageCreateInfo& image_info,
    const VmaAllocationCreateInfo& allocation_info
) const {
    struct enable_Image : public Image {};
    UniqueImage image = std::make_unique<enable_Image>();

    image->info_ = image_info;
    image->allocator_ = this;

    LEAF_CHECK(
        mff::to_result(
            vmaCreateImage(
                handle_,
                reinterpret_cast<const VkImageCreateInfo*>(&image_info),
                &allocation_info,
                reinterpret_cast<VkImage*>(&image->image_),
                &image->allocation_,
                nullptr
            )));

    return std::move(image);
};

const VmaAllocation& Buffer::get_allocation() const {
    return allocation_;
}

VmaAllocationInfo& Buffer::get_allocation_info() {
    return allocation_info_;
}


}