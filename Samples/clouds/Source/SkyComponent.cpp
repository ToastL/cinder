#include "SkyComponent.hpp"

#include "Game/Game.hpp"
#include "Gfx/RHI/Backend.hpp"
#include "Gfx/RHI/Commands.hpp"
#include "Gfx/RHI/PipelineBuilder.hpp"

#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cmath>
#include <cstdint>

namespace {

constexpr std::uint32_t PUSH_CONSTANT_BYTES = 80;
constexpr std::uint32_t FULLSCREEN_TRIANGLE_VERTICES = 3;
constexpr float DEG_TO_RAD = 3.14159265f / 180.0f;

struct Push {
    glm::mat4 invViewProj;
    glm::vec4 sunDir;
};

static_assert(sizeof(Push) == PUSH_CONSTANT_BYTES, "sky Push layout drift");

SkyComponent* active = nullptr;

}

SkyComponent::SkyComponent() = default;

void SkyComponent::onRender(float, cinder::Scene::DrawList&) {
    if (!pipeline_) createPipeline();
}

void SkyComponent::createPipeline() {
    const cinder::Gfx::RHI::Presenter& presenter = cinder::Game::presenter();
    ctx_ = &presenter.ctx();
    pipeline_ = cinder::Gfx::RHI::PipelineBuilder(presenter, cinder::Gfx::RHI::PassKind::Scene)
                        .shader("sky")
                        .pushConstants(PUSH_CONSTANT_BYTES, cinder::Gfx::RHI::ShaderStages::Fragment)
                        .build();

    active = this;
    cinder::Game::setPostPaint([this](cinder::Gfx::RHI::Commands cmd, std::uint32_t,
                                      const glm::mat4& viewProjection) {
        const float yaw = sunYaw_ * DEG_TO_RAD;
        const float pitch = sunPitch_ * DEG_TO_RAD;
        const glm::vec3 sun(std::cos(pitch) * std::cos(yaw),
                            std::sin(pitch),
                            std::cos(pitch) * std::sin(yaw));

        Push push{};
        push.invViewProj = glm::inverse(viewProjection);
        push.sunDir = glm::vec4(sun, 0.0f);

        pipeline_->bind(cmd);
        pipeline_->push(cmd, cinder::Gfx::RHI::ShaderStages::Fragment, sizeof(Push), &push);
        cinder::Gfx::RHI::draw(cmd, FULLSCREEN_TRIANGLE_VERTICES);
    });
}

SkyComponent::~SkyComponent() {
    if (ctx_ != nullptr) ctx_->waitIdle();
    if (active != this) return;
    cinder::Game::setPostPaint(nullptr);
    active = nullptr;
}

CINDER_REGISTER(SkyComponent);
