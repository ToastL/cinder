#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace cinder::Scene {
class Actor;
class Scene;
}

namespace cinder::Dev::Panels {

class OutlinerClipboard {
public:
    static constexpr std::string_view MAGIC = "cinder-clipboard 1\n";

    static std::string encode(cinder::Scene::Scene& scene, std::span<const cinder::Scene::Actor* const> roots);
    static std::vector<cinder::Scene::Actor*> decode(const std::string& text, cinder::Scene::Scene& scene,
                                                     cinder::Scene::Actor* parent);
    static bool recognizes(std::string_view text);
};

}
