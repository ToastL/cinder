#include "Dev/Panels/OutlinerClipboard.hpp"

#include "Platform/Log.hpp"
#include "Scene/Actor.hpp"
#include "Serial/SceneCodec.hpp"

#include <exception>

namespace cinder::Dev::Panels {

std::string OutlinerClipboard::encode(cinder::Scene::Scene& scene,
                                      std::span<const cinder::Scene::Actor* const> roots) {
    if (roots.empty()) return {};
    std::string text(MAGIC);
    text += cinder::Serial::SceneCodec::saveFragment(scene, roots);
    return text;
}

std::vector<cinder::Scene::Actor*> OutlinerClipboard::decode(const std::string& text,
                                                              cinder::Scene::Scene& scene,
                                                              cinder::Scene::Actor* parent) {
    if (!recognizes(text)) return {};
    try {
        return cinder::Serial::SceneCodec::loadFragment(text.substr(MAGIC.size()), scene, parent);
    } catch (const std::exception& error) {
        cinder::Platform::logError("[outliner] paste: %s\n", error.what());
        return {};
    }
}

bool OutlinerClipboard::recognizes(std::string_view text) {
    return text.size() >= MAGIC.size() && text.substr(0, MAGIC.size()) == MAGIC;
}

}
