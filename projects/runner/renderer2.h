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
#include <mff/graphics/vulkan/swapchain.h>
#include <mff/graphics/vulkan/vulkan.h>
#include <mff/graphics/window.h>

#include "./utils/logger.h"

//////////////////////
/// Queue families ///
//////////////////////

struct QueueFamilyIndices {
    std::optional<const mff::vulkan::QueueFamily*> graphics_family = std::nullopt;
    std::optional<const mff::vulkan::QueueFamily*> present_family = std::nullopt;
    std::optional<const mff::vulkan::QueueFamily*> transfer_family = std::nullopt;
    std::optional<const mff::vulkan::QueueFamily*> compute_family = std::nullopt;

    bool is_complete() {
        return graphics_family.has_value() && present_family.has_value() && transfer_family.has_value()
            && compute_family.has_value();
    }

    std::vector<const mff::vulkan::QueueFamily*> to_vector() const {
        return {
            graphics_family.value(),
            present_family.value(),
            transfer_family.value(),
            compute_family.value()
        };
    }
};

struct Queues {
    mff::vulkan::SharedQueue graphics_queue = nullptr;
    mff::vulkan::SharedQueue present_queue = nullptr;
    mff::vulkan::SharedQueue transfer_queue = nullptr;
    mff::vulkan::SharedQueue compute_queue = nullptr;

    static Queues from_vector(const std::vector<mff::vulkan::SharedQueue>& queues_vec) {
        Queues queues = {};

        queues.graphics_queue = queues_vec[0];
        queues.present_queue = queues_vec[1];
        queues.transfer_queue = queues_vec[2];
        queues.compute_queue = queues_vec[3];

        return queues;
    }
};

QueueFamilyIndices find_queue_families(
    const mff::vulkan::PhysicalDevice* physical_device,
    const mff::vulkan::Surface* surface
) {
    QueueFamilyIndices indices;

    auto queue_families = physical_device->get_queue_families();

    for (const auto& family: queue_families) {
        if (family->supports_graphics()) {
            indices.graphics_family = family;
        }

        if (family->supports_compute()) {
            indices.compute_family = family;
        }

        if (family->supports_transfer()) {
            indices.transfer_family = family;
        }

        if (surface->is_supported(family)) {
            indices.present_family = family;
        }

        if (indices.is_complete()) {
            break;
        }
    }

    return indices;
}

///////////////////////
/// Physical device ///
///////////////////////

bool is_device_suitable(
    const mff::vulkan::PhysicalDevice* physical_device,
    const mff::vulkan::Surface* surface,
    const std::vector<std::string>& required_extensions
) {
    bool is_discrete = physical_device->get_type() == vk::PhysicalDeviceType::eDiscreteGpu;
    bool has_queue_families = find_queue_families(physical_device, surface).is_complete();
    bool required_extensions_supported = physical_device->are_extensions_supported(required_extensions);

    bool swapchain_adequate = ([&]() {
        auto capabilities_result = surface->get_capabilities(physical_device);

        if (capabilities_result) {
            auto details = capabilities_result.value();
            return !details.supported_formats.empty() && !details.present_modes.empty();
        }

        return false;
    })();

    return is_discrete
        && has_queue_families
        && required_extensions_supported
        && swapchain_adequate;
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

////////////////////////
/// Vulkan utilities ///
////////////////////////

/**
 * Get stencil format which is supported by physical deice
 * @param physical_device
 * @return the best stencil format we want or std::nullopt
 */
std::optional<vk::Format> find_stencil_format(
    const mff::vulkan::PhysicalDevice* physical_device
) {
    return physical_device->find_supported_format(
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
    auto stencil_format = std::make_optional(vk::Format::eS8Uint);// find_stencil_format(physical_device);
    if (!stencil_format) return LEAF_NEW_ERROR();

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

    return mff::to_result(device.createRenderPassUnique(render_pass_info));
}

/**
 * This class provides us with resources which do not change really
 */
class VulkanEngine {
public:
    static boost::leaf::result<std::unique_ptr<VulkanEngine>> build(
        const std::shared_ptr<mff::window::Window>& window
    ) {
        struct enable_VulkanEngine : public VulkanEngine {};
        std::unique_ptr<VulkanEngine> engine = std::make_unique<enable_VulkanEngine>();

        engine->window_ = window;

        LEAF_AUTO_TO(engine->instance_,
                     mff::vulkan::Instance::build(std::nullopt, window->get_required_extensions(), {}));
        LEAF_AUTO_TO(engine->surface_, mff::vulkan::Surface::build(window, engine->instance_.get()));

        engine->physical_device_ = *(mff::find_if(
            engine->instance_->get_physical_devices(),
            [&](auto device) { return is_device_suitable(device, engine->surface_.get(), {}); }
        ).value());

        auto queue_indices = find_queue_families(engine->physical_device_, engine->surface_.get());

        LEAF_AUTO(
            device_result,
            mff::vulkan::Device::build(
                engine->physical_device_,
                queue_indices.to_vector(),
                {VK_KHR_SWAPCHAIN_EXTENSION_NAME}
            ));

        std::vector<mff::vulkan::SharedQueue> queues_vec;
        std::tie(engine->device_, queues_vec) = std::move(device_result);

        engine->queues_ = Queues::from_vector(queues_vec);

        return std::move(engine);
    }

    const mff::vulkan::Instance* get_instance() const {
        return instance_.get();
    }

    const mff::vulkan::Surface* get_surface() const {
        return surface_.get();
    }

    mff::vulkan::Device* get_device() {
        return device_.get();
    }

    const mff::vulkan::Device* get_device() const {
        return device_.get();
    }

    const vma::Allocator* get_allocator() const {
        return device_->get_allocator();
    }

    const std::shared_ptr<mff::window::Window>& get_window() const {
        return window_;
    }

    const Queues& get_queues() const {
        return queues_;
    }

private:
    VulkanEngine() = default;

    /**
     * Window on which we will initialize the vulkan context + draw (no multi window for now)
     */
    std::shared_ptr<mff::window::Window> window_ = nullptr;

    mff::vulkan::UniqueInstance instance_ = nullptr;
    mff::vulkan::UniqueSurface surface_ = nullptr;
    const mff::vulkan::PhysicalDevice* physical_device_ = nullptr;
    mff::vulkan::UniqueDevice device_ = nullptr;

    Queues queues_ = {};

    std::vector<std::uint32_t> queue_family_indices_;
};

class Presenter {
public:
    static boost::leaf::result<std::unique_ptr<Presenter>> build(VulkanEngine* engine) {
        struct enable_Presenter : public Presenter {};
        std::unique_ptr<Presenter> result = std::make_unique<enable_Presenter>();

        result->window_ = engine->get_window();
        result->present_queue_ = engine->get_queues().present_queue;
        result->device_ = engine->get_device();
        result->surface_ = engine->get_surface();

        LEAF_AUTO_TO(result->present_end_semaphore_, mff::vulkan::Semaphore::build(engine->get_device()));
        LEAF_AUTO_TO(result->draw_end_semaphore_, mff::vulkan::Semaphore::build(engine->get_device()));

        result->build_swapchain();

        return result;
    }

private:
    Presenter() = default;

    boost::leaf::result<void> build_swapchain() {
        LEAF_AUTO(capabilities, surface_->get_capabilities(device_->get_physical_device()));

        LEAF_CHECK_OPTIONAL(
            surface_format,
            capabilities.find_format({{vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}}));
        LEAF_CHECK_OPTIONAL(
            present_mode,
            capabilities.find_present_mode({vk::PresentModeKHR::eMailbox}));
        auto extent = capabilities.normalize_extent(window_->get_inner_size());

        auto image_count = capabilities.min_image_count + 1;

        if (capabilities.max_image_count && image_count > capabilities.max_image_count.value()) {
            image_count = capabilities.max_image_count.value();
        }

        LEAF_AUTO(
            swapchain_result,
            mff::vulkan::Swapchain::build(
                device_,
                surface_,
                image_count,
                surface_format.format,
                extent,
                1,
                capabilities.supported_usage_flags | vk::ImageUsageFlags(vk::ImageUsageFlagBits::eColorAttachment),
                mff::vulkan::SharingMode_::Exclusive{}, // we will use only present queue with swapchain
                capabilities.current_transform,
                vk::CompositeAlphaFlagBitsKHR::eOpaque,
                present_mode,
                true,
                surface_format.colorSpace,
                swapchain_ ? std::make_optional(swapchain_.get()) : std::nullopt
            ));
        std::tie(swapchain_, swapchain_images_) = std::move(swapchain_result);

/*        LEAF_AUTO(
            sample,
            mff::vulkan::AutoCommandBufferBuilder::build_primary(device_, present_queue_->get_queue_family()));
*/
        return {};
    }

    std::shared_ptr<mff::window::Window> window_ = nullptr;
    mff::vulkan::SharedQueue present_queue_ = nullptr;
    mff::vulkan::Device* device_ = nullptr;
    const mff::vulkan::Surface* surface_ = nullptr;
    mff::vulkan::UniqueSemaphore present_end_semaphore_ = nullptr;
    mff::vulkan::UniqueSemaphore draw_end_semaphore_ = nullptr;
    mff::vulkan::UniqueSwapchain swapchain_ = nullptr;
    std::vector<mff::vulkan::UniqueSwapchainImage> swapchain_images_ = {};

    // mff::vulkan::

};
