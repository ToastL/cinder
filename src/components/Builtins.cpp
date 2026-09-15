#include "components/Builtins.hpp"

#include "components/Camera.hpp"
#include "components/Folder.hpp"
#include "components/Group.hpp"
#include "components/MeshPart.hpp"
#include "components/Spin.hpp"
#include "components/Sprite.hpp"
#include "scene/NodeTypes.hpp"

namespace cinder::components {

void registerBuiltins(cinder::scene::NodeTypes& types) {
    types.add<Group>("Group");
    types.add<Folder>("Folder");
    types.add<MeshPart>("MeshPart");
    types.add<Sprite>("Sprite");
    types.add<Camera>("Camera");
    types.add<Spin>("Spin");
}

}
