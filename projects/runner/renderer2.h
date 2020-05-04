#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <range/v3/all.hpp>
#include <mff/leaf.h>
#include <mff/algorithms.h>
#include <mff/graphics/math.h>
#include <mff/graphics/memory.h>
#include <mff/graphics/vulkan/dispatcher.h>
#include <mff/graphics/vulkan/instance.h>
#include <mff/graphics/vulkan/swapchain.h>
#include <mff/graphics/vulkan/vulkan.h>
#include <mff/graphics/window.h>
#include <mff/graphics/vulkan/command_buffer/builders/unsafe.h>
#include <mff/graphics/vulkan/framebuffer/framebuffer.h>
#include <mff/graphics/vulkan/pipeline/raster.h>
#include <mff/graphics/vulkan/pipeline/blend.h>

#include "./third_party/earcut.hpp"
#include "./utils/logger.h"

const std::uint32_t kSTENCIL_CLIP_BIT = 0x1;
const std::uint32_t kSTENCIL_FILL_BIT = 0x2;
const std::uint32_t kSTENCIL_ALL_BIT = 0x3;

//////////////////////
/// Queue families ///
//////////////////////

struct QueueFamilyIndices {
    std::optional<const mff::vulkan::QueueFamily*> graphics_family = std::nullopt;
    std::optional<const mff::vulkan::QueueFamily*> present_family = std::nullopt;
    std::optional<const mff::vulkan::QueueFamily*> transfer_family = std::nullopt;
    std::optional<const mff::vulkan::QueueFamily*> compute_family = std::nullopt;

    bool is_complete() {
        return graphics_family.has_value() && present_family.has_value() && transfer_family.has_value()
            && compute_family.has_value();
    }

    std::vector<const mff::vulkan::QueueFamily*> to_vector() const {
        return {
            graphics_family.value(),
            present_family.value(),
            transfer_family.value(),
            compute_family.value()
        };
    }
};

struct Queues {
    mff::vulkan::SharedQueue graphics_queue = nullptr;
    mff::vulkan::SharedQueue present_queue = nullptr;
    mff::vulkan::SharedQueue transfer_queue = nullptr;
    mff::vulkan::SharedQueue compute_queue = nullptr;

    static Queues from_vector(const std::vector<mff::vulkan::SharedQueue>& queues_vec) {
        Queues queues = {};

        queues.graphics_queue = queues_vec[0];
        queues.present_queue = queues_vec[1];
        queues.transfer_queue = queues_vec[2];
        queues.compute_queue = queues_vec[3];

        return queues;
    }
};

QueueFamilyIndices find_queue_families(
    const mff::vulkan::PhysicalDevice* physical_device,
    const mff::vulkan::Surface* surface
) {
    QueueFamilyIndices indices;

    auto queue_families = physical_device->get_queue_families();

    for (const auto& family: queue_families) {
        if (family->supports_graphics()) {
            indices.graphics_family = family;
        }

        if (family->supports_compute()) {
            indices.compute_family = family;
        }

        if (family->supports_transfer()) {
            indices.transfer_family = family;
        }

        if (surface->is_supported(family)) {
            indices.present_family = family;
        }

        if (indices.is_complete()) {
            break;
        }
    }

    return indices;
}

///////////////////////
/// Physical device ///
///////////////////////

bool is_device_suitable(
    const mff::vulkan::PhysicalDevice* physical_device,
    const mff::vulkan::Surface* surface,
    const std::vector<std::string>& required_extensions
) {
    bool is_discrete = physical_device->get_type() == vk::PhysicalDeviceType::eDiscreteGpu;
    bool has_queue_families = find_queue_families(physical_device, surface).is_complete();
    bool required_extensions_supported = physical_device->are_extensions_supported(required_extensions);

    bool swapchain_adequate = ([&]() {
        auto capabilities_result = surface->get_capabilities(physical_device);

        if (capabilities_result) {
            auto details = capabilities_result.value();
            return !details.supported_formats.empty() && !details.present_modes.empty();
        }

        return false;
    })();

    return is_discrete
        && has_queue_families
        && required_extensions_supported
        && swapchain_adequate;
}

////////////////////////////
/// Vertex Input Structs ///
////////////////////////////

struct Vertex {
    mff::Vector2f pos;

    static vk::VertexInputBindingDescription get_binding_description(std::uint32_t binding = 0) {
        vk::VertexInputBindingDescription bindingDescription(binding, sizeof(Vertex), vk::VertexInputRate::eVertex);

        return bindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 1> get_attribute_descriptions() {
        return {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos)),
        };
    }
};

struct UBO {
    mff::Vector4f color;
    std::float_t scale;
};

struct PushConstants {
    std::float_t scale;
};


boost::leaf::result<vk::UniqueShaderModule> create_shader_module(
    const mff::vulkan::Device* device,
    const std::vector<char>& code
) {
    vk::ShaderModuleCreateInfo create_info(
        {},
        code.size() * sizeof(char),
        reinterpret_cast<const std::uint32_t*>(code.data()));

    return mff::to_result(device->get_handle().createShaderModuleUnique(create_info));
}

boost::leaf::result<vk::UniqueShaderModule> create_shader_module(
    const mff::vulkan::Device* device,
    const std::string& path
) {
    return create_shader_module(device, mff::read_file(path));
}


////////////////////////
/// Vulkan utilities ///
////////////////////////

/**
 * Get stencil format which is supported by physical deice
 * @param physical_device
 * @return the best stencil format we want or std::nullopt
 */
std::optional<vk::Format> find_stencil_format(
    const mff::vulkan::PhysicalDevice* physical_device
) {
    return physical_device->find_supported_format(
        {vk::Format::eS8Uint},
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

/**
 * This class provides us with resources which do not change really (and this can be reused...)
 */
class VulkanBaseEngine {
public:
    static boost::leaf::result<std::unique_ptr<VulkanBaseEngine>> build(
        const std::shared_ptr<mff::window::Window>& window
    ) {
        struct enable_VulkanEngine : public VulkanBaseEngine {};
        std::unique_ptr<VulkanBaseEngine> engine = std::make_unique<enable_VulkanEngine>();

        engine->window_ = window;

        LEAF_AUTO_TO(engine->instance_,
                     mff::vulkan::Instance::build(std::nullopt, window->get_required_extensions(), {}));
        LEAF_AUTO_TO(engine->surface_, mff::vulkan::Surface::build(window, engine->instance_.get()));

        engine->physical_device_ = *(mff::find_if(
            engine->instance_->get_physical_devices(),
            [&](auto device) { return is_device_suitable(device, engine->surface_.get(), {}); }
        ).value());

        auto queue_indices = find_queue_families(engine->physical_device_, engine->surface_.get());

        LEAF_AUTO(
            device_result,
            mff::vulkan::Device::build(
                engine->physical_device_,
                queue_indices.to_vector(),
                {VK_KHR_SWAPCHAIN_EXTENSION_NAME}
            ));

        std::vector<mff::vulkan::SharedQueue> queues_vec;
        std::tie(engine->device_, queues_vec) = std::move(device_result);

        engine->queues_ = Queues::from_vector(queues_vec);

        return std::move(engine);
    }

    const mff::vulkan::Instance* get_instance() const {
        return instance_.get();
    }

    const mff::vulkan::Surface* get_surface() const {
        return surface_.get();
    }

    mff::vulkan::Device* get_device() {
        return device_.get();
    }

    const mff::vulkan::Device* get_device() const {
        return device_.get();
    }

    const vma::Allocator* get_allocator() const {
        return device_->get_allocator();
    }

    const std::shared_ptr<mff::window::Window>& get_window() const {
        return window_;
    }

    const Queues& get_queues() const {
        return queues_;
    }

private:
    VulkanBaseEngine() = default;

    /**
     * Window on which we will initialize the vulkan context + draw (no multi window for now)
     */
    std::shared_ptr<mff::window::Window> window_ = nullptr;

    mff::vulkan::UniqueInstance instance_ = nullptr;
    mff::vulkan::UniqueSurface surface_ = nullptr;
    const mff::vulkan::PhysicalDevice* physical_device_ = nullptr;
    mff::vulkan::UniqueDevice device_ = nullptr;

    Queues queues_ = {};

    std::vector<std::uint32_t> queue_family_indices_;
};

class Presenter {
public:
    static boost::leaf::result<std::unique_ptr<Presenter>> build(VulkanBaseEngine* engine) {
        struct enable_Presenter : public Presenter {};
        std::unique_ptr<Presenter> result = std::make_unique<enable_Presenter>();

        result->window_ = engine->get_window();
        result->present_queue_ = engine->get_queues().present_queue;
        result->device_ = engine->get_device();
        result->surface_ = engine->get_surface();

        LEAF_AUTO_TO(result->present_end_semaphore_, mff::vulkan::Semaphore::build(engine->get_device()));
        LEAF_AUTO_TO(result->draw_end_semaphore_, mff::vulkan::Semaphore::build(engine->get_device()));

        result->build_swapchain();

        return result;
    }

    const mff::vulkan::Swapchain* get_swapchain() const {
        return swapchain_.get();
    }

    boost::leaf::result<void> build_commands(
        const mff::vulkan::Image* source,
        std::array<std::uint32_t, 2> dimensions
    ) {
        std::size_t index = 0;
        for (const auto& alloc: command_buffer_allocations_) {
            LEAF_AUTO(builder, mff::vulkan::UnsafeCommandBufferBuilder::from_buffer(alloc->get_handle(), {}));

            auto destination = swapchain_images_[index]->get_image_impl();

            // TODO: should be automated using SyncCommandBufferBuilder
            builder->pipeline_barrier(
                mff::vulkan::UnsafeCommandBufferBuilderPipelineBarrier()
                    .add_image_memory_barrier(
                        destination,
                        0,
                        1,
                        0,
                        1,
                        vk::PipelineStageFlagBits::eBottomOfPipe,
                        {},
                        vk::PipelineStageFlagBits::eTransfer,
                        vk::AccessFlagBits::eTransferWrite,
                        true,
                        std::nullopt,
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eTransferDstOptimal
                    )
                    .add_image_memory_barrier(
                        source,
                        0,
                        1,
                        0,
                        1,
                        vk::PipelineStageFlagBits::eColorAttachmentOutput,
                        vk::AccessFlagBits::eColorAttachmentWrite,
                        vk::PipelineStageFlagBits::eTransfer,
                        vk::AccessFlagBits::eTransferRead,
                        true,
                        std::nullopt,
                        vk::ImageLayout::eColorAttachmentOptimal,
                        vk::ImageLayout::eTransferSrcOptimal
                    ));

            builder->copy_image(
                source,
                vk::ImageLayout::eTransferSrcOptimal,
                destination,
                vk::ImageLayout::eTransferDstOptimal,
                {mff::vulkan::UnsafeCommandBufferBuilderImageCopy{
                    {true, false, false},
                    0,
                    1,
                    0,
                    1,
                    1,
                    {0, 0, 0},
                    {0, 0, 0},
                    {800, 600, 1}}}
            );

            builder->pipeline_barrier(
                mff::vulkan::UnsafeCommandBufferBuilderPipelineBarrier()
                    .add_image_memory_barrier(
                        destination,
                        0,
                        1,
                        0,
                        1,
                        vk::PipelineStageFlagBits::eTransfer,
                        vk::AccessFlagBits::eTransferWrite,
                        vk::PipelineStageFlagBits::eBottomOfPipe,
                        {},
                        true,
                        std::nullopt,
                        vk::ImageLayout::eTransferDstOptimal,
                        vk::ImageLayout::ePresentSrcKHR
                    )
                    .add_image_memory_barrier(
                        source,
                        0,
                        1,
                        0,
                        1,
                        vk::PipelineStageFlagBits::eTransfer,
                        {},
                        vk::PipelineStageFlagBits::eColorAttachmentOutput,
                        vk::AccessFlagBits::eColorAttachmentWrite,
                        true,
                        std::nullopt,
                        vk::ImageLayout::eTransferSrcOptimal,
                        vk::ImageLayout::eColorAttachmentOptimal
                    ));

            builder->build();
            index++;
        }

        return {};
    }

    boost::leaf::result<bool> draw() {
        logger::main->info("Drawing swapchain (copy)");
        LEAF_AUTO(acquire_result, swapchain_->acquire_next_image_raw(present_end_semaphore_.get(), std::nullopt));
        auto[index, optimal] = acquire_result;

        if (!optimal) {
            LEAF_CHECK(build_swapchain());

            return false;
        }

        auto swapchain = swapchain_->get_handle();
        auto present_end = present_end_semaphore_->get_handle();
        auto draw_end = draw_end_semaphore_->get_handle();
        auto buffer = command_buffer_allocations_[index]->get_handle();
        vk::PipelineStageFlags flag = vk::PipelineStageFlagBits::eColorAttachmentOutput;

        LEAF_CHECK(
            mff::to_result(
                present_queue_->get_handle()
                    .submit(
                        {vk::SubmitInfo(
                            1,
                            &present_end,
                            &flag,
                            1,
                            &buffer,
                            1,
                            &draw_end
                        )},
                        {}
                    )));

        LEAF_CHECK(mff::to_result(
            present_queue_->get_handle()
                .presentKHR(vk::PresentInfoKHR(1, &draw_end, 1, &swapchain, &index))));

        return true;
    }

private:
    Presenter() = default;

    boost::leaf::result<void> build_swapchain() {
        logger::main->info("Rebuilding swapchain");
        LEAF_AUTO(capabilities, surface_->get_capabilities(device_->get_physical_device()));

        LEAF_CHECK_OPTIONAL(
            surface_format,
            capabilities.find_format({{vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}}));
        LEAF_CHECK_OPTIONAL(
            present_mode,
            capabilities.find_present_mode({vk::PresentModeKHR::eMailbox}));
        auto extent = capabilities.normalize_extent(window_->get_inner_size());

        auto image_count = capabilities.min_image_count + 1;

        if (capabilities.max_image_count && image_count > capabilities.max_image_count.value()) {
            image_count = capabilities.max_image_count.value();
        }

        LEAF_AUTO(
            swapchain_result,
            mff::vulkan::Swapchain::build(
                device_,
                surface_,
                image_count,
                surface_format.format,
                extent,
                1,
                capabilities.supported_usage_flags | vk::ImageUsageFlags(vk::ImageUsageFlagBits::eColorAttachment),
                mff::vulkan::SharingMode_::Exclusive{}, // we will use only present queue with swapchain
                capabilities.current_transform,
                vk::CompositeAlphaFlagBitsKHR::eOpaque,
                present_mode,
                true,
                surface_format.colorSpace,
                swapchain_ ? std::make_optional(swapchain_.get()) : std::nullopt
            ));
        std::tie(swapchain_, swapchain_images_) = std::move(swapchain_result);

        LEAF_AUTO(command_pool, device_->get_command_pool(present_queue_->get_queue_family()));
        LEAF_AUTO_TO(command_buffer_allocations_, command_pool->allocate(swapchain_images_.size()));

        return {};
    }

    std::shared_ptr<mff::window::Window> window_ = nullptr;
    mff::vulkan::SharedQueue present_queue_ = nullptr;
    mff::vulkan::Device* device_ = nullptr;
    const mff::vulkan::Surface* surface_ = nullptr;
    mff::vulkan::UniqueSemaphore present_end_semaphore_ = nullptr;
    mff::vulkan::UniqueSemaphore draw_end_semaphore_ = nullptr;
    mff::vulkan::UniqueSwapchain swapchain_ = nullptr;
    std::vector<mff::vulkan::UniqueSwapchainImage> swapchain_images_ = {};
    std::vector<mff::vulkan::UniqueCommandPoolAllocation> command_buffer_allocations_ = {};

    // mff::vulkan::

};

class RendererContext {
public:
    static boost::leaf::result<std::unique_ptr<RendererContext>> build(
        VulkanBaseEngine* engine,
        Presenter* presenter
    ) {
        struct enable_RendererContext : public RendererContext {};
        std::unique_ptr<RendererContext> result = std::make_unique<enable_RendererContext>();

        result->engine_ = engine;
        result->presenter_ = presenter;
        LEAF_AUTO_TO(
            result->render_pass_main_,
            result->build_render_pass(vk::AttachmentLoadOp::eLoad, vk::AttachmentLoadOp::eLoad));
        LEAF_AUTO_TO(
            result->render_pass_clear_stencil_,
            result->build_render_pass(vk::AttachmentLoadOp::eLoad, vk::AttachmentLoadOp::eClear));
        LEAF_AUTO_TO(
            result->render_pass_clear_all_,
            result->build_render_pass(vk::AttachmentLoadOp::eClear, vk::AttachmentLoadOp::eClear));


        LEAF_CHECK(result->build_pipeline_layout());
        LEAF_CHECK(result->build_pipelines());

        return result;
    }

    const mff::vulkan::RenderPass* get_renderpass() const {
        return render_pass_clear_all_.get();
    }

    mff::vulkan::Device* get_device() {
        return engine_->get_device();
    }

    const mff::vulkan::Swapchain* get_swapchain() const {
        return presenter_->get_swapchain();
    }

    vk::PipelineLayout get_pipeline_layout() {
        return pipeline_layout_.get();
    }

    vk::Pipeline get_pipeline() {
        return pipeline_over_.get();
    }

private:
    RendererContext() = default;

    boost::leaf::result<mff::vulkan::UniqueRenderPass> build_render_pass(
        vk::AttachmentLoadOp load_op, vk::AttachmentLoadOp stencil_load_op
    ) {
        return mff::vulkan::RenderPassBuilder()
            .add_attachment(
                "color",
                load_op,
                vk::AttachmentStoreOp::eStore,
                presenter_->get_swapchain()->get_format(),
                vk::SampleCountFlagBits::e1,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::AttachmentLoadOp::eDontCare,
                vk::AttachmentStoreOp::eDontCare
            )
            .add_attachment(
                "depth",
                vk::AttachmentLoadOp::eDontCare,
                vk::AttachmentStoreOp::eDontCare,
                find_stencil_format(engine_->get_device()->get_physical_device()).value(),
                vk::SampleCountFlagBits::e1,
                vk::ImageLayout::eDepthStencilAttachmentOptimal,
                vk::ImageLayout::eDepthStencilAttachmentOptimal,
                stencil_load_op,
                vk::AttachmentStoreOp::eStore
            )
            .add_pass({"color"}, {})
            .build(engine_->get_device());
    }

    boost::leaf::result<void> build_pipeline_layout() {
        std::vector<vk::PushConstantRange> push_constant_range = {
            vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants))
        };

        vk::PipelineLayoutCreateInfo pipeline_layout_info(
            {},
            0,
            nullptr,
            push_constant_range.size(),
            push_constant_range.data());

        LEAF_AUTO_TO(
            pipeline_layout_,
            mff::to_result(get_device()->get_handle().createPipelineLayoutUnique(pipeline_layout_info)));

        return {};
    }

    boost::leaf::result<void> build_pipelines() {
        vk::StencilOpState fill_op_state(
            vk::StencilOp::eKeep,
            vk::StencilOp::eInvert,
            vk::StencilOp::eKeep,
            vk::CompareOp::eEqual,
            kSTENCIL_CLIP_BIT,
            kSTENCIL_FILL_BIT,
            0
        );

        vk::StencilOpState cliping_op_state(
            vk::StencilOp::eKeep,
            vk::StencilOp::eReplace,
            vk::StencilOp::eKeep,
            vk::CompareOp::eEqual,
            kSTENCIL_FILL_BIT,
            kSTENCIL_ALL_BIT,
            0x2
        );

        vk::StencilOpState stencil_op_state(
            vk::StencilOp::eKeep,
            vk::StencilOp::eZero,
            vk::StencilOp::eKeep,
            vk::CompareOp::eEqual,
            kSTENCIL_FILL_BIT,
            kSTENCIL_FILL_BIT,
            0x1
        );

        vk::ColorComponentFlags all_colors =
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB
                | vk::ColorComponentFlagBits::eA;

        LEAF_AUTO_TO(
            pipeline_over_,
            build_pipeline(
                vk::PrimitiveTopology::eTriangleList,
                stencil_op_state,
                all_colors,
                3,
                2,
                vk::BlendOp::eAdd,
                true
            ));

        LEAF_AUTO_TO(
            pipeline_sub_,
            build_pipeline(
                vk::PrimitiveTopology::eTriangleList,
                stencil_op_state,
                all_colors,
                3,
                2,
                vk::BlendOp::eSubtract,
                true
            ));

        LEAF_AUTO_TO(
            pipeline_clear_,
            build_pipeline(
                vk::PrimitiveTopology::eTriangleList,
                stencil_op_state,
                all_colors,
                3,
                2,
                vk::BlendOp::eSubtract,
                false
            ));

        LEAF_AUTO_TO(
            pipeline_polyfill_,
            build_pipeline(
                vk::PrimitiveTopology::eTriangleFan,
                fill_op_state,
                {},
                2,
                1,
                vk::BlendOp::eAdd,
                true
            ));


        LEAF_AUTO_TO(
            pipeline_clipping_,
            build_pipeline(
                vk::PrimitiveTopology::eTriangleList,
                cliping_op_state,
                {},
                5,
                1,
                vk::BlendOp::eAdd,
                true
            ));

        return {};
    }

    boost::leaf::result<vk::UniquePipeline> build_pipeline(
        vk::PrimitiveTopology topology,
        vk::StencilOpState stencil_op,
        vk::ColorComponentFlags color_mask = {},
        std::uint32_t dynamics_count = 5,
        std::uint32_t stages_count = 2,
        vk::BlendOp blend_op = vk::BlendOp::eAdd,
        bool blend_enabled = true
    ) {
        vk::PipelineRasterizationStateCreateInfo rasterization_info(
            {},
            false,
            false,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eNone,
            vk::FrontFace::eCounterClockwise,
            false,
            {},
            {},
            {},
            1.0f
        );
        vk::PipelineInputAssemblyStateCreateInfo input_assembly_info({}, topology);

        vk::PipelineColorBlendAttachmentState blend_attachment(
            blend_enabled,
            vk::BlendFactor::eOne,
            vk::BlendFactor::eOneMinusSrcAlpha,
            blend_op,
            vk::BlendFactor::eOne,
            vk::BlendFactor::eOne,
            blend_op,
            color_mask
        );

        vk::PipelineColorBlendStateCreateInfo color_blend_state(
            {},
            false,
            vk::LogicOp::eClear,
            1,
            &blend_attachment
        );


        vk::PipelineDepthStencilStateCreateInfo depth_stencil_info(
            {},
            false,
            false,
            vk::CompareOp::eAlways,
            false,
            true,
            stencil_op,
            stencil_op
        );

        std::vector<vk::DynamicState> dynamic_enabled_states = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
            vk::DynamicState::eStencilCompareMask,
            vk::DynamicState::eStencilReference,
            vk::DynamicState::eStencilWriteMask
        };

        vk::PipelineDynamicStateCreateInfo dynamics_info({}, dynamics_count, dynamic_enabled_states.data());
        vk::PipelineViewportStateCreateInfo viewport_info({}, 1, nullptr, 1, nullptr);

        // Build vertex info
        std::vector<vk::VertexInputBindingDescription> vertex_input_binding = {Vertex::get_binding_description()};
        auto vertex_input_attributes = Vertex::get_attribute_descriptions();
        vk::PipelineVertexInputStateCreateInfo vertex_input_info(
            {},
            vertex_input_binding.size(),
            vertex_input_binding.data(),
            vertex_input_attributes.size(),
            vertex_input_attributes.data());

        LEAF_AUTO(vertex_shader_module, create_shader_module(get_device(), "shaders/shader.vert.spv"));
        LEAF_AUTO(fragment_shader_module, create_shader_module(get_device(), "shaders/shader.frag.spv"));

        vk::PipelineShaderStageCreateInfo vertex_stage(
            {},
            vk::ShaderStageFlagBits::eVertex,
            vertex_shader_module.get(),
            "main"
        );

        vk::PipelineShaderStageCreateInfo fragment_stage(
            {},
            vk::ShaderStageFlagBits::eFragment,
            fragment_shader_module.get(),
            "main"
        );

        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = {vertex_stage, fragment_stage};

        vk::PipelineMultisampleStateCreateInfo multisample_info({}, vk::SampleCountFlagBits::e1);

        vk::GraphicsPipelineCreateInfo create_info(
            {},
            stages_count,
            shader_stages.data(),
            &vertex_input_info,
            &input_assembly_info,
            nullptr,
            &viewport_info,
            &rasterization_info,
            &multisample_info,
            &depth_stencil_info,
            &color_blend_state,
            &dynamics_info,
            pipeline_layout_.get(),
            render_pass_main_->get_handle()
        );

        return mff::to_result(get_device()->get_handle().createGraphicsPipelineUnique(nullptr, create_info));
    }

    VulkanBaseEngine* engine_;
    Presenter* presenter_;
    mff::vulkan::UniqueRenderPass render_pass_main_ = nullptr;
    mff::vulkan::UniqueRenderPass render_pass_clear_stencil_ = nullptr;
    mff::vulkan::UniqueRenderPass render_pass_clear_all_ = nullptr;

    vk::UniquePipelineLayout pipeline_layout_;

    vk::UniquePipeline pipeline_over_;
    vk::UniquePipeline pipeline_sub_;
    vk::UniquePipeline pipeline_clear_;

    vk::UniquePipeline pipeline_polyfill_;
    vk::UniquePipeline pipeline_clipping_;
};

class RendererSurface {
public:
    static boost::leaf::result<std::unique_ptr<RendererSurface>> build(
        RendererContext* renderer, std::array<std::uint32_t, 2> dimensions
    ) {
        struct enable_RendererSurface : public RendererSurface {};
        std::unique_ptr<RendererSurface> result = std::make_unique<enable_RendererSurface>();

        result->renderer_ = renderer;

        LEAF_AUTO_TO(
            result->image_,
            mff::vulkan::AttachmentImage::build(
                result->renderer_->get_device(),
                dimensions,
                result->renderer_->get_swapchain()->get_format(),
                vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment
                    | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc,
                vk::SampleCountFlagBits::e1
            ));

        LEAF_AUTO_TO(
            result->stencil_,
            mff::vulkan::AttachmentImage::build(
                result->renderer_->get_device(),
                dimensions,
                vk::Format::eS8Uint,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst
                    | vk::ImageUsageFlagBits::eTransferSrc,
                vk::SampleCountFlagBits::e1
            ));

        LEAF_AUTO_TO(
            result->framebuffer_,
            mff::vulkan::FramebufferBuilder::start(result->renderer_->get_renderpass())
                .add(result->image_->get_image_view_impl())
                .add(result->stencil_->get_image_view_impl())
                .build());

        return result;
    }

    const mff::vulkan::Image* get_main_image() const {
        return image_->get_image_impl();
    }

    const mff::vulkan::Framebuffer* get_framebuffer() const {
        return framebuffer_.get();
    }

    std::uint32_t get_width() const {
        return dimensions_[0];
    }

    std::uint32_t get_height() const {
        return dimensions_[1];
    }

private:
    RendererSurface() = default;

    RendererContext* renderer_;
    std::array<std::uint32_t, 2> dimensions_;
    vk::Format format_;
    mff::vulkan::UniqueFramebuffer framebuffer_;
    mff::vulkan::UniqueAttachmentImage image_;
    mff::vulkan::UniqueAttachmentImage stencil_;
    bool new_;
};

class Renderer {
public:
    boost::leaf::result<void> draw(
        std::vector<Vertex> vertexes, std::vector<std::uint32_t> indices, PushConstants push_constants
    ) {
        logger::main->info("Drawing renderer");

        request_vertex_buffer(vertexes.size() * sizeof(Vertex));
        request_index_buffer(indices.size() * sizeof(std::uint32_t));

        memcpy(vertex_buffer_->get_allocation_info().pMappedData, vertexes.data(), vertexes.size() * sizeof(Vertex));
        memcpy(
            index_buffer_->get_allocation_info().pMappedData,
            indices.data(),
            indices.size() * sizeof(std::uint32_t));

        vk::CommandBuffer buffer = command_buffer_alloc_->get_handle();
        LEAF_CHECK(mff::to_result(buffer.reset({})));

        LEAF_CHECK(mff::to_result(buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit))));

        std::vector<vk::ClearValue> clear_values = {
            vk::ClearValue(vk::ClearColorValue(std::array<std::uint32_t, 4>{0, 0, 0, 0})),
            vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0)),
            vk::ClearValue(vk::ClearColorValue(std::array<std::uint32_t, 4>{0, 0, 0, 0}))
        };

        buffer.beginRenderPass(
            vk::RenderPassBeginInfo(
                context_->get_renderpass()->get_handle(),
                surface_->get_framebuffer()->get_handle(),
                vk::Rect2D(surface_->get_width(), surface_->get_height()),
                clear_values.size(),
                clear_values.data()),
            vk::SubpassContents::eInline
        );

        buffer.setViewport(0, {vk::Viewport(0, 0, surface_->get_width(), surface_->get_height(), 0, 1)});

        buffer.setScissor(
            0,
            {vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(surface_->get_width(), surface_->get_height()))}
        );

        auto vb = vertex_buffer_->get_buffer();
        buffer.bindVertexBuffers(0, {vb}, {0});
        auto ib = index_buffer_->get_buffer();
        buffer.bindIndexBuffer(ib, 0, vk::IndexType::eUint32);

        buffer.pushConstants(
            context_->get_pipeline_layout(),
            vk::ShaderStageFlagBits::eVertex,
            0,
            sizeof(PushConstants),
            &push_constants
        );

        buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, context_->get_pipeline());
        buffer.setStencilCompareMask(vk::StencilFaceFlagBits::eFrontAndBack, kSTENCIL_CLIP_BIT);
        buffer.drawIndexed(indices.size(), 1, 0, 0, 0);
        buffer.endRenderPass();
        LEAF_CHECK(mff::to_result(buffer.end()));

        vk::PipelineStageFlags wait_flag = vk::PipelineStageFlagBits::eAllCommands;
        vk::SubmitInfo submit_info(0, nullptr, &wait_flag, 1, &buffer, 0, nullptr);
        graphics_queue_->get_handle().submit({submit_info}, fence_->get_handle());
        context_->get_device()->get_handle().waitForFences({fence_->get_handle()}, true, std::numeric_limits<std::uint64_t>::max());
        context_->get_device()->get_handle().resetFences({fence_->get_handle()});

        return {};
    }

    static boost::leaf::result<std::unique_ptr<Renderer>> build(
        RendererContext* context,
        RendererSurface* surface,
        mff::vulkan::SharedQueue graphics_queue
    ) {
        struct enable_Renderer: public Renderer {};
        std::unique_ptr<Renderer> result = std::make_unique<enable_Renderer>();

        result->context_ = context;
        result->surface_ = surface;
        result->graphics_queue_ = graphics_queue;

        auto device = context->get_device();

        LEAF_CHECK(result->request_vertex_buffer(1024));
        LEAF_CHECK(result->request_index_buffer(1024));
        LEAF_AUTO_TO(result->fence_, mff::vulkan::Fence::build(device, false));
        LEAF_AUTO(pool, context->get_device()->get_command_pool(graphics_queue->get_queue_family()));
        LEAF_AUTO(cmd_buffs, pool->allocate(1, false));
        result->command_buffer_alloc_ = std::move(cmd_buffs[0]);

        return result;
    }

private:
    Renderer() = default;

    boost::leaf::result<vma::UniqueBuffer> create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage) {
        auto buffer_info = vk::BufferCreateInfo(
            {},
            size,
            usage,
            vk::SharingMode::eExclusive
        );

        VmaAllocationCreateInfo allocation_info = {};
        allocation_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocation_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        return context_->get_device()->get_allocator()->create_buffer(buffer_info, allocation_info);
    }

    boost::leaf::result<void> request_vertex_buffer(vk::DeviceSize required_size) {
        if (vertex_buffer_ != nullptr && vertex_buffer_->get_size() >= required_size) return {};

        LEAF_AUTO_TO(vertex_buffer_, create_buffer(required_size, vk::BufferUsageFlagBits::eVertexBuffer));

        return {};
    }


    boost::leaf::result<void> request_index_buffer(vk::DeviceSize required_size) {
        if (index_buffer_ != nullptr && index_buffer_->get_size() >= required_size) return {};

        LEAF_AUTO_TO(index_buffer_, create_buffer(required_size, vk::BufferUsageFlagBits::eIndexBuffer));

        return {};
    }

    RendererContext* context_;
    RendererSurface* surface_;

    vma::UniqueBuffer vertex_buffer_;
    vma::UniqueBuffer index_buffer_;

    mff::vulkan::UniqueCommandPoolAllocation command_buffer_alloc_;
    mff::vulkan::UniqueFence fence_;

    mff::vulkan::SharedQueue graphics_queue_;
};
