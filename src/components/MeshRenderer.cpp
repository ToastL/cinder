#include "components/MeshRenderer.hpp"

#include "scene/Actor.hpp"
#include "scene/DrawList.hpp"

namespace cinder::components {

void MeshRenderer::onRender(float alpha, cinder::scene::DrawList& draws) {
    draws.mesh(mesh_, texture_, transform().world());
}

}
