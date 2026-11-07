#pragma once

#include "Game/Game.hpp"

#include <map>
#include <string>

inline std::map<std::string, double>& selftestMarks() {
    static std::map<std::string, double> marks;
    return marks;
}

inline int& selftestTicks() {
    static int ticks = 0;
    return ticks;
}

class Marker final : public cinder::Scene::Component {
public:
    CINDER_COMPONENT(Marker, cinder::Scene::Component) {}

    void beginPlay() override {
        cinder::Scene::Actor* marked = owner();
        if (marked == nullptr) return;
        const cinder::Scene::PropValue* mark = marked->attribute("mark");
        if (mark == nullptr) return;
        selftestMarks()[marked->name()] = mark->is<double>() ? mark->as<double>()
                                                             : static_cast<double>(mark->as<std::int64_t>());
    }
};

class Ticker final : public cinder::Scene::Component {
public:
    CINDER_COMPONENT(Ticker, cinder::Scene::Component) {}

    void tick(float dt) override { selftestTicks()++; }
};
