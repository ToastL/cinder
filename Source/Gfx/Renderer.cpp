#include "Gfx/Renderer.hpp"

#include "Gfx/RHI/Commands.hpp"
#include "Gfx/RHI/Backend.hpp"

#include "Gfx/RendererDrawList.hpp"
#include "Gfx/Pass/MeshPass.hpp"
#include "Gfx/Pass/SpritePass.hpp"
#include "Platform/Window.hpp"
#include "Scene/DrawList.hpp"

#include <algorithm>
#include <cmath>

namespace cinder::Gfx {

using cinder::Gfx::Asset::Assets;
using cinder::Gfx::Pass::DrawPass;
using cinder::Gfx::RHI::Ctx;

Renderer::Renderer(const Ctx& ctx, cinder::Platform::Window& window) : ctx_(ctx), window_(window) {
    presenter_ = std::make_unique<cinder::Gfx::RHI::Presenter>(ctx, window, FRAMES_IN_FLIGHT);
    presenter_->onRecreated([this](bool formatChanged) {
        if (formatChanged) ui_->rebuild();
        createTargets();
        resizeCameras();
    });

    createTargets();
    assets_ = std::make_unique<Assets>(ctx);
    spritePipeline_ = std::make_unique<cinder::Gfx::Pass::SpritePipeline>(*presenter_);
    meshPipeline_ = std::make_unique<cinder::Gfx::Pass::MeshPipeline>(*presenter_);
    compositePipeline_ = std::make_unique<CompositePipeline>(*presenter_);
    ui_ = std::make_unique<UiRenderer>(ctx, *assets_, *presenter_, FRAMES_IN_FLIGHT);

    auto meshPass = std::make_unique<cinder::Gfx::Pass::MeshPass>(ctx, *assets_, *meshPipeline_);
    auto spritePass = std::make_unique<cinder::Gfx::Pass::SpritePass>(
            ctx, *assets_, *spritePipeline_, FRAMES_IN_FLIGHT);

    draws_ = std::make_unique<RendererDrawList>(*this, *meshPass, *spritePass);
    sprites_ = spritePass.get();
    meshes_ = meshPass.get();

    passes_.push_back(std::move(meshPass));
    passes_.push_back(std::move(spritePass));

    resizeCameras();
}

glm::uvec2 Renderer::targetExtent() const {
    if (!embedded()) return presenter_->extent();

    const float scale = pixelsPerPoint();
    const auto pixels = [scale](int points) {
        return static_cast<uint32_t>(std::max(1L, std::lround(static_cast<float>(points) * scale)));
    };
    return {pixels(viewportWidth_), pixels(viewportHeight_)};
}

void Renderer::createTargets() {
    targets_.recreate(*presenter_, targetExtent(), FRAMES_IN_FLIGHT);
}

glm::vec2 Renderer::viewSize() const {
    if (embedded()) return glm::vec2(viewportWidth_, viewportHeight_);
    return glm::vec2(window_.logicalWidth(), window_.logicalHeight());
}

void Renderer::resizeCameras() {
    const glm::vec2 size = viewSize();
    camera_.setViewSize(size.x, size.y);
    if (override_) override_->setViewSize(size.x, size.y);
}

void Renderer::setClearColor(float r, float g, float b) {
    clearR_ = r;
    clearG_ = g;
    clearB_ = b;
}

void Renderer::setViewportSize(int width, int height) {
    if (width == viewportWidth_ && height == viewportHeight_) return;

    ctx_.waitIdle();
    viewportWidth_ = width;
    viewportHeight_ = height;
    createTargets();
    resizeCameras();
}

void Renderer::overrideCamera(const cinder::Scene::View& view) {
    if (!override_) override_.emplace(camera_);
    override_->setView(view);
}

void Renderer::releaseCamera() { override_.reset(); }

void Renderer::beginFrame() {
    for (const std::unique_ptr<DrawPass>& pass : passes_) pass->beginFrame();

    uiElements_.reset(glm::vec2(window_.logicalWidth(), window_.logicalHeight()), pixelsPerPoint());
    if (uiPaint_) uiPaint_(uiElements_);
}

void Renderer::drawFrame() {
    std::optional<cinder::Gfx::RHI::Frame> frame = presenter_->begin();
    if (!frame) return;

    lastFrame_ = frame->index;
    cinder::Gfx::RHI::RenderTarget& target = targets_.at(frame->index);
    const glm::uvec2 window = presenter_->extent();

    ui_->prepare(frame->uploads, frame->index, uiElements_);

    presenter_->beginScenePass(*frame, target, clearR_, clearG_, clearB_);
    cinder::Gfx::RHI::viewport(frame->commands, target.width(), target.height());
    cinder::Gfx::RHI::scissor(frame->commands, 0, 0, target.width(), target.height());
    const glm::mat4& viewProjection = override_ ? override_->viewProjection() : camera_.viewProjection();
    for (const std::unique_ptr<DrawPass>& pass : passes_) {
        pass->record(frame->commands, frame->index, viewProjection);
    }
    presenter_->endPass(*frame);

    presenter_->beginPresentPass(*frame, clearR_, clearG_, clearB_);
    cinder::Gfx::RHI::viewport(frame->commands, window.x, window.y);
    cinder::Gfx::RHI::scissor(frame->commands, 0, 0, window.x, window.y);
    if (!embedded()) compositePipeline_->draw(frame->commands, target.binding());
    ui_->record(frame->commands, frame->index, window, target.binding());
    presenter_->endPass(*frame);

    presenter_->end(*frame);
}

void Renderer::capture(const std::string& path) {
    presenter_->captureTarget(targets_.at(lastFrame_), path);
}

float Renderer::pixelsPerPoint() const { return presenter_->pixelsPerPoint(); }

void Renderer::requestWindowCapture(const std::string& path) {
    presenter_->requestWindowCapture(path);
}

cinder::Scene::DrawList& Renderer::draws() { return *draws_; }

Renderer::~Renderer() { ctx_.waitIdle(); }

}
