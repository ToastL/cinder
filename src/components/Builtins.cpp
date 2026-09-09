#include "components/Builtins.hpp"

#include "components/Camera.hpp"
#include "components/MeshRenderer.hpp"
#include "components/Spin.hpp"
#include "components/SpriteRenderer.hpp"
#include "scene/Components.hpp"

namespace cinder::components {

void registerBuiltins(cinder::scene::Components& types) {
    types.add<MeshRenderer>("MeshRenderer");
    types.add<SpriteRenderer>("SpriteRenderer");
    types.add<Camera>("Camera");
    types.add<Spin>("Spin");
}

}
