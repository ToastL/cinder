#include "nodes/Builtins.hpp"

#include "nodes/Camera.hpp"
#include "nodes/Folder.hpp"
#include "nodes/Group.hpp"
#include "nodes/MeshPart.hpp"
#include "nodes/Sprite.hpp"
#include "scene/NodeTypes.hpp"

namespace cinder::nodes {

void registerBuiltins(cinder::scene::NodeTypes& types) {
    types.add<Group>("Group");
    types.add<Folder>("Folder");
    types.add<MeshPart>("MeshPart");
    types.add<Sprite>("Sprite");
    types.add<Camera>("Camera");
}

}
