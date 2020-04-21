#include "./sync.h"

#include <mff/algorithms.h>

namespace mff::vulkan {

SharingMode get_sharing_mode(const std::vector<QueueFamily>& queue_families) {
    std::vector<std::uint32_t> uniq_families; // = mff::map([](auto family) { return family.get_index(); }, queue_families);

    for (const auto& queue_family: queue_families) {
        if (!mff::contains(uniq_families, queue_family.get_index())) uniq_families.push_back(queue_family.get_index());
    }

    if (uniq_families.size() > 1) {
        return ConcurrentSharingMode{uniq_families};
    }

    return ExclusiveSharingMode();
}

}
