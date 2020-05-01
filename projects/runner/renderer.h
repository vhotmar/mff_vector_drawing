#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <range/v3/all.hpp>
#include <mff/leaf.h>
#include <mff/algorithms.h>
#include <mff/graphics/math.h>
#include <mff/graphics/memory.h>
#include <mff/graphics/vulkan/dispatcher.h>
#include <mff/graphics/vulkan/instance.h>
#include <mff/graphics/vulkan/vulkan.h>
#include <mff/graphics/window.h>
#include <Eigen/Dense>

#include "./utils/logger.h"


#ifdef NDEBUG
const bool kDEBUG = false;
#else
const bool kDEBUG = true;
#endif

/////////////////
/// Constants ///
/////////////////

const bool kVULKAN_DEBUG = kDEBUG;

////////////////////
/// Leaf helpers ///
////////////////////


namespace boost::leaf {

template <>
struct is_e_type<vk::Result> : public std::true_type {};

}

template <typename T>
boost::leaf::result<T> to_result(vk::ResultValue<T> vk_result) {
    if (vk_result.result != vk::Result::eSuccess) {
        return boost::leaf::new_error(vk_result.result);
    }

    return std::move(vk_result.value);
}

boost::leaf::result<void> to_result(VkResult vk_result) {
    if (vk_result != VK_SUCCESS) {
        return boost::leaf::new_error(static_cast<vk::Result>(vk_result));
    }

    return {};
}

//////////////////////
/// Queue families ///
//////////////////////

struct QueueFamilyIndices {
    std::optional<std::uint32_t> graphics_family = std::nullopt;
    std::optional<std::uint32_t> present_family = std::nullopt;
    std::optional<std::uint32_t> transfer_family = std::nullopt;
    std::optional<std::uint32_t> compute_family = std::nullopt;

    bool is_complete() {
        return graphics_family.has_value() && present_family.has_value() && transfer_family.has_value()
            && compute_family.has_value();
    }
};

QueueFamilyIndices find_queue_families(
    vk::PhysicalDevice physical_device,
    vk::SurfaceKHR surface
) {
    QueueFamilyIndices indices;

    auto queue_families = physical_device.getQueueFamilyProperties();

    std::uint32_t i = 0;

    for (const auto& family: queue_families) {
        if (family.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphics_family = i;
        }

        if (family.queueFlags & vk::QueueFlagBits::eTransfer) {
            indices.transfer_family = i;
        }

        if (family.queueFlags & vk::QueueFlagBits::eCompute) {
            indices.compute_family = i;
        }

        LEAF_DEFAULT(
            is_surface_supported,
            false,
            to_result(physical_device.getSurfaceSupportKHR(i, surface)));

        if (is_surface_supported) {
            indices.present_family = i;
        }

        if (indices.is_complete()) {
            break;
        }

        i++;
    }

    return indices;
}

/////////////////
/// Swapchain ///
/////////////////

struct SwapchainCapabilities {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> present_modes;
};

std::optional<SwapchainCapabilities> query_swapchain_support(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    auto capabilities_result = to_result(device.getSurfaceCapabilitiesKHR(surface));
    if (!capabilities_result) return std::nullopt;

    auto formats_result = to_result(device.getSurfaceFormatsKHR(surface));
    if (!formats_result) return std::nullopt;

    auto present_modes_result = to_result(device.getSurfacePresentModesKHR(surface));
    if (!present_modes_result) return std::nullopt;

    return SwapchainCapabilities{
        capabilities_result.value(),
        formats_result.value(),
        present_modes_result.value()
    };
}

vk::SurfaceFormatKHR choose_swap_surface_format(const SwapchainCapabilities& capabilities) {
    for (const auto& available_format : capabilities.formats) {
        if (available_format.format == vk::Format::eB8G8R8A8Srgb
            && available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return available_format;
        }
    }

    return capabilities.formats.front();
}

vk::PresentModeKHR choose_swap_present_mode(const SwapchainCapabilities& capabilities) {
    for (const auto& available_present_mode : capabilities.present_modes) {
        if (available_present_mode == vk::PresentModeKHR::eMailbox) {
            return available_present_mode;
        }
    }

    return capabilities.present_modes.front();
}

vk::Extent2D choose_swap_extent(const SwapchainCapabilities& capabilities_, vk::Extent2D actual_extent) {
    auto capabilities = capabilities_.capabilities;

    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        vk::Extent2D result_extent(actual_extent);

        result_extent.width = std::max(
            capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, actual_extent.width));
        result_extent.height = std::max(
            capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, actual_extent.height));

        return result_extent;
    }
}

///////////////////////
/// Physical device ///
///////////////////////

bool is_device_suitable(
    vk::PhysicalDevice physical_device,
    vk::SurfaceKHR surface,
    const std::vector<std::string>& required_extensions
) {

    bool is_discrete = physical_device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
    bool has_queue_families = find_queue_families(physical_device, surface).is_complete();

    bool required_extensions_supported = true;

    auto get_extensions = [&]() -> std::vector<vk::ExtensionProperties> {
        auto extensions = to_result(physical_device.enumerateDeviceExtensionProperties());

        if (!extensions) return {};

        return extensions.value();
    };

    auto extensions = get_extensions();

    auto has_extension = [&](const std::string& required_extension) {
        return !mff::contains_if(
            extensions,
            [&](auto extension) { return extension.extensionName == required_extension; }
        );
    };

    for (auto required_extension: required_extensions) {
        if (has_extension(required_extension)) {
            required_extensions_supported = false;
            break;
        }
    }

    bool swapchain_adequate = false;

    // we can't query for swapchain support if the swapchain extension is not supported
    if (required_extensions_supported) {
        auto capabilities_result = query_swapchain_support(physical_device, surface);

        if (capabilities_result) {
            auto details = capabilities_result.value();
            swapchain_adequate = !details.formats.empty() && !details.present_modes.empty();
        }
    }

    return is_discrete
        && has_queue_families
        && required_extensions_supported
        && swapchain_adequate;
}

//////////////////////////////
/// Vulkan Debug Utilities ///
//////////////////////////////

VkBool32 debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT flags,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* p_user_data
) {
    auto level = spdlog::level::level_enum::debug;

    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        level = spdlog::level::level_enum::err;
    } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        level = spdlog::level::level_enum::warn;
    } else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        level = spdlog::level::level_enum::info;
    }

    mff::logger::vulkan->log(level, "Code {0}: {1}", data->pMessageIdName, data->pMessage);

    return false;
}

vk::DebugUtilsMessengerCreateInfoEXT get_debug_utils_create_info() {
    return vk::DebugUtilsMessengerCreateInfoEXT(
        {},
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
            | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
            | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
        &debug_callback,
        nullptr
    );
}

//////////////////////
/// File Utilities ///
//////////////////////

/**
 * Read file from specified path
 * @param path
 * @return
 */
std::vector<char> read_file(const std::string& path) {
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

    std::ifstream::pos_type file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> bytes(file_size);
    file.read(bytes.data(), file_size);
    file.close();

    return bytes;
}

/////////////////////////
/// General Utilities ///
/////////////////////////

/**
 * Convert TContainer<std::string> to std::vector<const char*> which is consumable by Vulkan API
 * @tparam TContainer the container containing strings
 * @param data the real value of container
 * @return
 */
template <typename TContainer>
std::vector<const char*> to_pointer_char_data(const TContainer& data) {
    std::vector<const char*> result;

    for (const auto& item: data)
        result.push_back(&item.front());

    return result;
}

vk::Extent2D to_extent(mff::Vector2ui v) {
    return vk::Extent2D(v[0], v[1]);
}

vk::Offset2D to_offset(mff::Vector2ui v) {
    return vk::Offset2D(v[0], v[1]);
}

////////////////////////////
/// Vertex Input Structs ///
////////////////////////////

struct Vertex {
    mff::Vector2f pos;

    static vk::VertexInputBindingDescription get_binding_description() {
        vk::VertexInputBindingDescription bindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex);

        return bindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 1> get_attribute_descriptions() {
        return {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos)),
        };
    }
};

struct UBO {
    mff::Vector4f color;
    std::float_t scale;
};

/////////////////////////////////
/// Memory allocator wrappers ///
/////////////////////////////////

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

    vk::DeviceSize get_size() const {
        return info_.size;
    }

    vk::Buffer get_buffer() {
        return buffer_;
    }

private:
    Buffer() = default;

    const Allocator* allocator_;
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

    vk::Image get_image() {
        return image_;
    }

private:
    Image() = default;

    const Allocator* allocator_;
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

    ~Allocator() {
        vmaDestroyAllocator(handle_);
    }

    /**
     * Create new allocator
     */
    static boost::leaf::result<UniqueAllocator> build(VmaAllocatorCreateInfo info) {
        struct enable_Allocator : public Allocator {};
        std::unique_ptr<Allocator> allocator = std::make_unique<enable_Allocator>();

        allocator->device_ = info.device;

        LEAF_CHECK(to_result(vmaCreateAllocator(&info, &allocator->handle_)));

        return std::move(allocator);
    }

    /**
     * Get the allocator handle
     * @return
     */
    const VmaAllocator get_handle() const {
        return handle_;
    }

    /**
     * Create buffer based on vk::BufferCreateInfo and VmaAllocationCreateInfo
     * @param buffer_info
     * @param allocation_info
     * @return
     */
    boost::leaf::result<UniqueBuffer> create_buffer(
        const vk::BufferCreateInfo& buffer_info,
        const VmaAllocationCreateInfo& allocation_info
    ) {
        struct enable_Buffer : public Buffer {};
        UniqueBuffer buffer = std::make_unique<enable_Buffer>();

        buffer->info_ = buffer_info;
        buffer->allocator_ = this;

        LEAF_CHECK(
            to_result(
                vmaCreateBuffer(
                    handle_,
                    reinterpret_cast<const VkBufferCreateInfo*>(&buffer_info),
                    &allocation_info,
                    reinterpret_cast<VkBuffer*>(&buffer->buffer_),
                    &buffer->allocation_,
                    nullptr
                )));

        return std::move(buffer);
    };

    /**
     * Create image based on vk::BufferCreateInfo and VmaAllocationCreateInfo
     * @param buffer_info
     * @param allocation_info
     * @return
     */
    boost::leaf::result<UniqueImage> create_image(
        const vk::ImageCreateInfo& image_info,
        const VmaAllocationCreateInfo& allocation_info
    ) {
        struct enable_Image : public Image {};
        UniqueImage image = std::make_unique<enable_Image>();

        image->info_ = image_info;
        image->allocator_ = this;

        LEAF_CHECK(
            to_result(
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

private:
    Allocator() = default;

    /**
     * Device where was Allocator created
     */
    vk::Device device_;
    VmaAllocator handle_;
};

// Delayed because of Allocator completness
Buffer::~Buffer() {
    vmaDestroyBuffer(allocator_->get_handle(), buffer_, allocation_);
}

// Delayed because of Allocator completness
Image::~Image() {
    vmaDestroyImage(allocator_->get_handle(), image_, allocation_);
}

}


////////////////////////
/// Vulkan utilities ///
////////////////////////

/**
 * Keep the Queue index
 */
struct Queue {
    vk::Queue queue;
    std::uint32_t index;
};

/**
 * Get format from candidates which has specified features
 * @param physical_device
 * @param candidates
 * @param features
 * @return the supported format or std::nullopt
 */
std::optional<vk::Format> find_supported_format(
    vk::PhysicalDevice physical_device,
    const std::vector<vk::Format>& candidates,
    vk::FormatFeatureFlags features
) {
    auto i = ranges::find_if(
        candidates,
        [&](const auto& format) {
            auto props = physical_device.getFormatProperties(format);

            return (props.optimalTilingFeatures & features) == features;
        }
    );

    if (i == ranges::end(candidates)) return std::nullopt;

    return *i;
}

/**
 * Get stencil format which is supported by physical deice
 * @param physical_device
 * @return the best stencil format we want or std::nullopt
 */
std::optional<vk::Format> find_stencil_format(
    vk::PhysicalDevice physical_device
) {
    return find_supported_format(
        physical_device,
        {vk::Format::eS8Uint},
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

/**
 * Create render pass with specified load operations
 * @param device vk::Device where to create the render_pass
 * @param physical_device vk::PhysicalDevice (the origin of device)
 * @param color_format the color format ot use (use swapchain format)
 * @param load_op
 * @param stencil_load_op
 * @return
 */
boost::leaf::result<vk::UniqueRenderPass> create_render_pass(
    const vk::Device& device,
    const vk::PhysicalDevice& physical_device,
    const vk::Format& color_format,
    const vk::AttachmentLoadOp& load_op,
    const vk::AttachmentLoadOp& stencil_load_op
) {
    auto stencil_format = find_stencil_format(physical_device);
    if (!stencil_format) return boost::leaf::new_error();

    vk::AttachmentDescription color_attachment(
        {},
        color_format,
        vk::SampleCountFlagBits::e1,
        load_op,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::eColorAttachmentOptimal
    );

    vk::AttachmentDescription stencil_attachment(
        {},
        stencil_format.value(),
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        stencil_load_op,
        vk::AttachmentStoreOp::eStore,
        vk::ImageLayout::eDepthStencilAttachmentOptimal,
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    );

    std::vector<vk::AttachmentDescription> attachments = {color_attachment, stencil_attachment};

    vk::AttachmentReference color_attachment_ref(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depth_attachment_ref(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::SubpassDescription subpass(
        {},
        vk::PipelineBindPoint::eGraphics,
        0,
        nullptr,
        1,
        &color_attachment_ref,
        nullptr,
        &depth_attachment_ref
    );

    vk::SubpassDependency color_dependency(
        VK_SUBPASS_EXTERNAL,
        0,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::AccessFlagBits::eMemoryRead,
        vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead,
        vk::DependencyFlagBits::eByRegion
    );

    vk::SubpassDependency depth_dependency(
        0,
        VK_SUBPASS_EXTERNAL,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead,
        vk::AccessFlagBits::eMemoryRead,
        vk::DependencyFlagBits::eByRegion
    );

    std::vector<vk::SubpassDependency> dependencies = {color_dependency, depth_dependency};

    vk::RenderPassCreateInfo render_pass_info(
        {},
        attachments.size(),
        attachments.data(),
        1,
        &subpass,
        dependencies.size(),
        dependencies.data()
    );

    return to_result(device.createRenderPassUnique(render_pass_info));
}

/**
 * This class provides us with resources which do not change really
 */
class BaseEngine {
public:
    static boost::leaf::result<std::unique_ptr<BaseEngine>> build(const std::shared_ptr<mff::window::Window>& window) {
        struct enable_BaseEngine : public BaseEngine {};
        std::unique_ptr<BaseEngine> engine = std::make_unique<enable_BaseEngine>();

        engine->window_ = window;

        LEAF_AUTO_TO(engine->instance_, mff::graphics::Instance::build());
    }

private:
    BaseEngine() = default;

    /**
     * Window on which we will initialize the vulkan context + draw (no multi window for now)
     */
    std::shared_ptr<mff::window::Window> window_ = nullptr;

    mff::graphics::UniqueInstance instance_ = nullptr;


    /**
     * Required device extensions
     */
    std::vector<std::string> device_extensions_ = {{VK_KHR_SWAPCHAIN_EXTENSION_NAME}};

    vk::UniqueInstance instance_;
    vk::UniqueDebugUtilsMessengerEXT debug_utils_messenger_;
    vk::UniqueSurfaceKHR surface_;
    vk::PhysicalDevice physical_device_;
    vk::UniqueDevice device_;
    vma::UniqueAllocator allocator_;
    Queue graphics_queue_;
    Queue present_queue_;
    std::vector<std::uint32_t> queue_family_indices_;
};

////////////////
/// Renderer ///
////////////////

class Renderer {
public:
    Renderer(const std::shared_ptr<mff::window::Window>& window)
        : window_(window) {

    }

    boost::leaf::result<void> init() {
        instance_extensions_ = window_->get_required_extensions();

        LEAF_CHECK(init_instance());
        LEAF_CHECK(init_surface());
        LEAF_CHECK(pick_physical_device());
        LEAF_CHECK(init_logical_device());
        LEAF_CHECK(init_allocator());
        LEAF_CHECK(init_swapchain());
        LEAF_CHECK(init_image_views());
        LEAF_CHECK(init_render_pass());
        LEAF_CHECK(init_descriptor_set_layout());
        LEAF_CHECK(init_graphics_pipeline());
        LEAF_CHECK(init_command_pool());
        LEAF_CHECK(init_depth_resources());
        LEAF_CHECK(init_framebuffers());
        LEAF_CHECK(request_vertex_buffer(1024 * 1024));

        return {};
    }

private:
    boost::leaf::result<void> init_instance() {
        mff::vulkan::init_dispatcher();

        LEAF_AUTO(instance_extensions, to_result(vk::enumerateInstanceExtensionProperties()));

        // check whether extension exists in instance
        auto has_extension = [&](std::string extension) -> bool {
            return mff::contains_if(
                instance_extensions,
                [&](auto item) { return item.extensionName == extension; }
            );
        };

        // add extension and returns true if is available / we are already requiring it
        auto add_extension = [&](const std::string& extension) -> bool {
            if (mff::contains(instance_extensions_, extension)) return true;
            if (!has_extension(extension)) return false;

            instance_extensions_.push_back(extension);

            return true;
        };

        if (kVULKAN_DEBUG) {
            bool has_debug_extension = false;

            has_debug_extension = add_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME) || has_debug_extension;
            has_debug_extension = add_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME) || has_debug_extension;

            if (!has_debug_extension) {
                // we are in debug but extension not found
                return boost::leaf::new_error();
            }
        }

        // check whether we can use the extensions
        for (const auto& extension: instance_extensions_) {
            if (!has_extension(extension)) {
                return boost::leaf::new_error();
            }
        }

        // get supported instance layers
        LEAF_AUTO(instance_layers, to_result(vk::enumerateInstanceLayerProperties()));

        // check if is layer supported
        auto has_layer = [&](std::string layer) -> bool {
            return mff::contains_if(
                instance_layers,
                [&](auto item) { return item.layerName == layer; }
            );
        };

        // add layer and return true if is layer required by user or supported by instance
        auto add_layer = [&](const std::string& layer) -> bool {
            if (mff::contains(instance_layers_, layer)) return true;
            if (!has_layer(layer)) return false;

            instance_layers_.push_back(layer);

            return true;
        };

        if (kVULKAN_DEBUG) {
            bool has_debug_layer = false;

            has_debug_layer = has_debug_layer || add_layer("VK_LAYER_LUNARG_standard_validation");

            if (!has_debug_layer) {
                // we are in debug but debug layer not found
                return boost::leaf::new_error();
            }
        }

        // check whether we can use the layers
        for (const auto& layer: instance_layers_) {
            if (!has_layer(layer)) {
                return boost::leaf::new_error();
            }
        }

        // vulkan needs strings as utf-8 char pointers
        auto extensions_c = to_pointer_char_data(instance_extensions_);
        auto layers_c = to_pointer_char_data(instance_layers_);

        std::optional<vk::ApplicationInfo> application_info = std::nullopt;

        vk::InstanceCreateInfo create_instance_info(
            {},
            application_info.has_value() ? &application_info.value() : nullptr,
            layers_c.size(),
            layers_c.data(),
            extensions_c.size(),
            extensions_c.data()
        );

        if (kVULKAN_DEBUG) {
            vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT> chain = {
                create_instance_info,
                get_debug_utils_create_info()
            };

            LEAF_AUTO_TO(instance_, to_result(vk::createInstanceUnique(chain.get<vk::InstanceCreateInfo>())));
        } else {
            LEAF_AUTO_TO(instance_, to_result(vk::createInstanceUnique(create_instance_info)));
        }

        // add instance functions to dispatcher
        mff::vulkan::init_dispatcher(instance_.get());


        if (kVULKAN_DEBUG) {
            auto create_info = get_debug_utils_create_info();
            LEAF_AUTO_TO(debug_utils_messenger_, to_result(instance_->createDebugUtilsMessengerEXTUnique(create_info)));
        }

        return {};
    }


    boost::leaf::result<void> init_swapchain() {
        auto swapchain_support = query_swapchain_support(physical_device_, surface_.get()).value();

        auto surface_format = choose_swap_surface_format(swapchain_support);
        auto present_mode = choose_swap_present_mode(swapchain_support);
        auto extent = choose_swap_extent(swapchain_support, to_extent(window_->get_inner_size()));

        auto image_count = swapchain_support.capabilities.minImageCount + 1;

        if (swapchain_support.capabilities.maxImageCount
            && image_count > swapchain_support.capabilities.maxImageCount) {
            image_count = swapchain_support.capabilities.maxImageCount;
        }

        auto different_queues = queue_family_indices_.size() > 1;

        vk::SwapchainCreateInfoKHR swapchain_create_info(
            {},
            *surface_,
            image_count,
            surface_format.format,
            surface_format.colorSpace,
            extent,
            1,
            vk::ImageUsageFlagBits::eColorAttachment,
            different_queues ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
            different_queues ? 2 : 0,
            different_queues ? queue_family_indices_.data() : nullptr,
            swapchain_support.capabilities.currentTransform,
            vk::CompositeAlphaFlagBitsKHR::eOpaque,
            present_mode,
            {},
            *swapchain_
        );

        LEAF_AUTO_TO(swapchain_, to_result(device_->createSwapchainKHRUnique(swapchain_create_info)));
        LEAF_AUTO_TO(swapchain_images_, to_result(device_->getSwapchainImagesKHR(swapchain_.get())));
        swapchain_image_format_ = surface_format.format;
        swapchain_extent_ = extent;

        return {};
    }

    boost::leaf::result<vk::UniqueImageView> create_image_view(
        vk::Image image,
        vk::Format format,
        vk::ImageAspectFlags flags
    ) {
        vk::ComponentMapping component_mapping(
            vk::ComponentSwizzle::eR,
            vk::ComponentSwizzle::eG,
            vk::ComponentSwizzle::eB,
            vk::ComponentSwizzle::eA
        );
        vk::ImageSubresourceRange subresource_range(flags, 0, 1, 0, 1);
        vk::ImageViewCreateInfo image_view_create_info(
            vk::ImageViewCreateFlags(),
            image,
            vk::ImageViewType::e2D,
            format,
            component_mapping,
            subresource_range
        );

        return to_result(device_->createImageViewUnique(image_view_create_info));
    }

    boost::leaf::result<vma::UniqueImage> create_image(
        std::uint32_t width,
        std::uint32_t height,
        vk::Format format,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        VmaMemoryUsage memory_usage
    ) {
        auto info = vk::ImageCreateInfo(
            {},
            vk::ImageType::e2D,
            format,
            vk::Extent3D(width, height, 1),
            1,
            1,
            vk::SampleCountFlagBits::e1,
            tiling,
            usage,
            vk::SharingMode::eExclusive
        );

        VmaAllocationCreateInfo allocation_info = {};
        allocation_info.usage = memory_usage;

        return allocator_->create_image(info, allocation_info);
    }

    boost::leaf::result<void> init_image_views() {
        swapchain_image_views_.clear();
        swapchain_image_views_.reserve(swapchain_images_.size());

        for (auto& image: swapchain_images_) {
            LEAF_AUTO(image_view, create_image_view(image, swapchain_image_format_, vk::ImageAspectFlagBits::eColor));
            swapchain_image_views_.push_back(std::move(image_view));
        }

        return {};
    }

    boost::leaf::result<void> init_render_pass() {
        vk::AttachmentDescription color_attachment(
            {},
            swapchain_image_format_,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::ePresentSrcKHR
        );

        vk::AttachmentDescription depth_attachment(
            {},
            find_depth_format(),
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthStencilAttachmentOptimal
        );

        vk::AttachmentReference color_attachment_ref(0, vk::ImageLayout::eColorAttachmentOptimal);
        vk::AttachmentReference depth_attachment_ref(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

        vk::SubpassDescription subpass(
            {},
            vk::PipelineBindPoint::eGraphics,
            0,
            nullptr,
            1,
            &color_attachment_ref,
            nullptr,
            &depth_attachment_ref
        );

        vk::SubpassDependency color_dependency(
            VK_SUBPASS_EXTERNAL,
            0,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::AccessFlagBits::eMemoryRead,
            vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentWrite,
            vk::DependencyFlagBits::eByRegion
        );

        vk::SubpassDependency depth_dependency(
            0,
            VK_SUBPASS_EXTERNAL,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eMemoryRead,
            vk::DependencyFlagBits::eByRegion
        );

        std::array<vk::AttachmentDescription, 2> attachments = {color_attachment, depth_attachment};
        std::array<vk::SubpassDependency, 2> dependencies = {color_dependency, depth_dependency};

        vk::RenderPassCreateInfo render_pass_info(
            {},
            attachments.size(),
            attachments.data(),
            1,
            &subpass,
            dependencies.size(),
            dependencies.data()
        );

        LEAF_AUTO_TO(render_pass_, to_result(device_->createRenderPassUnique(render_pass_info)));

        return {};
    }

    boost::leaf::result<void> init_descriptor_set_layout() {
        auto layout_binding = vk::DescriptorSetLayoutBinding(
            0,
            vk::DescriptorType::eUniformBuffer,
            1,
            vk::ShaderStageFlagBits::eVertex,
            nullptr
        );

        auto info = vk::DescriptorSetLayoutCreateInfo({}, 1, &layout_binding);

        LEAF_AUTO_TO(descriptor_set_layout_, to_result(device_->createDescriptorSetLayoutUnique(info)));

        return {};
    }

    boost::leaf::result<vk::UniqueShaderModule> create_shader_module(const std::vector<char>& code) {
        vk::ShaderModuleCreateInfo create_info(
            {},
            code.size() * sizeof(char),
            reinterpret_cast<const std::uint32_t*>(code.data()));

        return to_result(device_->createShaderModuleUnique(create_info));
    }

    boost::leaf::result<vk::UniqueShaderModule> create_shader_module(
        const std::string& path
    ) {
        return create_shader_module(read_file(path));
    }

    boost::leaf::result<void> init_graphics_pipeline() {
        LEAF_AUTO(fragment_shader_module,
                  create_shader_module("shaders/shader.frag.spv"));
        LEAF_AUTO(vertex_shader_module,
                  create_shader_module("shaders/shader.vert.spv"));

        vk::PipelineShaderStageCreateInfo fragment_shader_stage_info(
            {},
            vk::ShaderStageFlagBits::eFragment,
            *fragment_shader_module,
            "main"
        );

        vk::PipelineShaderStageCreateInfo vertex_shader_stage_info(
            {},
            vk::ShaderStageFlagBits::eVertex,
            *vertex_shader_module,
            "main"
        );

        vk::PipelineShaderStageCreateInfo shader_stages[]{vertex_shader_stage_info, fragment_shader_stage_info};

        auto binding_description = Vertex::get_binding_description();
        auto attribute_descriptions = Vertex::get_attribute_descriptions();

        vk::PipelineVertexInputStateCreateInfo vertex_input_info(
            {},
            1,
            &binding_description,
            attribute_descriptions.size(),
            attribute_descriptions.data());

        vk::PipelineInputAssemblyStateCreateInfo input_assembly_info(
            {},
            vk::PrimitiveTopology::eTriangleList,
            false
        );

        vk::Viewport viewport(0.0f, 0.0f, swapchain_extent_.width, swapchain_extent_.height, 0.0f, 1.0f);
        vk::Rect2D scissor({0, 0}, swapchain_extent_);
        vk::PipelineViewportStateCreateInfo viewport_state(
            {},
            1,
            &viewport,
            1,
            &scissor
        );

        vk::PipelineRasterizationStateCreateInfo rasterizer(
            {},
            false,
            false,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack,
            vk::FrontFace::eClockwise,
            false,
            0.0f,
            0.0f,
            0.0f,
            1.0f
        );

        vk::PipelineMultisampleStateCreateInfo multisampling(
            {},
            vk::SampleCountFlagBits::e1,
            false,
            1.0f,
            nullptr,
            false,
            false
        );

        vk::PipelineColorBlendAttachmentState color_blend_attachment(
            false,
            vk::BlendFactor::eSrcAlpha,
            vk::BlendFactor::eOneMinusSrcAlpha,
            vk::BlendOp::eAdd,
            vk::BlendFactor::eOne,
            vk::BlendFactor::eZero,
            vk::BlendOp::eAdd,
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB
                | vk::ColorComponentFlagBits::eA
        );

        vk::PipelineColorBlendStateCreateInfo color_blending(
            {},
            false,
            vk::LogicOp::eCopy,
            1,
            &color_blend_attachment,
            {0.0f, 0.0f, 0.0f, 0.0f}
        );

        vk::PipelineLayoutCreateInfo layout_info(
            {},
            1,
            &descriptor_set_layout_.get(),
            0,
            nullptr
        );

        // TODO: stencil
        vk::PipelineDepthStencilStateCreateInfo depth_stencil({}, true, true, vk::CompareOp::eLess, false, false);

        LEAF_AUTO_TO(pipeline_layout_, to_result(device_->createPipelineLayoutUnique(layout_info)));

        vk::GraphicsPipelineCreateInfo pipeline_info(
            {},
            2,
            shader_stages,
            &vertex_input_info,
            &input_assembly_info,
            nullptr,
            &viewport_state,
            &rasterizer,
            &multisampling,
            &depth_stencil,
            &color_blending,
            nullptr,
            *pipeline_layout_,
            *render_pass_,
            0
        );

        LEAF_AUTO_TO(pipeline_, to_result(device_->createGraphicsPipelineUnique(nullptr, pipeline_info)));

        return {};
    }

    boost::leaf::result<void> init_framebuffers() {
        swapchain_framebuffers_.clear();
        swapchain_framebuffers_.reserve(swapchain_image_views_.size());

        for (auto const& view: swapchain_image_views_) {
            std::array<vk::ImageView, 2> attachments = {view.get(), depth_image_view_.get()};

            vk::FramebufferCreateInfo framebuffer_info(
                {},
                *render_pass_,
                attachments.size(),
                attachments.data(),
                swapchain_extent_.width,
                swapchain_extent_.height,
                1
            );

            LEAF_AUTO(framebuffer, to_result(device_->createFramebufferUnique(framebuffer_info)));
            swapchain_framebuffers_.push_back(std::move(framebuffer));
        }

        return {};
    }

    boost::leaf::result<void> init_command_pool() {
        LEAF_AUTO_TO(
            command_pool_,
            to_result(
                device_->createCommandPoolUnique(
                    vk::CommandPoolCreateInfo(
                        {},
                        graphics_queue_.index
                    ))));

        return {};
    }

    boost::leaf::result<void> init_depth_resources() {
        auto depth_format = find_depth_format();

        LEAF_AUTO_TO(
            depth_image_,
            create_image(
                swapchain_extent_.width,
                swapchain_extent_.height,
                depth_format,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eDepthStencilAttachment,
                VMA_MEMORY_USAGE_GPU_ONLY
            ));

        LEAF_AUTO_TO(
            depth_image_view_,
            create_image_view(
                depth_image_->get_image(),
                depth_format,
                vk::ImageAspectFlagBits::eStencil | vk::ImageAspectFlagBits::eDepth
            ));

        return {};
    }

    boost::leaf::result<void> init_command_buffers() {
//        auto command_buffers_count =
//        LEAF_AUTO(command_buffers, to_result(device_->allocateCommandBuffersUnique()))

        return {};
    }

    boost::leaf::result<vma::UniqueBuffer> create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage) {
        auto buffer_info = vk::BufferCreateInfo(
            {},
            size,
            usage,
            vk::SharingMode::eExclusive
        );

        VmaAllocationCreateInfo allocation_info = {};
        allocation_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

        return allocator_->create_buffer(buffer_info, allocation_info);
    }

    boost::leaf::result<void> request_vertex_buffer(vk::DeviceSize required_size) {
        if (vertex_buffer_ != nullptr && vertex_buffer_->get_size() >= required_size) return {};

        LEAF_AUTO_TO(vertex_buffer_, create_buffer(required_size, vk::BufferUsageFlagBits::eVertexBuffer));

        return {};
    }

private:
    std::shared_ptr<mff::window::Window> window_;
    vk::UniqueInstance instance_;
    vk::UniqueDebugUtilsMessengerEXT debug_utils_messenger_;
    std::vector<std::string> instance_layers_;
    std::vector<std::string> instance_extensions_;
    vk::UniqueSurfaceKHR surface_;
    vk::PhysicalDevice physical_device_;
    std::vector<std::string> device_extensions_ = std::vector<std::string>{{VK_KHR_SWAPCHAIN_EXTENSION_NAME}};
    vk::UniqueDevice device_;
    vma::UniqueAllocator allocator_;
    Queue graphics_queue_;
    Queue present_queue_;
    std::vector<std::uint32_t> queue_family_indices_;
    vk::UniqueSwapchainKHR swapchain_;
    std::vector<vk::Image> swapchain_images_;
    vk::Format swapchain_image_format_;
    vk::Extent2D swapchain_extent_;
    std::vector<vk::UniqueImageView> swapchain_image_views_;
    vk::UniqueRenderPass render_pass_;
    vk::UniqueDescriptorSetLayout descriptor_set_layout_;
    vk::UniquePipelineLayout pipeline_layout_;
    vk::UniquePipeline pipeline_;
    std::vector<vk::UniqueFramebuffer> swapchain_framebuffers_;
    vk::UniqueCommandPool command_pool_;
    vma::UniqueBuffer vertex_buffer_;
    vma::UniqueImage depth_image_;
    vk::UniqueImageView depth_image_view_;
};

