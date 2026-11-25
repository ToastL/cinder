#include "CloudComponent.hpp"

#include "Game/Game.hpp"
#include "Gfx/RHI/Backend.hpp"
#include "Gfx/RHI/Commands.hpp"
#include "Gfx/RHI/PipelineBuilder.hpp"
#include "Scene/Transform.hpp"

#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cmath>
#include <cstdint>

namespace {

constexpr std::uint32_t PUSH_CONSTANT_BYTES = 176;
constexpr std::uint32_t FULLSCREEN_TRIANGLE_VERTICES = 3;
constexpr float DEG_TO_RAD = 3.14159265f / 180.0f;

struct Push {
    glm::mat4 invViewProj;
    glm::mat4 invWorld;
    glm::vec4 cameraTime;
    glm::vec4 sunCoverage;
    glm::vec4 extras;
};

static_assert(sizeof(Push) == PUSH_CONSTANT_BYTES, "clouds Push layout drift");

}

CloudComponent::CloudComponent() {
    pipeline_ = cinder::Gfx::RHI::PipelineBuilder(cinder::Game::presenter(),
                                                   cinder::Gfx::RHI::PassKind::Overlay)
                        .shader("clouds")
                        .pushConstants(PUSH_CONSTANT_BYTES, cinder::Gfx::RHI::ShaderStages::Fragment)
                        .textureSlots(1)
                        .premultipliedBlend()
                        .build();

    cinder::Gfx::RHI::GraphicsPipeline* pipeline = pipeline_.get();
    CloudComponent* self = this;
    cinder::Game::setOverlayPaint([pipeline, self](cinder::Gfx::RHI::Commands cmd, std::uint32_t,
                                                   const glm::mat4& viewProjection,
                                                   cinder::Gfx::RHI::TextureBinding depth) {
        const float yaw = self->sunYaw_ * DEG_TO_RAD;
        const float pitch = self->sunPitch_ * DEG_TO_RAD;
        const glm::vec3 sun(std::cos(pitch) * std::cos(yaw),
                            std::sin(pitch),
                            std::cos(pitch) * std::sin(yaw));
        const glm::vec3 camPos = cinder::Game::cameraWorldPosition();
        const float time = static_cast<float>(cinder::Game::time());

        Push push{};
        push.invViewProj = glm::inverse(viewProjection);
        push.invWorld = glm::inverse(self->transform().world());
        push.cameraTime = glm::vec4(camPos, time);
        push.sunCoverage = glm::vec4(sun, self->coverage_);
        push.extras = glm::vec4(self->density_, self->windSpeed_, 0.0f, 0.0f);

        pipeline->bind(cmd);
        pipeline->bindTexture(cmd, 0, depth);
        pipeline->push(cmd, cinder::Gfx::RHI::ShaderStages::Fragment, sizeof(Push), &push);
        cinder::Gfx::RHI::draw(cmd, FULLSCREEN_TRIANGLE_VERTICES);
    });
}

CloudComponent::~CloudComponent() { cinder::Game::setOverlayPaint(nullptr); }

CINDER_REGISTER(CloudComponent);
