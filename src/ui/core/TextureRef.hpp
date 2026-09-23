#pragma once

#include <cstdint>

namespace cinder::ui {

struct TextureRef {
    enum class Kind : std::uint8_t { None, GlyphPage, Viewport, Named };

    Kind kind = Kind::None;
    std::uint32_t index = 0;

    static TextureRef none() { return {}; }
    static TextureRef glyphPage(int page) { return {Kind::GlyphPage, static_cast<std::uint32_t>(page)}; }
    static TextureRef viewport() { return {Kind::Viewport, 0}; }

    bool operator==(const TextureRef&) const = default;
};

}
