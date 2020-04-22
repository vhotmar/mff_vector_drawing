#include "./sync.h"

#include <mff/algorithms.h>

namespace mff::vulkan {

SharingMode get_sharing_mode(const std::vector<std::shared_ptr<QueueFamily>>& queue_families) {
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
