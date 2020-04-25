#pragma once

#include <memory>

#include <mff/leaf.h>
#include <mff/graphics/vulkan/pipeline/blend.h>
#include <mff/graphics/vulkan/pipeline/depth_stencil.h>
#include <mff/graphics/vulkan/pipeline/raster.h>
#include <mff/graphics/vulkan/pipeline/shader.h>
#include <mff/graphics/vulkan/pipeline/viewport.h>
#include <mff/graphics/vulkan/pipeline/vertex.h>
#include <mff/graphics/vulkan/render_pass.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

class GraphicsPipelineBuilder;

class GraphicsPipeline {
private:
    std::shared_ptr<Device> device_;
    std::shared_ptr<PipelineLayout> pipeline_layout_;
    vk::UniquePipeline pipeline_;

    GraphicsPipeline() = default;

    friend class GraphicsPipelineBuilder;
};

class GraphicsPipelineBuilder {
private:
    pipeline::VertexDefinition vertex_input;
    GraphicsEntryPoint vertex_shader;
    GraphicsEntryPoint fragment_shader;
    vk::PipelineInputAssemblyStateCreateInfo input_assembly;
    vk::PrimitiveTopology input_assembly_topology;
    std::optional<ViewportsState> viewport;
    Rasterization raster;
    vk::PipelineMultisampleStateCreateInfo multisample;
    DepthStencil depth_stencil;
    Blend blend;
    std::optional<std::shared_ptr<Subpass>> render_pass;

public:
    boost::leaf::result<std::shared_ptr<GraphicsPipeline>> build(std::shared_ptr<Device> device);
};

}