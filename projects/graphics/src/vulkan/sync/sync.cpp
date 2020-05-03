#include <mff/graphics/vulkan/sync/sync.h>

#include <mff/algorithms.h>
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

}
