#include "./vulkan_engine.h"

#include <mff/algorithms.h>

bool QueueFamilyIndices::is_complete() {
    return graphics_family.has_value() && present_family.has_value() && transfer_family.has_value()
        && compute_family.has_value();
}

std::vector<const mff::vulkan::QueueFamily*> QueueFamilyIndices::to_vector() const {
    return {
        graphics_family.value(),
        present_family.value(),
        transfer_family.value(),
        compute_family.value()
    };
}

Queues Queues::from_vector(const std::vector<mff::vulkan::SharedQueue>& queues_vec) {
    Queues queues = {};

    queues.graphics_queue = queues_vec[0];
    queues.present_queue = queues_vec[1];
    queues.transfer_queue = queues_vec[2];
    queues.compute_queue = queues_vec[3];

    return queues;
}

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

boost::leaf::result<std::unique_ptr<VulkanEngine>> VulkanEngine::build(
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


const mff::vulkan::Instance* VulkanEngine::get_instance() const {
    return instance_.get();
}

const mff::vulkan::Surface* VulkanEngine::get_surface() const {
    return surface_.get();
}

mff::vulkan::Device* VulkanEngine::get_device() {
    return device_.get();
}

const mff::vulkan::Device* VulkanEngine::get_device() const {
    return device_.get();
}

const vma::Allocator* VulkanEngine::get_allocator() const {
    return device_->get_allocator();
}

const std::shared_ptr<mff::window::Window>& VulkanEngine::get_window() const {
    return window_;
}

const Queues& VulkanEngine::get_queues() const {
    return queues_;
}