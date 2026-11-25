#pragma once

#include "Scene/Component.hpp"

#include <memory>

namespace cinder::Gfx { class OffscreenPass; }

class CloudComponent final : public cinder::Scene::Component {
public:
    CINDER_COMPONENT(CloudComponent, cinder::Scene::Component) {
        CINDER_PROP_S(sunYaw_, -180.0f, 180.0f, 1.0f);
        CINDER_PROP_S(sunPitch_, -90.0f, 90.0f, 1.0f);
        CINDER_PROP_S(bottom_, -1000.0f, 10000.0f, 0.5f);
        CINDER_PROP_S(top_, -1000.0f, 10000.0f, 0.5f);
        CINDER_PROP_R(coverage_, 0.0f, 1.0f);
        CINDER_PROP_R(density_, 0.0f, 5.0f);
        CINDER_PROP_R(windSpeed_, -10.0f, 10.0f);
        CINDER_PROP_S(weatherScale_, 0.0001f, 1.0f, 0.001f);
        CINDER_PROP_S(shapeScale_, 0.0001f, 1.0f, 0.001f);
        CINDER_PROP_S(detailScale_, 0.0001f, 4.0f, 0.005f);
        CINDER_PROP_S(maxDistance_, 1.0f, 100000.0f, 1.0f);
        CINDER_PROP_R(resolutionScale_, 1, 4);
    }

    CloudComponent();
    ~CloudComponent() override;

    CloudComponent(const CloudComponent&) = delete;
    CloudComponent& operator=(const CloudComponent&) = delete;

    void onRender(float alpha, cinder::Scene::DrawList& draws) override;

private:
    struct Gpu;

    void createGpu();
    void march(cinder::Gfx::OffscreenPass& pass);

    float sunYaw_ = -45.0f;
    float sunPitch_ = 25.0f;
    float bottom_ = 10.0f;
    float top_ = 34.0f;
    float coverage_ = 0.6f;
    float density_ = 1.0f;
    float windSpeed_ = 0.5f;
    float weatherScale_ = 0.01f;
    float shapeScale_ = 0.04f;
    float detailScale_ = 0.3f;
    float maxDistance_ = 300.0f;
    int resolutionScale_ = 2;

    std::unique_ptr<Gpu> gpu_;
};
