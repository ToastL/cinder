#include "Gfx/RHI/Image.hpp"

#include "Gfx/RHI/Png.hpp"

namespace cinder::Gfx::RHI {

Image loadImage(const std::string& path) { return readPng(path); }

}
