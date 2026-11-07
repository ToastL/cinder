#include "Dev/ReferencePick.hpp"

#include "Scene/Actor.hpp"
#include "Scene/Component.hpp"

namespace cinder::Dev {

bool ReferencePick::take(cinder::Scene::Object& object) {
    if (!armed() || object.destroyed()) return false;
    Accept accept = accept_;
    if (!accept(object)) return false;
    cancel();
    return true;
}

bool ReferencePick::offer(cinder::Scene::Actor& actor, cinder::Scene::Component* hit) {
    if (hit != nullptr && take(*hit)) return true;
    if (take(actor)) return true;
    for (cinder::Scene::Component* component : actor.components()) {
        if (component != hit && take(*component)) return true;
    }
    return false;
}

}
