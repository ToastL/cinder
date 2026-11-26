#pragma once

#include "Text/Font.hpp"

namespace cinder::Text {

struct Outline;

GlyphBitmap rasterizeOutline(const Outline& outline, float scale);

}
