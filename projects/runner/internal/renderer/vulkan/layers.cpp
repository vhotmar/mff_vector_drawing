#include "layers.h"

#include "../../constants.h"

namespace mff::internal::renderer::vulkan {

std::set<std::string> get_required_mff_layers() {
    if (constants::kVULKAN_DEBUG) {
        return {"VK_LAYER_LUNARG_standard_validation"};
    }

    return {};
}

}