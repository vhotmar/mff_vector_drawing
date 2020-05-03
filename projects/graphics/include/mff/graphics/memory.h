#pragma once

#include "vk_mem_alloc.h"

#include <mff/graphics/utils.h>
#include <mff/graphics/vulkan/device.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

// Forward declaration
class Device;

}

namespace vma {

struct Allocator;

/**
 * Wrap VMA buffer with RAII pattern (it consists of vk::Buffer and the allocated memory)
 */
class Buffer {
    friend class Allocator;

public:
    Buffer(const Buffer&) = delete;

    Buffer& operator=(const Buffer&) = delete;

    ~Buffer();

    vk::DeviceSize get_size() const;

    vk::Buffer get_buffer() const;

private:
    Buffer() = default;

    const Allocator* allocator_ = nullptr;
    vk::Buffer buffer_;
    vk::BufferCreateInfo info_;
    VmaAllocation allocation_;
};

/**
 * Wrap VMA Image with RAII pattern (it consists of vk::Image and the allocated memory)
 */
class Image {
    friend class Allocator;

public:
    Image(const Image&) = delete;

    Image& operator=(const Image&) = delete;

    ~Image();

    vk::Image get_image() const;

private:
    Image() = default;

    const Allocator* allocator_ = nullptr;
    vk::Image image_;
    vk::ImageCreateInfo info_;
    VmaAllocation allocation_;
};

/**
 * Wrappers so the api is a bit similar to vulkan.hpp
 */
using UniqueBuffer = std::unique_ptr<Buffer>;
using UniqueImage = std::unique_ptr<Image>;
using UniqueAllocator = std::unique_ptr<Allocator>;

/**
 * Wrap VMA Allocator with RAII pattern and add helper functions
 */
struct Allocator {
public:
    // Disable copying (just move)
    Allocator(const Allocator&) = delete;

    // Disable copying (just move)
    Allocator& operator=(const Allocator&) = delete;

    ~Allocator();

    /**
     * Create new allocator
     */
    static boost::leaf::result<UniqueAllocator> build(const mff::vulkan::Device* device);

    /**
     * Get the allocator handle
     * @return
     */
    const VmaAllocator get_handle() const;

    /**
     * Create buffer based on vk::BufferCreateInfo and VmaAllocationCreateInfo
     * @param buffer_info
     * @param allocation_info
     * @return
     */
    boost::leaf::result<UniqueBuffer> create_buffer(
        const vk::BufferCreateInfo& buffer_info,
        const VmaAllocationCreateInfo& allocation_info
    ) const;

    /**
     * Create image based on vk::BufferCreateInfo and VmaAllocationCreateInfo
     * @param buffer_info
     * @param allocation_info
     * @return
     */
    boost::leaf::result<UniqueImage> create_image(
        const vk::ImageCreateInfo& image_info,
        const VmaAllocationCreateInfo& allocation_info
    ) const;

private:
    Allocator() = default;

    /**
     * Device where was Allocator created
     */
    const mff::vulkan::Device* device_;
    VmaAllocator handle_;
};

}