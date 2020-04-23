#include <mff/graphics/vulkan/framebuffer/desc.h>

namespace mff::vulkan {

vk::AttachmentDescription AttachmentDescription::to_vulkan() const {
    return vk::AttachmentDescription(
        {},
        format,
        samples,
        load,
        store,
        stencil_load,
        stencil_store,
        initial_layout,
        final_layout
    );
}

vk::SubpassDependency SubpassDependency::to_vulkan() const {
    return vk::SubpassDependency(
        source_subpass,
        destination_subpass,
        source_stages,
        destination_stages,
        source_access,
        destination_access,
        by_region ? vk::DependencyFlagBits::eByRegion : vk::DependencyFlags{}
    );
}

}