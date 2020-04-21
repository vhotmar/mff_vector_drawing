#pragma once

#include "../../leaf.h"
#include "../../vulkan.h"

#include "./instance.h"

namespace mff::vulkan {

class Device;

class Queue {
private:
    std::vector<vk::Queue> queues_;
    std::shared_ptr<Device> device_;
    QueueFamily queue_family_;

public:
    Queue(
        std::shared_ptr<Device> device,
        QueueFamily queue_family,
        std::vector<vk::Queue> queues
    );
};

enum class create_device_error_code {
    priority_out_of_range_error,
    too_many_queues_for_family_error
};

class Device {
private:
    std::shared_ptr<Instance> instance_;
    std::shared_ptr<PhysicalDevice> physical_device_;
    vk::UniqueDevice handle_;
    std::vector<std::string> layers_;
    std::vector<std::string> extensions_;

    Device(std::shared_ptr<PhysicalDevice> physical_device);

public:
    std::shared_ptr<PhysicalDevice> get_physical_device() const;

    const std::vector<std::string>& get_layers() const;

    const std::vector<std::string>& get_extensions() const;

    vk::Device get_handle() const;

    static boost::leaf::result<std::tuple<std::shared_ptr<Device>, std::vector<std::shared_ptr<Queue>>>> build(
        const std::shared_ptr<PhysicalDevice>& physical_device,
        const std::vector<QueueFamily>& queue_families,
        const std::vector<std::string>& extensions
    );
};

}

namespace boost::leaf {

template <>
struct is_e_type<mff::vulkan::create_device_error_code> : public std::true_type {};

}