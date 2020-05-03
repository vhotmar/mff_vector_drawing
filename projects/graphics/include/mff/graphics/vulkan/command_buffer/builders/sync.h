#pragma once

#include <array>

#include <mff/leaf.h>
#include <mff/graphics/vulkan/command_buffer/builders/base.h>
#include <mff/graphics/vulkan/command_buffer/builders/unsafe.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

class SyncCommandBufferBuilder;
using UniqueSyncCommandBufferBuilder = std::unique_ptr<SyncCommandBufferBuilder>;

class FinalCommand {
public:
    virtual std::string get_name() const = 0;
    virtual const Image* image(std::size_t num) {
        assert(false);
    };
    virtual std::string image_name(std::size_t num) {
        assert(false);
    };
};

class Command {
public:
    virtual std::string get_name() const = 0;
    virtual void send(UnsafeCommandBufferBuilder* builder) = 0;
    virtual std::unique_ptr<FinalCommand> to_final_command() = 0;
    virtual const Image* image(std::size_t num) {
        assert(false);
    };
    virtual std::string image_name(std::size_t num) {
        assert(false);
    };
};

enum class ResourceType {
    Buffer,
    Image
};

/**
 * Sync resources
 */
class SyncCommandBufferBuilder : public CommandBufferBuilder<SyncCommandBufferBuilder> {
public:
    boost::leaf::result<void> copy_image(
        const Image* source,
        vk::ImageLayout source_layout,
        const Image* destination,
        vk::ImageLayout destination_layout,
        const std::vector<UnsafeCommandBufferBuilderImageCopy>& regions
    );

    static boost::leaf::result<UniqueSyncCommandBufferBuilder> build(
        CommandPool* pool,
        Kind kind,
        vk::CommandBufferUsageFlags usage = {}
    );

private:
    SyncCommandBufferBuilder() = default;

    UniqueUnsafeCommandBufferBuilder inner_ = nullptr;
    std::size_t first_unflushed = -1;
    std::size_t latest_render_pass_enter_ = -1;
    std::vector<std::unique_ptr<Command>> commands_ = {};
    bool is_secondary_ = false;
    UnsafeCommandBufferBuilderPipelineBarrier pending_barrier_;
};

}
