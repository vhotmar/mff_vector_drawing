#include "./renderer_context.h"

#include "./vulkan_shaders.h"

/**
 * Get stencil format which is supported by physical deice
 * @param physical_device
 * @return the best stencil format we want or std::nullopt
 */
std::optional<vk::Format> find_stencil_format(
    const mff::vulkan::PhysicalDevice* physical_device
) {
    return physical_device->find_supported_format(
        {vk::Format::eS8Uint, vk::Format::eD16UnormS8Uint, vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint},
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

/**
 * Build the description for this vertex
 * @param binding on which position it should be bound in shader
 * @return
 */
vk::VertexInputBindingDescription Vertex::get_binding_description(std::uint32_t binding) {
    vk::VertexInputBindingDescription bindingDescription(binding, sizeof(Vertex), vk::VertexInputRate::eVertex);

    return bindingDescription;
}

/**
 * Get the description of "inputs" for shaders
 * @return
 */
std::array<vk::VertexInputAttributeDescription, 1> Vertex::get_attribute_descriptions() {
    return {
        vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos)),
    };
}

boost::leaf::result<std::unique_ptr<RendererContext>> RendererContext::build(
    VulkanEngine* engine,
    vk::Format color_format
) {
    struct enable_RendererContext : public RendererContext {};
    std::unique_ptr<RendererContext> result = std::make_unique<enable_RendererContext>();

    result->engine_ = engine;
    result->color_format_ = color_format;
    LEAF_CHECK_OPTIONAL(stencil_format, find_stencil_format(engine->get_device()->get_physical_device()));
    result->stencil_format_ = stencil_format;

    // build render passes and pipeines
    LEAF_AUTO_TO(
        result->render_pass_main_,
        result->build_render_pass(vk::AttachmentLoadOp::eLoad, vk::AttachmentLoadOp::eLoad));

    LEAF_CHECK(result->build_pipeline_layout());
    LEAF_CHECK(result->build_pipelines());

    return result;
}

const mff::vulkan::RenderPass* RendererContext::get_renderpass() const {
    return render_pass_main_.get();
}

mff::vulkan::Device* RendererContext::get_device() {
    return engine_->get_device();
}

vk::PipelineLayout RendererContext::get_pipeline_layout() {
    return pipeline_layout_.get();
}

vk::Pipeline RendererContext::get_over_pipeline() {
    return pipeline_over_.get();
}

boost::leaf::result<mff::vulkan::UniqueRenderPass> RendererContext::build_render_pass(
    vk::AttachmentLoadOp load_op,
    vk::AttachmentLoadOp stencil_load_op
) {
    // we will use two attachments (for now we are using only color but in future stencil will be
    // needed)
    return mff::vulkan::RenderPassBuilder()
        .add_attachment(
            "color",
            load_op,
            vk::AttachmentStoreOp::eStore,
            color_format_,
            vk::SampleCountFlagBits::e1,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare
        )
        .add_attachment(
            "stencil",
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            stencil_format_,
            vk::SampleCountFlagBits::e1,
            vk::ImageLayout::eDepthStencilAttachmentOptimal,
            vk::ImageLayout::eDepthStencilAttachmentOptimal,
            stencil_load_op,
            vk::AttachmentStoreOp::eStore
        )
        .add_pass({"color"}, {"stencil"})
        .build(engine_->get_device());
}

boost::leaf::result<void> RendererContext::build_pipeline_layout() {
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

boost::leaf::result<void> RendererContext::build_pipelines() {
    vk::StencilOpState stencil_op_state(
        vk::StencilOp::eKeep,
        vk::StencilOp::eZero,
        vk::StencilOp::eKeep,
        vk::CompareOp::eEqual,
        kSTENCIL_FILL_BIT,
        kSTENCIL_FILL_BIT,
        0x1
    );

    BuildPipelineInfo over_info = {};
    over_info.dynamics_count = 3;
    over_info.stencil_op = stencil_op_state;
    LEAF_AUTO_TO(pipeline_over_,build_pipeline(over_info));

    return {};
}

boost::leaf::result<vk::UniquePipeline> RendererContext::build_pipeline(BuildPipelineInfo info) {
    // most of this is basic setting for pipelines
    vk::PipelineRasterizationStateCreateInfo rasterization_info(
        {},
        false, // no depth calmping
        false, // no discarding
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eNone, // display clockwise and counter-clockwise polygons
        vk::FrontFace::eClockwise,
        false,
        {},
        {},
        {},
        1.0f
    );
    vk::PipelineInputAssemblyStateCreateInfo input_assembly_info({}, info.topology);

    vk::PipelineColorBlendAttachmentState blend_attachment(
        info.blend_enabled,
        vk::BlendFactor::eSrcAlpha, // use alpha blending
        vk::BlendFactor::eOneMinusSrcAlpha,
        info.blend_op,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        info.blend_op,
        info.color_mask
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
        false, // disable depth testing (for now we are using only 2D)
        false,
        vk::CompareOp::eAlways,
        false,
        false, // TODO: find why not working with enabled
        info.stencil_op,
        info.stencil_op
    );

    std::vector<vk::DynamicState> dynamic_enabled_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
        vk::DynamicState::eStencilCompareMask,
        vk::DynamicState::eStencilReference,
        vk::DynamicState::eStencilWriteMask
    };

    vk::PipelineDynamicStateCreateInfo dynamics_info({}, info.dynamics_count, dynamic_enabled_states.data());
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
        info.stages_count,
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

vk::Format RendererContext::get_color_attachment_format() const {
    return color_format_;
}

vk::Format RendererContext::get_stencil_attachment_format() const {
    return stencil_format_;
}
