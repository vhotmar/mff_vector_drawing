#include <mff/graphics/vulkan/sync/semaphore.h>

#include <mff/graphics/utils.h>
#include <mff/graphics/vulkan/instance.h>

namespace mff::vulkan {

boost::leaf::result<UniqueSemaphore> Semaphore::build(const Device* device) {
    struct enable_Sempahore : public Semaphore {};
    UniqueSemaphore result = std::make_unique<enable_Sempahore>();

    result->device_ = device;

    vk::SemaphoreCreateInfo info = {};

    LEAF_AUTO_TO(
        result->handle_,
        to_result(device->get_handle().createSemaphoreUnique(info)));

    return result;
}

vk::Semaphore Semaphore::get_handle() const {
    return handle_.get();
}

boost::leaf::result<UniquePooledSemaphore> Semaphore::from_pool(Device* device) {
    LEAF_AUTO(result, device->get_semaphore_pool()->acquire());

    return std::move(result);
}

}