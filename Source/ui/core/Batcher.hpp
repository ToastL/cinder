#pragma once

#include "ui/core/Rect.hpp"
#include "ui/core/TextureRef.hpp"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <vector>

namespace cinder::ui {

class ElementList;

enum class UiMode : int { Solid = 0, Textured = 1, Glyph = 2, Box = 3, Opaque = 4 };

struct UiVertex {
    glm::vec2 position{0.0f};
    glm::vec2 uv{0.0f};
    glm::vec4 color{0.0f};
    glm::vec4 border{0.0f};
    glm::vec4 shape{0.0f};
    glm::vec4 radii{0.0f};
};

struct UiBatch {
    TextureRef texture;
    Rect clip;
    std::uint32_t firstIndex = 0;
    std::uint32_t indexCount = 0;
};

struct UiGeometry {
    std::vector<UiVertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<UiBatch> batches;

    void clear();
    bool empty() const { return indices.empty(); }
};

void batch(const ElementList& list, UiGeometry& out);

}
