#include <mff/graphics/vulkan/device.h>

#include <unordered_map>
#include <utility>

#include <range/v3/all.hpp>
#include <mff/graphics/memory.h>
#include <mff/algorithms.h>
#include <mff/graphics/utils.h>
#include <mff/graphics/vulkan/dispatcher.h>

namespace mff::vulkan {

boost::leaf::result<std::tuple<UniqueDevice, std::vector<SharedQueue>>> Device::build(
    const PhysicalDevice* physical_device,
    const std::vector<const QueueFamily*>& queue_families,
    const std::vector<std::string>& extensions
) {
    auto instance = physical_device->get_instance();

    // we need to forward the layers from instance (legacy reasons vulkan docs say)
    auto layers = instance->get_loaded_layers();
    auto layers_c = utils::to_pointer_char_data(layers);

    // TODO (vhotmar): better error handling - check whether the extension is supported
    auto extensions_c = utils::to_pointer_char_data(extensions);

    std::vector<const QueueFamily*> uniq_families;
    std::vector<std::size_t> original_index_to_uniq_index;
    original_index_to_uniq_index.reserve(queue_families.size());

    for (auto& queue_family: queue_families) {
        // TODO (vhotmar): better error handling - check whether the queue family is from correct physical device
        auto it = mff::find(uniq_families, queue_family);

        if (!it) {
            uniq_families.push_back(queue_family);
            original_index_to_uniq_index.emplace_back(uniq_families.size() - 1);
        } else {
            original_index_to_uniq_index.emplace_back(std::distance(uniq_families.cbegin(), *it));
        }
    }

    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
    std::vector<std::vector<float>> queue_priorities;
    queue_priorities.reserve(uniq_families.size());

    for (const auto& queue_family: uniq_families) {
        queue_priorities.emplace_back(std::vector<float>(queue_family->get_queues_count(), 1.0f));
        queue_create_infos.push_back(
            vk::DeviceQueueCreateInfo(
                {},
                queue_family->get_index(),
                queue_priorities.back().size(),
                queue_priorities.back().data()));
    }

    auto device_create_info = vk::DeviceCreateInfo(
        {},
        queue_create_infos.size(),
        queue_create_infos.data(),
        layers_c.size(),
        layers_c.data(),
        extensions_c.size(),
        extensions_c.data());

    struct enable_Device : public Device {};
    std::unique_ptr<Device> device = std::make_unique<enable_Device>();

    device->instance_ = instance;
    device->physical_device_ = physical_device;
    device->layers_ = layers;
    device->extensions_ = extensions;

    LEAF_AUTO_TO(device->handle_, to_result(physical_device->get_handle().createDeviceUnique(device_create_info)));

    init_dispatcher(device->handle_.get());

    std::vector<std::shared_ptr<Queue>> uniq_queues = uniq_families
        | ranges::views::transform(
            [&](const auto& queue_family) {
                auto count = queue_family->get_queues_count();
                std::vector<vk::Queue> queues;
                queues.reserve(count);

                for (std::size_t i = 0; i < count; i++) {
                    queues.push_back(device->handle_->getQueue(queue_family->get_index(), i));
                }

                struct enable_Queue : public Queue {};
                std::shared_ptr<Queue> queue = std::make_shared<enable_Queue>();

                queue->queue_family_ = queue_family;
                queue->device_ = device.get();
                queue->queues_ = std::move(queues);

                return std::move(queue);
            }
        )
        | ranges::to<std::vector>();

    std::vector<std::shared_ptr<Queue>> output_queues;
    output_queues.reserve(original_index_to_uniq_index.size());

    for (auto index: original_index_to_uniq_index) {
        output_queues.push_back(uniq_queues[index]);
    }

    LEAF_AUTO_TO(device->allocator_, vma::Allocator::build(device.get()));
    device->semaphores_pool_ = std::make_unique<ObjectPool<mff::vulkan::Semaphore>>(
        [&]() {
            assert(false);
            return mff::vulkan::Semaphore::build(device.get());
        }
    );

    device->fences_pool_ = std::make_unique<ObjectPool<mff::vulkan::Fence>>(
        [&]() {
            assert(false);
            return mff::vulkan::Fence::build(device.get(), false);
        }
    );

    return std::make_tuple(std::move(device), std::move(output_queues));
}

const PhysicalDevice* Device::get_physical_device() const {
    return physical_device_;
}

const std::vector<std::string>& Device::get_layers() const {
    return layers_;
}

const std::vector<std::string>& Device::get_extensions() const {
    return extensions_;
}

vk::Device Device::get_handle() const {
    return handle_.get();
}

boost::leaf::result<CommandPool*> Device::get_command_pool(const QueueFamily* queue_family) {
    auto id = queue_family->get_index();

    if (!mff::has(command_pools_, id)) {
        LEAF_AUTO(command_pool, CommandPool::build(this, queue_family));

        command_pools_.emplace(id, std::move(command_pool));
    }

    return command_pools_[id].get();
}

const vma::Allocator* Device::get_allocator() const {
    return allocator_.get();
}

mff::ObjectPool<mff::vulkan::Semaphore>* Device::get_semaphore_pool() {
    return semaphores_pool_.get();
}

mff::ObjectPool<mff::vulkan::Fence>* Device::get_fence_pool() {
    return fences_pool_.get();
}

const vk::Queue& Queue::get_handle() const {
    return queues_.front();
}

const QueueFamily* Queue::get_queue_family() const {
    return queue_family_;
}

}