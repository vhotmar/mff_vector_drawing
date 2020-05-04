#include <mff/graphics/vulkan/command_buffer/builders/sync.h>

#include <mff/graphics/utils.h>
#include <mff/utils.h>

namespace mff::vulkan {

boost::leaf::result<UniqueSyncCommandBufferBuilder> SyncCommandBufferBuilder::build(
    CommandPool* pool,
    Kind kind,
    vk::CommandBufferUsageFlags usage
) {
    struct enable_SyncCommandBufferBuilder : public SyncCommandBufferBuilder {};
    UniqueSyncCommandBufferBuilder result = std::make_unique<enable_SyncCommandBufferBuilder>();

    LEAF_AUTO_TO(result->inner_, UnsafeCommandBufferBuilder::build(pool, kind, usage));
    result->is_secondary_ = std::visit(
        overloaded{
            [&](Kind_::Primary) -> bool { return false; },
            [&](Kind_::Secondary) -> bool { return true; }
        },
        kind
    );
    result->pending_barrier_ = UnsafeCommandBufferBuilderPipelineBarrier{};

    return result;
}

boost::leaf::result<void> SyncCommandBufferBuilder::copy_image(
    const Image* source,
    vk::ImageLayout source_layout,
    const Image* destination,
    vk::ImageLayout destination_layout,
    const std::vector<UnsafeCommandBufferBuilderImageCopy>& regions
) {
    class _Command : public Command {
    public:
        _Command(
            const Image* source,
            vk::ImageLayout source_layout,
            const Image* destination,
            vk::ImageLayout destination_layout,
            const std::vector<UnsafeCommandBufferBuilderImageCopy>& regions
        )
            : source_(source)
            , source_layout_(source_layout)
            , destination_(destination)
            , destination_layout_(destination_layout)
            , regions_(regions) {

        }

        std::string get_name() const override {
            return "vkCmdCopyImage";
        }

        void send(UnsafeCommandBufferBuilder* builder) override {
            builder->copy_image(
                source_.value(),
                source_layout_,
                destination_.value(),
                destination_layout_,
                regions_.value());
        }

        std::unique_ptr<FinalCommand> to_final_command() override {
            class _FinalCommand : public FinalCommand {
            public:
                _FinalCommand(const Image* source, const Image* destination)
                    : source_(source), destination_(destination_) {}

                std::string get_name() const override {
                    return "vkCmdCopyImage";
                }

                const Image* image(std::size_t num) const override {
                    if (num == 0) return source_;
                    if (num == 1) return destination_;

                    assert(false);
                }

                std::string image_name(std::size_t num) const override {
                    if (num == 0) return "source";
                    if (num == 1) return "destination";

                    assert(false);
                }

            private:
                const Image* source_;
                const Image* destination_;
            };

            return std::make_unique<_FinalCommand>(source_.value(), destination_.value());
        }

        const Image* image(std::size_t num) const override {
            if (num == 0) return source_.value();
            if (num == 1) return destination_.value();

            assert(false);
        }

        std::string image_name(std::size_t num) const override {
            if (num == 0) return "source";
            if (num == 1) return "destination";

            assert(false);
        }

    private:
        std::optional<const Image*> source_;
        vk::ImageLayout source_layout_;
        std::optional<const Image*> destination_;
        vk::ImageLayout destination_layout_;
        std::optional<std::vector<UnsafeCommandBufferBuilderImageCopy>> regions_;
    };

    commands_.push_back(std::make_unique<_Command>(source, source_layout, destination, destination_layout, regions));

    return boost::leaf::result<void>();
}

}