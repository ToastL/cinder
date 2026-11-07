#include "Nodes/Builtins.hpp"

#include "Nodes/Camera.hpp"
#include "Nodes/Folder.hpp"
#include "Nodes/Group.hpp"
#include "Nodes/MeshPart.hpp"
#include "Nodes/Sprite.hpp"
#include "Scene/NodeTypes.hpp"

namespace cinder::Nodes {

void registerBuiltins(cinder::Scene::NodeTypes& types) {
    types.add<Group>("Group");
    types.add<Folder>("Folder");
    types.add<MeshPart>("MeshPart");
    types.add<Sprite>("Sprite");
    types.add<Camera>("Camera");
}

}
