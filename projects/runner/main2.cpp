#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <tl/expected.hpp>
#include <shaderc/shaderc.hpp>
#include <glm/glm.hpp>

#include "glfw/raii.h"
#include "utils/vulkan.h"
#include "internal/window/window.h"

#ifdef NDEBUG
const bool ENABLE_VALIDATION_LAYERS = false;
#else
const bool ENABLE_VALIDATION_LAYERS = true;
#endif

const std::vector<std::string> VALIDATION_LAYERS = { "VK_LAYER_LUNARG_standard_validation" };
const std::vector<std::string> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

const int MAX_FRAMES_IN_FLIGHT = 2;

namespace run_utils {

template <typename T>
std::string join(const T& v, const std::string& delim) {
    std::ostringstream s;
    for (const auto& i : v) {
        if (&i != &v[0]) {
            s << delim;
        }
        s << i;
    }
    return s.str();
}

template <typename T>
void print_vector(T v) {
    std::cout << "(" << join(v, ", ") << ")" << std::endl;
}

std::vector<const char *> to_pointer_char_data(const std::vector<std::string>& data) {
    std::vector<const char *> result;

    for(const auto& item: data)
        result.push_back(&item.front());

    return result;
}

}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT flags,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* pUserData) {
    auto level = spdlog::level::level_enum::debug;

    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        level = spdlog::level::level_enum::err;
    } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        level = spdlog::level::level_enum::warn;
    } else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        level = spdlog::level::level_enum::info;
    }

    logger::vulkan->log(level, "Code {0}: {1}", data->pMessageIdName, data->pMessage);

    return false;
}

struct queue_family_indices {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;

    bool is_complete() {
        return graphics_family.has_value() && present_family.has_value();
    }
};

tl::expected<queue_family_indices, std::string> find_queue_families(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    queue_family_indices indices;

    auto queue_families = device.getQueueFamilyProperties();

    int i = 0;

    for (const auto& family: queue_families) {
        if (family.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphics_family = i;
        }

        if (VK_TRY(device.getSurfaceSupportKHR(i, surface))) {
            indices.present_family = i;
        }

        if (indices.is_complete()) {
            break;
        }

        i++;
    }

    return indices;
}

tl::expected<bool, std::string> check_device_extension_support(vk::PhysicalDevice device) {
    auto available_extensions = VK_TRY(device.enumerateDeviceExtensionProperties());

    std::set<std::string> required_extensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

    for (const auto& extension: available_extensions) {
        required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
}

struct swapchain_support_details {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> present_modes;
};

tl::expected<swapchain_support_details, std::string> query_swapchain_support(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    swapchain_support_details details;

    details.capabilities = VK_TRY(device.getSurfaceCapabilitiesKHR(surface));
    details.formats = VK_TRY(device.getSurfaceFormatsKHR(surface));
    details.present_modes = VK_TRY(device.getSurfacePresentModesKHR(surface));

    return details;
}

tl::expected<bool, std::string> is_device_suitable(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    auto properties = device.getProperties();
    auto features = device.getFeatures();

    bool is_discrete = properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
    bool has_queue_families = TRY(find_queue_families(device, surface)).is_complete();
    bool required_extensions_supported = TRY(check_device_extension_support(device));

    bool swapchain_adequate = false;

    // we can't query for swapchain support if the swapchain extension is not supported
    if (required_extensions_supported) {
        auto details = TRY(query_swapchain_support(device, surface));
        swapchain_adequate = !details.formats.empty() && !details.present_modes.empty();
    }

    return is_discrete
        && has_queue_families
        && required_extensions_supported
        && swapchain_adequate;
}

vk::SurfaceFormatKHR choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats) {
    for (const auto& available_format : available_formats) {
        if (available_format.format == vk::Format::eB8G8R8A8Srgb && available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return available_format;
        }
    }

    return available_formats[0];
}

vk::PresentModeKHR choose_swap_present_mode(const std::vector<vk::PresentModeKHR>& available_present_modes) {
    for (const auto& available_present_mode : available_present_modes) {
        if (available_present_mode == vk::PresentModeKHR::eMailbox) {
            return available_present_mode;
        }
    }

    return available_present_modes[0];
}

vk::Extent2D choose_swap_extent(const vk::SurfaceCapabilitiesKHR& capabilities, vk::Extent2D actual_extent) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        vk::Extent2D result_extent(actual_extent);

        result_extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actual_extent.width));
        result_extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actual_extent.height));

        return result_extent;
    }
}

std::string read_file(const std::string& path)
{
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

    std::ifstream::pos_type file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> bytes(file_size);
    file.read(bytes.data(), file_size);

    return std::string(bytes.data(), file_size);
}

tl::expected<std::vector<uint32_t>, std::string> load_shader(const std::string& path, const std::string& name, shaderc_shader_kind kind) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    auto result = compiler.CompileGlslToSpv(read_file(path), kind, name.c_str(), options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        return tl::make_unexpected("Shader compilation failed");
    }

    return std::vector<uint32_t>{result.begin(), result.end()};
}

void init_dispatcher() {
    vk::DynamicLoader dl;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
}

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static vk::VertexInputBindingDescription get_binding_description() {
        vk::VertexInputBindingDescription bindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex);

        return bindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 2> get_attribute_descriptions() {
        return {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
        };
    }
};

const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};

class base_application {
public:
    tl::expected<void, std::string> run() {
        TRY_V(init_window());
        TRY_V(init_vulkan());
        main_loop();

        return {};
    }

private:
    tl::expected<void, std::string> init_window() {
        context_ = glfw::create_context();

        TRY_V(glfw::window_hint(context_, glfw::WindowHint::ClientApi, GLFW_NO_API));

        window_ = TRY(glfw::create_window(context_, 800, 600, "Vulkan", nullptr, nullptr));
        window_->set_user_pointer(this);
        window_->set_framebuffer_size_callback(framebuffer_size_callback);

        return {};
    }

    static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<base_application*>(glfwGetWindowUserPointer(window));
        app->framebuffer_resized = true;
    }

    tl::expected<void, std::string> init_vulkan() {
        if (!TRY(glfw::vulkan_supported(context_)))
        {
            logger::it->critical("Vulkan not available");

            return tl::make_unexpected("Vulkan not available");
        }

        init_dispatcher();
        TRY_V(create_instance());

        logger::system->trace("Vulkan instance created successfully");

        TRY_V(setup_debug_messenger());
        TRY_V(create_surface());
        TRY_V(pick_physical_device());
        TRY_V(create_logical_device());
        TRY_V(create_swapchain());
        TRY_V(create_image_views());
        TRY_V(create_render_pass());
        TRY_V(create_graphics_pipeline());
        TRY_V(create_framebuffers());
        TRY_V(create_command_pool());
        TRY_V(create_vertex_buffer());
        TRY_V(create_command_buffer());
        TRY_V(create_sync_objects());

        logger::system->trace("Vulkan initialized successfully");

        return {};
    }

    void main_loop() {
        while (!window_->should_close()) {
            glfw::poll_events(context_);
            draw_frame();
        }

        device_->waitIdle();
    }

    tl::expected<void, std::string> create_instance() {
        if (ENABLE_VALIDATION_LAYERS && !TRY(check_validation_layer_support())) {
            return tl::make_unexpected("Validation layers requestd, but not available");
        }

        vk::ApplicationInfo application_info(
            "mff_vector_drawing",
            VK_MAKE_VERSION(1, 0, 0),
            "vc_draw",
            VK_MAKE_VERSION(1, 0, 0),
            VK_API_VERSION_1_2
        );

        std::vector<std::string> layers;
        auto required_extensions = TRY(glfw::get_required_instance_extensions(context_));

        if (ENABLE_VALIDATION_LAYERS) {
            required_extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
            required_extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

            layers = VALIDATION_LAYERS;
        }

        auto required_extensions_c = run_utils::to_pointer_char_data(required_extensions);
        auto layers_c = run_utils::to_pointer_char_data(layers);

        vk::InstanceCreateInfo create_instance_info(
            {},
            &application_info,
            layers_c.size(),
            layers_c.data(),
            required_extensions_c.size(),
            required_extensions_c.data()
        );

        if (ENABLE_VALIDATION_LAYERS) {
            vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT> chain = {
                create_instance_info,
                get_debug_utils_create_info()
            };

            instance_ = VK_TRY(vk::createInstanceUnique(chain.get<vk::InstanceCreateInfo>()));
        } else {
            instance_ = VK_TRY(vk::createInstanceUnique(create_instance_info));
        }

        // Update the DynamicLoadDispatcher
        VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance_);

        return {};
    }

    tl::expected<void, std::string> create_surface() {
        surface_ = TRY(window_->create_surface(*instance_, nullptr));

        return {};
    }

    tl::expected<void, std::string> create_swapchain() {
        auto swapchain_support = TRY(query_swapchain_support(physical_device_, *surface_));

        auto surface_format = choose_swap_surface_format(swapchain_support.formats);
        auto present_mode = choose_swap_present_mode(swapchain_support.present_modes);
        auto extent = choose_swap_extent(swapchain_support.capabilities, window_->framebuffer_extent());

        auto image_count = swapchain_support.capabilities.minImageCount + 1;

        if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount) {
            image_count = swapchain_support.capabilities.maxImageCount;
        }

        auto indices = TRY(find_queue_families(physical_device_, *surface_));
        std::vector<uint32_t> queue_family_indices = {*indices.graphics_family, *indices.present_family};
        bool different_queues = indices.present_family != indices.graphics_family;

        vk::SwapchainCreateInfoKHR swapchain_create_info(
            {},
            *surface_,
            image_count,
            surface_format.format,
            surface_format.colorSpace,
            extent,
            1,
            vk::ImageUsageFlagBits::eColorAttachment,
            different_queues ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive, // TODO: check how to use only Concurrent
            different_queues ? 2 : 0,
            different_queues ? queue_family_indices.data() : nullptr,
            swapchain_support.capabilities.currentTransform,
            vk::CompositeAlphaFlagBitsKHR::eOpaque,
            present_mode,
            {},
            *swapchain_);

        swapchain_ = VK_TRY(device_->createSwapchainKHRUnique(swapchain_create_info));
        swapchain_images_ = VK_TRY(device_->getSwapchainImagesKHR(*swapchain_));
        swapchain_image_format_ = surface_format.format;
        swapchain_extent_ = extent;

        return {};
    }

    tl::expected<void, std::string> recreate_swapchain() {
        auto size = window_->framebuffer_extent();

        while (size.height == 0 || size.width == 0) {
            size = window_->framebuffer_extent();

            glfw::poll_events(context_);
        }

        device_->waitIdle();

        TRY_V(create_swapchain());
        TRY_V(create_image_views());
        TRY_V(create_render_pass());
        TRY_V(create_graphics_pipeline());
        TRY_V(create_framebuffers());
        TRY_V(create_command_buffer());

        return {};
    }

    tl::expected<void, std::string> create_image_views() {
        swapchain_image_views_.clear();
        swapchain_image_views_.reserve(swapchain_images_.size());

        vk::ComponentMapping component_mapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
        vk::ImageSubresourceRange subresource_range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

        for (auto& image: swapchain_images_) {
            vk::ImageViewCreateInfo image_view_create_info(vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D, swapchain_image_format_, component_mapping, subresource_range);
            swapchain_image_views_.push_back(VK_TRY(device_->createImageViewUnique(image_view_create_info)));
        }

        return {};
    }

    tl::expected<bool, std::string> check_validation_layer_support() {
        auto instance_layers = VK_TRY(vk::enumerateInstanceLayerProperties());

        for (auto validation_layer: VALIDATION_LAYERS) {
            if (std::find_if(
                instance_layers.begin(),
                instance_layers.end(),
                [&validation_layer](const auto& layer) { return layer.layerName == validation_layer; }) == std::end(instance_layers)) {
                return tl::make_unexpected("Does not contain required layers");
            }
        }

        return true;
    }

    vk::DebugUtilsMessengerCreateInfoEXT get_debug_utils_create_info() {
        return vk::DebugUtilsMessengerCreateInfoEXT(
            {},
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
                | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
            &debug_callback,
            nullptr);
    }

    tl::expected<void, std::string> setup_debug_messenger() {
        if (!ENABLE_VALIDATION_LAYERS) return {};

        auto create_info = get_debug_utils_create_info();
        debug_utils_messenger_ = VK_TRY(instance_->createDebugUtilsMessengerEXTUnique(create_info));

        return {};
    }

    tl::expected<void, std::string> create_logical_device() {
        auto indices = TRY(find_queue_families(physical_device_, *surface_));

        auto queue_priority = 1.0f;
        std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
        std::set<uint32_t> unique_queue_families = {*indices.graphics_family, *indices.present_family};

        for (auto queue_family: unique_queue_families) {
            auto queue_create_info = vk::DeviceQueueCreateInfo(
                {},
                queue_family,
                1,
                &queue_priority
            );

            queue_create_infos.push_back(queue_create_info);
        }

        vk::PhysicalDeviceFeatures device_features;

        std::vector<std::string> layers;

        if (ENABLE_VALIDATION_LAYERS) {
            layers = VALIDATION_LAYERS;
        }

        auto layers_c = run_utils::to_pointer_char_data(layers);
        auto device_extensions_c = run_utils::to_pointer_char_data(DEVICE_EXTENSIONS);

        auto device_create_info = vk::DeviceCreateInfo(
            {},
            queue_create_infos.size(),
            queue_create_infos.data(),
            layers_c.size(),
            layers_c.data(),
            device_extensions_c.size(),
            device_extensions_c.data());

        device_ = VK_TRY(physical_device_.createDeviceUnique(device_create_info));

        // Update dispatcher
        VULKAN_HPP_DEFAULT_DISPATCHER.init(*device_);

        graphics_queue_ = device_->getQueue(*indices.graphics_family, 0);
        present_queue_ = device_->getQueue(*indices.present_family, 0);

        return {};
    }

    tl::expected<void, std::string> pick_physical_device() {
        auto devices = VK_TRY(instance_->enumeratePhysicalDevices());

        bool found = false;

        for (const auto& device: devices) {
            if (is_device_suitable(device, *surface_)) {
                physical_device_ = device;

                found = true;

                break;
            }
        }

        if (!found) return tl::make_unexpected("physical device not found");

        return {};
    }

    tl::expected<vk::UniqueShaderModule, std::string> create_shader_module(const std::vector<uint32_t>& code) {
        vk::ShaderModuleCreateInfo create_info({}, code.size() * sizeof(uint32_t), code.data());

        return VK_TRY(device_->createShaderModuleUnique(create_info));
    }

    tl::expected<vk::UniqueShaderModule, std::string> create_shader_module(const std::string& path, const std::string& name, shaderc_shader_kind kind) {
        auto code = TRY(load_shader(path, name, kind));

        return create_shader_module(code);
    }

    tl::expected<void, std::string> create_render_pass() {
        vk::AttachmentDescription color_attachment(
            {},
            swapchain_image_format_,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::ePresentSrcKHR);

        vk::AttachmentReference color_attachment_ref(0, vk::ImageLayout::eColorAttachmentOptimal);

        vk::SubpassDescription subpass(
            {},
            vk::PipelineBindPoint::eGraphics,
            0,
            nullptr,
            1,
            &color_attachment_ref);

        vk::SubpassDependency dependency(
            VK_SUBPASS_EXTERNAL,
            0,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            {},
            vk::AccessFlagBits::eColorAttachmentWrite);

        vk::RenderPassCreateInfo render_pass_info(
            {},
            1,
            &color_attachment,
            1,
            &subpass,
            1,
            &dependency);

        render_pass_ = VK_TRY(device_->createRenderPassUnique(render_pass_info));

        return {};
    }

    tl::expected<void, std::string> create_graphics_pipeline() {
        auto fragment_shader_module = TRY(create_shader_module("shaders/shader.frag", "fragment_shader", shaderc_fragment_shader));
        auto vertex_shader_module = TRY(create_shader_module("shaders/shader.vert", "vertex_shader", shaderc_vertex_shader));

        vk::PipelineShaderStageCreateInfo fragment_shader_stage_info(
            {},
            vk::ShaderStageFlagBits::eFragment,
            *fragment_shader_module,
            "main");

        vk::PipelineShaderStageCreateInfo vertex_shader_stage_info(
            {},
            vk::ShaderStageFlagBits::eVertex,
            *vertex_shader_module,
            "main");

        vk::PipelineShaderStageCreateInfo shader_stages[] {vertex_shader_stage_info, fragment_shader_stage_info};

        auto binding_description = Vertex::get_binding_description();
        auto attribute_descriptions = Vertex::get_attribute_descriptions();

        vk::PipelineVertexInputStateCreateInfo vertex_input_info(
            {},
            1,
            &binding_description,
            attribute_descriptions.size(),
            attribute_descriptions.data());

        vk::PipelineInputAssemblyStateCreateInfo input_assembly_info(
            {},
            vk::PrimitiveTopology::eTriangleList,
            false);

        vk::Viewport viewport(0.0f, 0.0f, swapchain_extent_.width, swapchain_extent_.height, 0.0f, 1.0f);
        vk::Rect2D scissor({0, 0}, swapchain_extent_);
        vk::PipelineViewportStateCreateInfo viewport_state(
            {},
            1,
            &viewport,
            1,
            &scissor);

        vk::PipelineRasterizationStateCreateInfo rasterizer(
            {},
            false,
            false,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack,
            vk::FrontFace::eClockwise,
            false,
            0.0f,
            0.0f,
            0.0f,
            1.0f);

        vk::PipelineMultisampleStateCreateInfo multisampling(
            {},
            vk::SampleCountFlagBits::e1,
            false,
            1.0f,
            nullptr,
            false,
            false);

        vk::PipelineColorBlendAttachmentState color_blend_attachment(
            false,
            vk::BlendFactor::eSrcAlpha,
            vk::BlendFactor::eOneMinusSrcAlpha,
            vk::BlendOp::eAdd,
            vk::BlendFactor::eOne,
            vk::BlendFactor::eZero,
            vk::BlendOp::eAdd,
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

        vk::PipelineColorBlendStateCreateInfo color_blending(
            {},
            false,
            vk::LogicOp::eCopy,
            1,
            &color_blend_attachment,
            {0.0f, 0.0f, 0.0f, 0.0f});

        vk::PipelineLayoutCreateInfo layout_info(
            {},
            0,
            nullptr,
            0,
            nullptr);

        pipeline_layout_ = VK_TRY(device_->createPipelineLayoutUnique(layout_info));

        vk::GraphicsPipelineCreateInfo pipeline_info(
            {},
            2,
            shader_stages,
            &vertex_input_info,
            &input_assembly_info,
            nullptr,
            &viewport_state,
            &rasterizer,
            &multisampling,
            nullptr,
            &color_blending,
            nullptr,
            *pipeline_layout_,
            *render_pass_,
            0);

        pipeline_ = VK_TRY(device_->createGraphicsPipelineUnique(nullptr, pipeline_info));

        return {};
    }

    tl::expected<void, std::string> create_framebuffers() {
        swapchain_framebuffers_.clear();
        swapchain_framebuffers_.reserve(swapchain_image_views_.size());

        for (auto const& view: swapchain_image_views_) {
            vk::ImageView attachments[] = { *view };

            vk::FramebufferCreateInfo framebuffer_info(
                {},
                *render_pass_,
                1,
                attachments,
                swapchain_extent_.width,
                swapchain_extent_.height,
                1);

            swapchain_framebuffers_.push_back(VK_TRY(device_->createFramebufferUnique(framebuffer_info)));
        }

        return {};
    }

    tl::expected<void, std::string> create_command_pool() {
        auto indices = TRY(find_queue_families(physical_device_, *surface_));

        command_pool_ = VK_TRY(device_->createCommandPoolUnique(vk::CommandPoolCreateInfo({}, *indices.graphics_family)));

        return {};
    }

    tl::expected<uint32_t, std::string> find_memory_type(vk::PhysicalDevice physical_device, uint32_t type_filter, vk::MemoryPropertyFlags properties) {
        auto mem_properties = physical_device.getMemoryProperties();

        for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
            if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        return tl::make_unexpected("Failed to find suitable memory type");
    }

    tl::expected<void, std::string> create_vertex_buffer() {
        vertex_buffer_ = VK_TRY(device_->createBufferUnique(
            vk::BufferCreateInfo(
                {},
                sizeof(vertices[0]) * vertices.size(),
                vk::BufferUsageFlagBits::eVertexBuffer,
                vk::SharingMode::eExclusive)));

        auto requirements = device_->getBufferMemoryRequirements(*vertex_buffer_);
        auto memory_type = TRY(find_memory_type(
            physical_device_,
            requirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));

        vertex_buffer_memory_ = VK_TRY(device_->allocateMemoryUnique(
            vk::MemoryAllocateInfo(
                requirements.size,
                memory_type)));

        device_->bindBufferMemory(*vertex_buffer_, *vertex_buffer_memory_, 0);

        void* data = VK_TRY(device_->mapMemory(*vertex_buffer_memory_, 0, requirements.size));
        memcpy(data, vertices.data(), sizeof(vertices[0]) * vertices.size());
        device_->unmapMemory(*vertex_buffer_memory_);

        return {};
    }

    tl::expected<void, std::string> create_command_buffer() {
        command_buffers_ = VK_TRY(device_->allocateCommandBuffersUnique(
            vk::CommandBufferAllocateInfo(
                *command_pool_,
                vk::CommandBufferLevel::ePrimary,
                swapchain_framebuffers_.size())));

        int ix = 0;
        for (const auto& buffer: command_buffers_) {
            VK_TRY_V(buffer->begin(vk::CommandBufferBeginInfo({}, nullptr)));

            vk::ClearValue clear_color(
                vk::ClearColorValue(
                    std::array<float, 4>({0.0f, 0.0f, 0.0f, 1.0f})));

            buffer->beginRenderPass(
                vk::RenderPassBeginInfo(
                    *render_pass_,
                    *swapchain_framebuffers_[ix],
                    vk::Rect2D({0, 0}, {swapchain_extent_}),
                    1,
                    &clear_color),
                vk::SubpassContents::eInline);

            buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline_);
            std::array<vk::Buffer, 1> vertex_buffers = {*vertex_buffer_};
            std::array<vk::DeviceSize, 1> offsets = {0};
            buffer->bindVertexBuffers(0, vertex_buffers, offsets);
            buffer->draw(3, 1, 0, 0);
            buffer->endRenderPass();

            VK_TRY_V(buffer->end());

            ix++;
        }

        return {};
    }

    tl::expected<void, std::string> create_sync_objects() {
        image_available_semaphores_.reserve(MAX_FRAMES_IN_FLIGHT);
        render_finished_semaphores_.reserve(MAX_FRAMES_IN_FLIGHT);
        in_flight_fences_.reserve(MAX_FRAMES_IN_FLIGHT);
        images_in_flight_.resize(MAX_FRAMES_IN_FLIGHT, nullptr);

        vk::SemaphoreCreateInfo semafore_info;
        vk::FenceCreateInfo fence_info(vk::FenceCreateFlagBits::eSignaled);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            image_available_semaphores_.push_back(VK_TRY(device_->createSemaphoreUnique(semafore_info)));
            render_finished_semaphores_.push_back(VK_TRY(device_->createSemaphoreUnique(semafore_info)));
            in_flight_fences_.push_back(VK_TRY(device_->createFenceUnique(fence_info)));
        }

        return {};
    }

    tl::expected<void, std::string> draw_frame() {
        device_->waitForFences(*in_flight_fences_[current_frame], true, UINT64_MAX);

        auto [result, image_index] = device_->acquireNextImageKHR(*swapchain_, UINT64_MAX, *image_available_semaphores_[current_frame], nullptr);

        if (result == vk::Result::eErrorOutOfDateKHR || framebuffer_resized) {
            framebuffer_resized = false;

            recreate_swapchain();

            return {};
        } else if (result != vk::Result::eSuboptimalKHR) {
            VK_TRY_V(result);
        }

        if (images_in_flight_[image_index]) {
            device_->waitForFences(images_in_flight_[image_index], true, UINT64_MAX);
        }

        images_in_flight_[image_index] = *in_flight_fences_[current_frame];

        vk::Semaphore wait_semaphores[] = {*image_available_semaphores_[current_frame]};
        vk::PipelineStageFlags wait_stages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        vk::Semaphore signal_semaphores[] = {*render_finished_semaphores_[current_frame]};

        device_->resetFences(*in_flight_fences_[current_frame]);

        graphics_queue_.submit(
            vk::SubmitInfo(
                1,
                wait_semaphores,
                wait_stages,
                1,
                &(*command_buffers_[image_index]),
                1,
                signal_semaphores),
            *in_flight_fences_[current_frame]);

        vk::SwapchainKHR swapchains[] = {*swapchain_};

        present_queue_.presentKHR(vk::PresentInfoKHR(
            1,
            signal_semaphores,
            1,
            swapchains,
            &image_index));

        current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

        return {};
    }

private:
    // strict order of vk objects is important :(
    vk::UniqueInstance instance_;
    vk::UniqueDebugUtilsMessengerEXT debug_utils_messenger_;
    vk::PhysicalDevice physical_device_;
    vk::UniqueDevice device_;
    vk::Queue graphics_queue_;
    vk::Queue present_queue_;
    vk::UniqueSurfaceKHR surface_;
    vk::UniqueSwapchainKHR swapchain_;
    std::vector<vk::Image> swapchain_images_;
    vk::Format swapchain_image_format_;
    vk::Extent2D swapchain_extent_;
    std::vector<vk::UniqueImageView> swapchain_image_views_;
    vk::UniqueRenderPass render_pass_;
    vk::UniquePipelineLayout pipeline_layout_;
    vk::UniquePipeline pipeline_;
    std::vector<vk::UniqueFramebuffer> swapchain_framebuffers_;
    vk::UniqueCommandPool command_pool_;
    vk::UniqueBuffer vertex_buffer_;
    vk::UniqueDeviceMemory vertex_buffer_memory_;
    std::vector<vk::UniqueCommandBuffer> command_buffers_;
    std::vector<vk::UniqueSemaphore> image_available_semaphores_;
    std::vector<vk::UniqueSemaphore> render_finished_semaphores_;
    std::vector<vk::UniqueFence> in_flight_fences_;
    std::vector<vk::Fence> images_in_flight_;

    std::shared_ptr<glfw::context> context_;
    std::unique_ptr<glfw::window> window_;

    int current_frame = 0;

public:
    bool framebuffer_resized;
};

int main() {
    base_application app;


    auto res = app.run();

    if (!res) {
        logger::system->critical(res.error());
    }

    /*auto debug_create_info = vk::DebugReportCallbackCreateInfoEXT(
        vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning,
        &debug_callback
    );*/


    return 0;
}
