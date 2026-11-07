#pragma once

#include "Gfx/RHI/Fwd.hpp"

#include "Gfx/CompositePipeline.hpp"
#include "Gfx/FrameTargets.hpp"
#include "Gfx/UiRenderer.hpp"
#include "Gfx/Asset/Assets.hpp"
#include "Gfx/Pass/DrawPass.hpp"
#include "Gfx/Pass/MeshPipeline.hpp"
#include "Gfx/Pass/SpritePipeline.hpp"
#include "Gfx/Pass/ViewCamera.hpp"
#include "UI/Core/ElementList.hpp"

#include <glm/vec2.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cinder::Platform { class Window; }
namespace cinder::Gfx::Pass {
class MeshPass;
class SpritePass;
}
namespace cinder::Scene { class DrawList; }

namespace cinder::Gfx {

class Renderer {
public:
    static constexpr uint32_t FRAMES_IN_FLIGHT = 2;

    Renderer(const cinder::Gfx::RHI::Ctx& ctx, cinder::Platform::Window& window);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void setClearColor(float r, float g, float b);
    void setViewportSize(int width, int height);
    void overrideCamera(const cinder::Scene::View& view);
    void releaseCamera();
    void beginFrame();
    void drawFrame();
    void capture(const std::string& path);
    void requestWindowCapture(const std::string& path);

    cinder::Gfx::Asset::Assets& assets() { return *assets_; }
    cinder::Gfx::Pass::ViewCamera& camera() { return camera_; }
    cinder::Gfx::Pass::SpritePass& sprites() { return *sprites_; }
    cinder::Gfx::Pass::MeshPass& meshes() { return *meshes_; }
    UiRenderer& ui() { return *ui_; }
    float pixelsPerPoint() const;
    cinder::Scene::DrawList& draws();

    void setUiPaint(std::function<void(cinder::UI::ElementList&)> paint) { uiPaint_ = std::move(paint); }


private:
    bool embedded() const { return viewportWidth_ > 0; }
    glm::vec2 viewSize() const;
    glm::uvec2 targetExtent() const;
    void createTargets();
    void resizeCameras();

    const cinder::Gfx::RHI::Ctx& ctx_;
    cinder::Platform::Window& window_;

    std::unique_ptr<cinder::Gfx::RHI::Presenter> presenter_;

    FrameTargets targets_;
    std::unique_ptr<cinder::Gfx::Asset::Assets> assets_;
    std::unique_ptr<cinder::Gfx::Pass::SpritePipeline> spritePipeline_;
    std::unique_ptr<cinder::Gfx::Pass::MeshPipeline> meshPipeline_;
    std::unique_ptr<CompositePipeline> compositePipeline_;
    std::unique_ptr<UiRenderer> ui_;
    std::vector<std::unique_ptr<cinder::Gfx::Pass::DrawPass>> passes_;
    cinder::Gfx::Pass::SpritePass* sprites_ = nullptr;
    cinder::Gfx::Pass::MeshPass* meshes_ = nullptr;
    std::unique_ptr<cinder::Scene::DrawList> draws_;
    std::function<void(cinder::UI::ElementList&)> uiPaint_;
    cinder::UI::ElementList uiElements_;

    cinder::Gfx::Pass::ViewCamera camera_;
    std::optional<cinder::Gfx::Pass::ViewCamera> override_;

    uint32_t lastFrame_ = 0;
    int viewportWidth_ = 0;
    int viewportHeight_ = 0;

    float clearR_ = 0.02f;
    float clearG_ = 0.02f;
    float clearB_ = 0.05f;
};

}
