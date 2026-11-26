#include "Text/Rasterizer.hpp"

#include "Text/TrueType.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>

namespace cinder::Text {

namespace {

constexpr float FLAT_ENOUGH = 0.333f;
constexpr float TOLERANCE = 3.0f;

class Accumulator {
public:
    Accumulator(int width, int height)
        : width_(width), height_(height), cells_(static_cast<std::size_t>(width) * height + 4, 0.0f) {}

    void line(glm::vec2 from, glm::vec2 to) {
        if (from.y == to.y) return;
        float direction = 1.0f;
        if (from.y > to.y) {
            std::swap(from, to);
            direction = -1.0f;
        }
        const float slope = (to.x - from.x) / (to.y - from.y);
        float x = from.x;
        if (from.y < 0.0f) x -= from.y * slope;
        const int firstRow = std::max(0, static_cast<int>(from.y));
        const int lastRow = std::min(height_, static_cast<int>(std::ceil(to.y)));
        for (int row = firstRow; row < lastRow; ++row) {
            const std::size_t rowStart = static_cast<std::size_t>(row) * width_;
            const float dy = std::min(static_cast<float>(row + 1), to.y) - std::max(static_cast<float>(row), from.y);
            const float xNext = x + slope * dy;
            const float d = dy * direction;
            const float x0 = std::min(x, xNext);
            const float x1 = std::max(x, xNext);
            const float x0Floor = std::floor(x0);
            const int x0i = static_cast<int>(x0Floor);
            const float x1Ceil = std::ceil(x1);
            const int x1i = static_cast<int>(x1Ceil);
            if (x0i < 0) {
                x = xNext;
                continue;
            }
            if (x1i <= x0i + 1) {
                const float middle = 0.5f * (x + xNext) - x0Floor;
                add(rowStart + x0i, d - d * middle);
                add(rowStart + x0i + 1, d * middle);
            } else {
                const float s = 1.0f / (x1 - x0);
                const float x0f = x0 - x0Floor;
                const float a0 = 0.5f * s * (1.0f - x0f) * (1.0f - x0f);
                const float x1f = x1 - x1Ceil + 1.0f;
                const float am = 0.5f * s * x1f * x1f;
                add(rowStart + x0i, d * a0);
                if (x1i == x0i + 2) {
                    add(rowStart + x0i + 1, d * (1.0f - a0 - am));
                } else {
                    const float a1 = s * (1.5f - x0f);
                    add(rowStart + x0i + 1, d * (a1 - a0));
                    for (int column = x0i + 2; column < x1i - 1; ++column) add(rowStart + column, d * s);
                    const float a2 = a1 + static_cast<float>(x1i - x0i - 3) * s;
                    add(rowStart + x1i - 1, d * (1.0f - a2 - am));
                }
                add(rowStart + x1i, d * am);
            }
            x = xNext;
        }
    }

    void curve(glm::vec2 from, glm::vec2 control, glm::vec2 to) {
        const glm::vec2 deviation = from - 2.0f * control + to;
        const float deviationSquared = glm::dot(deviation, deviation);
        if (deviationSquared < FLAT_ENOUGH) {
            line(from, to);
            return;
        }
        const int steps = 1 + static_cast<int>(std::floor(std::sqrt(std::sqrt(TOLERANCE * deviationSquared))));
        glm::vec2 previous = from;
        const float step = 1.0f / static_cast<float>(steps);
        for (int i = 1; i < steps; ++i) {
            const float t = step * static_cast<float>(i);
            const glm::vec2 a = glm::mix(from, control, t);
            const glm::vec2 b = glm::mix(control, to, t);
            const glm::vec2 point = glm::mix(a, b, t);
            line(previous, point);
            previous = point;
        }
        line(previous, to);
    }

    std::vector<std::uint8_t> resolve() const {
        std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width_) * height_);
        float sum = 0.0f;
        for (std::size_t i = 0; i < pixels.size(); ++i) {
            sum += cells_[i];
            const float coverage = std::min(std::abs(sum), 1.0f);
            pixels[i] = static_cast<std::uint8_t>(coverage * 255.0f + 0.5f);
        }
        return pixels;
    }

private:
    void add(std::size_t index, float value) {
        if (index < cells_.size()) cells_[index] += value;
    }

    int width_;
    int height_;
    std::vector<float> cells_;
};

}

GlyphBitmap rasterizeOutline(const Outline& outline, float scale) {
    if (outline.empty()) return {};
    const int left = static_cast<int>(std::floor(outline.min.x * scale));
    const int right = static_cast<int>(std::ceil(outline.max.x * scale));
    const int top = static_cast<int>(std::ceil(outline.max.y * scale));
    const int bottom = static_cast<int>(std::floor(outline.min.y * scale));
    if (right <= left || top <= bottom) return {};

    GlyphBitmap result;
    result.width = right - left;
    result.height = top - bottom;
    result.bearing = glm::vec2(static_cast<float>(left), -static_cast<float>(top));

    const glm::vec2 origin(static_cast<float>(left), static_cast<float>(top));
    const auto place = [&](glm::vec2 point) {
        const glm::vec2 pixel(point.x * scale - origin.x, origin.y - point.y * scale);
        return glm::clamp(pixel, glm::vec2(0.0f),
                          glm::vec2(static_cast<float>(result.width), static_cast<float>(result.height)));
    };

    Accumulator accumulator(result.width, result.height);
    for (const OutlineSegment& segment : outline.segments) {
        if (segment.curved) accumulator.curve(place(segment.from), place(segment.control), place(segment.to));
        else accumulator.line(place(segment.from), place(segment.to));
    }
    result.coverage = accumulator.resolve();
    return result;
}

}
