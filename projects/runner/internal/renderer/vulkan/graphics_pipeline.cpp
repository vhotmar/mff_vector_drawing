#include "./graphics_pipeline.h"

#include <utility>

namespace mff::vulkan {

GraphicsPipeline::GraphicsPipeline(std::shared_ptr<Device> device)
    : device_(std::move(device)) {

}

GraphicsPipelineBuilder::GraphicsPipelineBuilder() {

}

boost::leaf::result<std::shared_ptr<GraphicsPipeline>> GraphicsPipelineBuilder::build(std::shared_ptr<Device> device) {


    return boost::leaf::result<std::shared_ptr<GraphicsPipeline>>();
}

}