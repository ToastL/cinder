#include "dev/ImGuiLayer.hpp"

#include "gfx/vk/VkCtx.hpp"
#include "gfx/vk/VkUtil.hpp"
#include "platform/Window.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <memory>

namespace cinder::dev {
namespace {

void checkResult(VkResult result) { cinder::gfx::vk::check(result, "imgui"); }

}

ImGuiLayer::ImGuiLayer(const cinder::gfx::vk::VkCtx& ctx, cinder::platform::Window& window,
                       VkRenderPass renderPass, uint32_t minImageCount, uint32_t imageCount)
    : ctx_(ctx) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(window.handle(), true);

    ImGui_ImplVulkan_InitInfo info{};
    info.ApiVersion = VK_API_VERSION_1_2;
    info.Instance = ctx.instance();
    info.PhysicalDevice = ctx.physicalDevice();
    info.Device = ctx.device();
    info.QueueFamily = ctx.graphicsFamily();
    info.Queue = ctx.graphicsQueue();
    info.DescriptorPoolSize = 64;
    info.MinImageCount = minImageCount;
    info.ImageCount = imageCount;
    info.PipelineInfoMain.RenderPass = renderPass;
    info.PipelineInfoMain.Subpass = 0;
    info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    info.CheckVkResultFn = checkResult;

    ImGui_ImplVulkan_Init(&info);
}

void ImGuiLayer::beginFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    building_ = true;
}

void ImGuiLayer::record(VkCommandBuffer cmd) {
    if (!building_) return;

    ImGui::Render();
    building_ = false;
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

void ImGuiLayer::discardFrame() {
    if (!building_) return;

    ImGui::EndFrame();
    building_ = false;
}

void ImGuiLayer::setMinImageCount(uint32_t minImageCount) {
    ImGui_ImplVulkan_SetMinImageCount(minImageCount);
}

bool ImGuiLayer::capturesMouse() const { return ImGui::GetIO().WantCaptureMouse; }

bool ImGuiLayer::capturesKeyboard() const { return ImGui::GetIO().WantCaptureKeyboard; }

ImGuiLayer::~ImGuiLayer() {
    ctx_.waitIdle();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

cinder::gfx::OverlayFactory overlayFactory() {
    return [](const cinder::gfx::vk::VkCtx& ctx, cinder::platform::Window& window,
              VkRenderPass renderPass, uint32_t minImageCount, uint32_t imageCount) {
        return std::make_unique<ImGuiLayer>(ctx, window, renderPass, minImageCount, imageCount);
    };
}

}
