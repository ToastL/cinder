#pragma once

#include "Reflect/PropType.hpp"
#include "Scene/Object.hpp"
#include "Scene/Scene.hpp"

namespace cinder::Scene {

template <class T>
struct Ref : cinder::Reflect::RefBase {
    Ref() = default;
    explicit Ref(const T* object) { set(object); }

    static bool accepts(const void* object) {
        return dynamic_cast<const T*>(static_cast<const Object*>(object)) != nullptr;
    }

    T* get(const Scene& scene) const {
        if (id == 0) return nullptr;
        Object* object = scene.byId(id);
        if (object == nullptr || object->destroyed()) return nullptr;
        return dynamic_cast<T*>(object);
    }

    void set(const T* object) { id = object == nullptr ? 0 : object->id(); }
    void reset() { id = 0; }
    explicit operator bool() const { return id != 0; }
};

template <class Fn>
void forEachRef(Object& object, Fn&& fn) {
    for (const cinder::Reflect::PropDef& def : object.propList()) {
        if (def.type() == cinder::Reflect::PropType::Ref) fn(def, object.propTarget());
    }
}

}
