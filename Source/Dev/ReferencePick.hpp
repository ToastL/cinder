#pragma once

#include <functional>
#include <utility>

namespace cinder::Scene {
class Actor;
class Component;
class Object;
}

namespace cinder::Dev {

class ReferencePick {
public:
    using Accept = std::function<bool(cinder::Scene::Object&)>;

    void arm(const void* key, Accept accept) {
        key_ = key;
        accept_ = std::move(accept);
    }
    void cancel() {
        key_ = nullptr;
        accept_ = nullptr;
    }

    bool armed() const { return accept_ != nullptr; }
    bool armedBy(const void* key) const { return armed() && key_ == key; }

    bool offer(cinder::Scene::Actor& actor, cinder::Scene::Component* hit = nullptr);

private:
    bool take(cinder::Scene::Object& object);

    const void* key_ = nullptr;
    Accept accept_;
};

}
