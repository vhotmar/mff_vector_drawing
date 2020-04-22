#include "device.h"

#include <mff/algorithms.h>
#include <unordered_map>
#include <utility>

#include "../utils.h"
#include "./dispatcher.h"

namespace mff::vulkan {

boost::leaf::result<std::tuple<std::shared_ptr<Device>, std::vector<std::shared_ptr<Queue>>>> Device::build(
    const std::shared_ptr<PhysicalDevice>& physical_device,
    const std::vector<QueueFamily>& queue_families,
    const std::vector<std::string>& extensions
) {
    auto instance = physical_device->get_instance();
    auto layers = instance->get_loaded_layers();
    auto layers_c = utils::to_pointer_char_data(layers);

    // TODO: check whether the extension is supported
    auto extensions_c = utils::to_pointer_char_data(extensions);

    std::vector<QueueFamily> uniq_families;
    std::vector<std::size_t> original_to_uniq;
    original_to_uniq.reserve(queue_families.size());

    for (auto& queue_family: queue_families) {
        // TODO: check whether the queue family is from correct physical device
        auto it = mff::find(uniq_families, queue_family);

        if (!it) {
            uniq_families.push_back(queue_family);
            original_to_uniq.emplace_back(uniq_families.size() - 1);
        } else {
            original_to_uniq.emplace_back(std::distance(uniq_families.cbegin(), *it));
        }
    }

    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
    std::vector<std::vector<float>> priorities_owner;
    priorities_owner.reserve(uniq_families.size());

    for (const auto& queue_family: uniq_families) {
        priorities_owner.emplace_back(std::vector<float>(queue_family.get_queues_count(), 0.5f));
        queue_create_infos.push_back(
            vk::DeviceQueueCreateInfo(
                {},
                queue_family.get_index(),
                priorities_owner.back().size(),
                priorities_owner.back().data()));
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
    std::shared_ptr<Device> device = std::make_shared<enable_Device>();

    device->physical_device_ = physical_device;
    device->layers_ = layers;
    device->extensions_ = extensions;

    LEAF_AUTO_TO(device->handle_, to_result(physical_device->get_handle().createDeviceUnique(device_create_info)));

    init_dispatcher(device->handle_.get());

    std::vector<std::shared_ptr<Queue>> uniq_queues = mff::map(
        [&](auto queue_family) {
            auto count = queue_family.get_queues_count();
            std::vector<vk::Queue> queues;
            queues.reserve(count);

            for (std::size_t i = 0; i < count; i++) {
                queues.push_back(device->handle_->getQueue(queue_family.get_index(), i));
            }

            struct enable_Queue : public Queue {};
            std::shared_ptr<Queue> queue = std::make_shared<enable_Queue>();

            queue->device_ = device;
            queue->queue_family_ = queue_family;
            queue->queues_ = std::move(queues);

            return queue;
        },
        uniq_families
    );

    std::vector<std::shared_ptr<Queue>> output_queues;
    output_queues.reserve(original_to_uniq.size());

    for (auto index: original_to_uniq) {
        output_queues.push_back(uniq_queues[index]);
    }

    return std::make_tuple(std::move(device), std::move(output_queues));
}

std::shared_ptr<PhysicalDevice> Device::get_physical_device() const {
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

}