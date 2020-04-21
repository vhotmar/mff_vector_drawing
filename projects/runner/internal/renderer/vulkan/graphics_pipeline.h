#pragma once

#include "../../leaf.h"
#include "../../vulkan.h"

#include "./device.h"

namespace mff::vulkan {

class VertexDefinition {};

class GraphicsPipeline {
private:
    vk::UniquePipeline handle_;
    std::shared_ptr<Device> device_;

    GraphicsPipeline(std::shared_ptr<Device> device);
};

class GraphicsPipelineBuilder {
public:
    GraphicsPipelineBuilder();

    boost::leaf::result<std::shared_ptr<GraphicsPipeline>> build(std::shared_ptr<Device> device);
};

}