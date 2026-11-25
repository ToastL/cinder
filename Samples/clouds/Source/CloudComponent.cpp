#include "CloudComponent.hpp"

#include "CloudNoise.hpp"

#include "Game/Game.hpp"
#include "Gfx/OffscreenPass.hpp"
#include "Gfx/RHI/Backend.hpp"
#include "Gfx/RHI/Commands.hpp"
#include "Gfx/RHI/PipelineBuilder.hpp"

#include <glm/common.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cmath>
#include <cstdint>

namespace {

constexpr std::uint32_t FULLSCREEN_TRIANGLE_VERTICES = 3;
constexpr float DEG_TO_RAD = 3.14159265f / 180.0f;

constexpr std::uint32_t DEPTH_SLOT = 0;
constexpr std::uint32_t SHAPE_SLOT = 1;
constexpr std::uint32_t DETAIL_SLOT = 2;
constexpr std::uint32_t WEATHER_SLOT = 3;
constexpr std::uint32_t TEXTURE_SLOTS = 4;

struct Push {
    glm::mat4 invViewProj;
    glm::vec4 cameraTime;
    glm::vec4 sunCoverage;
    glm::vec4 layer;
    glm::vec4 scales;
};

static_assert(sizeof(Push) == 128, "clouds Push layout drift");

struct AccumulatePush {
    glm::mat4 reproject;
    glm::vec4 params;
};

constexpr std::uint32_t FRESH_SLOT = 0;
constexpr std::uint32_t HISTORY_SLOT = 1;
constexpr std::uint32_t FRAME_OFFSETS = 4;

CloudComponent* active = nullptr;

}

struct CloudComponent::Gpu {
    Gpu(const cinder::Gfx::RHI::Ctx& context, const CloudNoise& noise)
        : ctx(&context),
          shape(context, noise.shape.data(), CloudNoise::SHAPE_SIZE, CloudNoise::SHAPE_SIZE, CloudNoise::SHAPE_SIZE,
                cinder::Gfx::RHI::Format::RGBA8Unorm),
          detail(context, noise.detail.data(), CloudNoise::DETAIL_SIZE, CloudNoise::DETAIL_SIZE, CloudNoise::DETAIL_SIZE,
                 cinder::Gfx::RHI::Format::RGBA8Unorm),
          weather(context, noise.weather.data(), CloudNoise::WEATHER_SIZE, CloudNoise::WEATHER_SIZE, 1,
                  cinder::Gfx::RHI::Format::RGBA8Unorm),
          pool(context, 3, cinder::Gfx::RHI::SamplerDesc{cinder::Gfx::RHI::SamplerFilter::Linear,
                                                     cinder::Gfx::RHI::SamplerAddress::Repeat}),
          shapeBinding(pool.bind(shape, SHAPE_SLOT)),
          detailBinding(pool.bind(detail, DETAIL_SLOT)),
          weatherBinding(pool.bind(weather, WEATHER_SLOT)) {}

    ~Gpu() { ctx->waitIdle(); }

    Gpu(const Gpu&) = delete;
    Gpu& operator=(const Gpu&) = delete;

    const cinder::Gfx::RHI::Ctx* ctx;
    cinder::Gfx::RHI::VolumeTexture shape;
    cinder::Gfx::RHI::VolumeTexture detail;
    cinder::Gfx::RHI::VolumeTexture weather;
    cinder::Gfx::RHI::TexturePool pool;
    cinder::Gfx::RHI::TextureBinding shapeBinding;
    cinder::Gfx::RHI::TextureBinding detailBinding;
    cinder::Gfx::RHI::TextureBinding weatherBinding;
    std::unique_ptr<cinder::Gfx::RHI::GraphicsPipeline> march;
    std::unique_ptr<cinder::Gfx::RHI::GraphicsPipeline> accumulate;
    std::unique_ptr<cinder::Gfx::RHI::GraphicsPipeline> composite;
    std::unique_ptr<cinder::Gfx::RHI::RenderTarget> fresh;
    std::unique_ptr<cinder::Gfx::RHI::RenderTarget> history[2];
    glm::uvec2 lowSize{0, 0};
    glm::mat4 previousViewProjection{1.0f};
    std::uint32_t frame = 0;
    bool historyValid = false;
};

CloudComponent::CloudComponent() = default;

CloudComponent::~CloudComponent() {
    if (active != this) return;
    cinder::Game::setOffscreenPaint(nullptr);
    cinder::Game::setOverlayPaint(nullptr);
    active = nullptr;
}

void CloudComponent::onRender(float, cinder::Scene::DrawList&) {
    if (!gpu_) createGpu();
}

void CloudComponent::createGpu() {
    const cinder::Gfx::RHI::Presenter& presenter = cinder::Game::presenter();
    gpu_ = std::make_unique<Gpu>(presenter.ctx(), cloudNoise());
    gpu_->march = cinder::Gfx::RHI::PipelineBuilder(presenter, cinder::Gfx::RHI::PassKind::Scene)
                          .shader("clouds")
                          .pushConstants(sizeof(Push), cinder::Gfx::RHI::ShaderStages::Fragment)
                          .textureSlots(TEXTURE_SLOTS)
                          .build();
    gpu_->accumulate = cinder::Gfx::RHI::PipelineBuilder(presenter, cinder::Gfx::RHI::PassKind::Scene)
                               .shader("cloudsAccumulate")
                               .pushConstants(sizeof(AccumulatePush), cinder::Gfx::RHI::ShaderStages::Fragment)
                               .textureSlots(2)
                               .build();
    gpu_->composite = cinder::Gfx::RHI::PipelineBuilder(presenter, cinder::Gfx::RHI::PassKind::Overlay)
                              .shader("cloudsComposite")
                              .textureSlots(1)
                              .premultipliedBlend()
                              .build();

    active = this;
    cinder::Game::setOffscreenPaint([this](cinder::Gfx::OffscreenPass& pass) { march(pass); });
    cinder::Game::setOverlayPaint([this](cinder::Gfx::RHI::Commands cmd, std::uint32_t, const glm::mat4&,
                                         cinder::Gfx::RHI::TextureBinding) {
        const Gpu& gpu = *gpu_;
        const cinder::Gfx::RHI::RenderTarget* current = gpu.history[gpu.frame & 1u].get();
        if (current == nullptr) return;
        gpu.composite->bind(cmd);
        gpu.composite->bindTexture(cmd, current->binding());
        cinder::Gfx::RHI::draw(cmd, FULLSCREEN_TRIANGLE_VERTICES);
    });
}

void CloudComponent::march(cinder::Gfx::OffscreenPass& pass) {
    Gpu& gpu = *gpu_;
    const cinder::Gfx::RHI::Presenter& presenter = cinder::Game::presenter();
    const std::uint32_t scale = static_cast<std::uint32_t>(resolutionScale_);
    const glm::uvec2 lowSize = glm::max((pass.sceneSize() + scale - 1u) / scale, glm::uvec2(1));
    if (lowSize != gpu.lowSize) {
        gpu.ctx->waitIdle();
        gpu.lowSize = lowSize;
        gpu.fresh = presenter.createTarget((lowSize + 1u) / 2u);
        gpu.history[0] = presenter.createTarget(lowSize);
        gpu.history[1] = presenter.createTarget(lowSize);
        gpu.historyValid = false;
    }

    gpu.frame += 1;
    const std::uint32_t offset = gpu.frame % FRAME_OFFSETS;
    const glm::mat4& viewProjection = pass.viewProjection();
    const glm::mat4 inverseViewProjection = glm::inverse(viewProjection);
    const glm::vec3 cameraPos = cinder::Game::cameraWorldPosition();

    const float yaw = sunYaw_ * DEG_TO_RAD;
    const float pitch = sunPitch_ * DEG_TO_RAD;
    const glm::vec3 sun(std::cos(pitch) * std::cos(yaw), std::sin(pitch), std::cos(pitch) * std::sin(yaw));

    Push push{};
    push.invViewProj = inverseViewProjection;
    push.cameraTime = glm::vec4(cameraPos, windSpeed_ * static_cast<float>(cinder::Game::time()));
    push.sunCoverage = glm::vec4(sun, coverage_);
    push.layer = glm::vec4(bottom_, top_, maxDistance_, density_);
    push.scales = glm::vec4(weatherScale_, shapeScale_, detailScale_, static_cast<float>(scale * FRAME_OFFSETS + offset));

    const cinder::Gfx::RHI::Commands cmd = pass.commands();
    pass.begin(*gpu.fresh);
    gpu.march->bind(cmd);
    gpu.march->bindTexture(cmd, DEPTH_SLOT, pass.sceneDepth());
    gpu.march->bindTexture(cmd, SHAPE_SLOT, gpu.shapeBinding);
    gpu.march->bindTexture(cmd, DETAIL_SLOT, gpu.detailBinding);
    gpu.march->bindTexture(cmd, WEATHER_SLOT, gpu.weatherBinding);
    gpu.march->push(cmd, cinder::Gfx::RHI::ShaderStages::Fragment, sizeof(Push), &push);
    cinder::Gfx::RHI::draw(cmd, FULLSCREEN_TRIANGLE_VERTICES);
    pass.end();

    const glm::vec4 nearPoint = inverseViewProjection * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    const glm::vec4 farPoint = inverseViewProjection * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    const glm::vec3 forward = glm::normalize(glm::vec3(farPoint) / farPoint.w - glm::vec3(nearPoint) / nearPoint.w);
    const float thickness = top_ - bottom_;
    const float distance = glm::clamp(std::abs((bottom_ + top_) * 0.5f - cameraPos.y) * 3.0f, thickness, maxDistance_);
    const glm::vec4 representative = viewProjection * glm::vec4(cameraPos + forward * distance, 1.0f);

    AccumulatePush accumulate{};
    accumulate.reproject = gpu.previousViewProjection * inverseViewProjection;
    accumulate.params = glm::vec4(representative.z / representative.w, static_cast<float>(offset),
                                  gpu.historyValid ? 1.0f : 0.0f, 0.0f);

    const std::uint32_t current = gpu.frame & 1u;
    pass.begin(*gpu.history[current]);
    gpu.accumulate->bind(cmd);
    gpu.accumulate->bindTexture(cmd, FRESH_SLOT, gpu.fresh->binding(FRESH_SLOT));
    const cinder::Gfx::RHI::RenderTarget& previous = gpu.historyValid ? *gpu.history[current ^ 1u] : *gpu.fresh;
    gpu.accumulate->bindTexture(cmd, HISTORY_SLOT, previous.binding(HISTORY_SLOT));
    gpu.accumulate->push(cmd, cinder::Gfx::RHI::ShaderStages::Fragment, sizeof(AccumulatePush), &accumulate);
    cinder::Gfx::RHI::draw(cmd, FULLSCREEN_TRIANGLE_VERTICES);
    pass.end();

    gpu.previousViewProjection = viewProjection;
    gpu.historyValid = true;
}

CINDER_REGISTER(CloudComponent);
