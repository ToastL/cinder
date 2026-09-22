#pragma once

#include <cstdint>

namespace cinder::platform {

enum class CursorShape : std::uint8_t {
    Arrow,
    IBeam,
    Hand,
    Crosshair,
    ResizeHorizontal,
    ResizeVertical,
    ResizeAll,
    NotAllowed,
    Count,
};

}
