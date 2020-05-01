#include <mff/graphics/vulkan/sync.h>

#include <mff/algorithms.h>
#include <mff/graphics/utils.h>
#include <mff/graphics/vulkan/instance.h>

namespace mff::vulkan {

SharingMode get_sharing_mode(const std::vector<const QueueFamily*>& queue_families) {
    std::vector<std::uint32_t> uniq_families;

    for (const auto& queue_family: queue_families) {
        if (!mff::contains(uniq_families, queue_family->get_index())) {
            uniq_families.push_back(queue_family->get_index());
        }
    }

    if (uniq_families.size() > 1) {
        return SharingMode_::Concurrent{uniq_families};
    }

    return SharingMode_::Exclusive();
}

boost::leaf::result<UniqueSemaphore> Semaphore::build(const Device* device) {
    struct enable_Sempahore : public Semaphore {};
    UniqueSemaphore result = std::make_unique<enable_Sempahore>();

    result->device_ = device;
    LEAF_AUTO_TO(
        result->handle_,
        to_result(device->get_handle().createSemaphoreUnique(vk::SemaphoreCreateInfo())));

    return result;
}

}
