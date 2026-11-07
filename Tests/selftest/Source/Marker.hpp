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

class Marker final : public cinder::Scene::Node {
public:
    CINDER_NODE(Marker, cinder::Scene::Node) {}

    void onStart() override {
        cinder::Scene::Node* marked = parent();
        if (marked == nullptr) return;
        const cinder::Scene::PropValue* mark = marked->attribute("mark");
        if (mark == nullptr) return;
        selftestMarks()[marked->name()] = mark->is<double>() ? mark->as<double>()
                                                             : static_cast<double>(mark->as<std::int64_t>());
    }
};

class Ticker final : public cinder::Scene::Node {
public:
    CINDER_NODE(Ticker, cinder::Scene::Node) {}

    void onUpdate(float dt) override { selftestTicks()++; }
};
