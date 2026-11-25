#pragma once

#include "Gfx/RHI/Fwd.hpp"
#include "Scene/Component.hpp"

#include <memory>

class SkyComponent final : public cinder::Scene::Component {
public:
    CINDER_COMPONENT(SkyComponent, cinder::Scene::Component) {
        CINDER_PROP_S(sunYaw_, -180.0f, 180.0f, 1.0f);
        CINDER_PROP_S(sunPitch_, -90.0f, 90.0f, 1.0f);
    }

    SkyComponent();
    ~SkyComponent() override;

    SkyComponent(const SkyComponent&) = delete;
    SkyComponent& operator=(const SkyComponent&) = delete;

    float sunYaw() const { return sunYaw_; }
    float sunPitch() const { return sunPitch_; }

private:
    float sunYaw_ = -45.0f;
    float sunPitch_ = 25.0f;

    std::unique_ptr<cinder::Gfx::RHI::GraphicsPipeline> pipeline_;
};
