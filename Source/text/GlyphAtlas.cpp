#include "text/GlyphAtlas.hpp"

#include "text/Font.hpp"

#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>

namespace cinder::text {

namespace {

constexpr float SIZE_STEPS = 4.0f;

}

struct GlyphAtlas::Page {
    std::vector<std::uint8_t> pixels;
    std::vector<stbrp_node> nodes;
    stbrp_context packer{};
    std::optional<AtlasRect> dirty;

    Page() : pixels(static_cast<std::size_t>(PAGE_SIZE) * PAGE_SIZE, 0), nodes(PAGE_SIZE) { reset(); }

    void reset() {
        std::fill(pixels.begin(), pixels.end(), std::uint8_t{0});
        stbrp_init_target(&packer, PAGE_SIZE, PAGE_SIZE, nodes.data(), static_cast<int>(nodes.size()));
        dirty = AtlasRect{0, 0, PAGE_SIZE, PAGE_SIZE};
    }
};

std::size_t GlyphAtlas::KeyHash::operator()(const Key& key) const {
    std::size_t hash = std::hash<const Font*>{}(key.font);
    hash ^= std::hash<std::uint32_t>{}(key.glyph) + 0x9e3779b9u + (hash << 6) + (hash >> 2);
    hash ^= std::hash<std::uint32_t>{}(key.size) + 0x9e3779b9u + (hash << 6) + (hash >> 2);
    return hash;
}

GlyphAtlas::GlyphAtlas() = default;
GlyphAtlas::~GlyphAtlas() = default;

const AtlasGlyph& GlyphAtlas::glyph(const Font& font, std::uint32_t glyph, float pixelSize) {
    const auto steps = static_cast<std::uint32_t>(std::max(0L, std::lround(pixelSize * SIZE_STEPS)));
    const Key key{&font, glyph, steps};
    if (auto found = glyphs_.find(key); found != glyphs_.end()) return found->second;

    AtlasGlyph placed;
    const GlyphBitmap bitmap = font.rasterize(glyph, static_cast<float>(steps) / SIZE_STEPS);
    placed.bearing = bitmap.bearing;
    if (bitmap.width > 0 && bitmap.height > 0) {
        int page = 0;
        if (std::optional<AtlasRect> rect = place(bitmap.width, bitmap.height, page)) {
            Page& target = *pages_[static_cast<std::size_t>(page)];
            for (int row = 0; row < bitmap.height; ++row) {
                std::memcpy(target.pixels.data() + static_cast<std::size_t>(rect->y + row) * PAGE_SIZE + rect->x,
                            bitmap.coverage.data() + static_cast<std::size_t>(row) * bitmap.width,
                            static_cast<std::size_t>(bitmap.width));
            }
            placed.page = page;
            placed.rect = *rect;
            markDirty(page, *rect);
        }
    }
    return glyphs_.emplace(key, placed).first->second;
}

std::optional<AtlasRect> GlyphAtlas::place(int width, int height, int& page) {
    const int paddedWidth = width + PADDING * 2;
    const int paddedHeight = height + PADDING * 2;
    if (paddedWidth > PAGE_SIZE || paddedHeight > PAGE_SIZE) return std::nullopt;

    for (std::size_t index = 0; index <= pages_.size(); ++index) {
        if (index == pages_.size()) pages_.push_back(std::make_unique<Page>());
        stbrp_rect rect{};
        rect.w = paddedWidth;
        rect.h = paddedHeight;
        if (stbrp_pack_rects(&pages_[index]->packer, &rect, 1) != 0 && rect.was_packed != 0) {
            page = static_cast<int>(index);
            return AtlasRect{rect.x + PADDING, rect.y + PADDING, width, height};
        }
    }
    return std::nullopt;
}

void GlyphAtlas::markDirty(int page, const AtlasRect& rect) {
    std::optional<AtlasRect>& dirty = pages_[static_cast<std::size_t>(page)]->dirty;
    if (!dirty) {
        dirty = rect;
        return;
    }
    const int left = std::min(dirty->x, rect.x);
    const int top = std::min(dirty->y, rect.y);
    const int right = std::max(dirty->x + dirty->width, rect.x + rect.width);
    const int bottom = std::max(dirty->y + dirty->height, rect.y + rect.height);
    dirty = AtlasRect{left, top, right - left, bottom - top};
}

const std::vector<std::uint8_t>& GlyphAtlas::pixels(int page) const {
    return pages_.at(static_cast<std::size_t>(page))->pixels;
}

std::optional<AtlasRect> GlyphAtlas::dirty(int page) const {
    return pages_.at(static_cast<std::size_t>(page))->dirty;
}

void GlyphAtlas::clearDirty(int page) {
    pages_.at(static_cast<std::size_t>(page))->dirty.reset();
}

void GlyphAtlas::clear() {
    glyphs_.clear();
    for (auto& page : pages_) page->reset();
}

}
