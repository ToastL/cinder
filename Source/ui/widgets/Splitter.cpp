#include "ui/widgets/Splitter.hpp"

#include "ui/core/ElementList.hpp"
#include "ui/framework/Application.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace cinder::ui {

namespace {

bool present(const std::shared_ptr<Widget>& widget) { return widget && takesSpace(widget->visibility()); }

float along(Orientation orientation, glm::vec2 value) {
    return orientation == Orientation::Horizontal ? value.x : value.y;
}

float across(Orientation orientation, glm::vec2 value) {
    return orientation == Orientation::Horizontal ? value.y : value.x;
}

glm::vec2 compose(Orientation orientation, float main, float cross) {
    return orientation == Orientation::Horizontal ? glm::vec2(main, cross) : glm::vec2(cross, main);
}

}

void Splitter::construct(const Args& args) {
    orientation_ = args.orientation_;
    style_ = args.style_;
    slots_ = args.slots_;
    onFinished_ = args.onFinishedResizing_;
}

SplitterSlot& Splitter::addSlot(SplitterSlot slot) {
    slots_.push_back(std::move(slot));
    return slots_.back();
}

float Splitter::handleSize() const { return Application::get().theme().get<SplitterStyle>(style_).handleSize; }

glm::vec2 Splitter::computeDesiredSize(float) const {
    float main = 0.0f;
    float cross = 0.0f;
    for (const SplitterSlot& slot : slots_) {
        if (!slot.widget) continue;
        main += along(orientation_, slot.widget->desiredSize());
        cross = std::max(cross, across(orientation_, slot.widget->desiredSize()));
    }
    if (!slots_.empty()) main += handleSize() * static_cast<float>(slots_.size() - 1);
    return compose(orientation_, main, cross);
}

std::vector<Span> Splitter::spans(const Geometry& geometry) const {
    std::vector<Span> result;
    if (slots_.empty()) return result;
    const float handle = handleSize();
    const float total = std::max(0.0f, along(orientation_, geometry.size) - handle * static_cast<float>(slots_.size() - 1));
    float sum = 0.0f;
    for (const SplitterSlot& slot : slots_) sum += std::max(slot.value_.get(), 0.0f);
    float offset = 0.0f;
    for (const SplitterSlot& slot : slots_) {
        const float size = sum > 0.0f ? std::round(total * std::max(slot.value_.get(), 0.0f) / sum) : 0.0f;
        result.push_back({offset, size});
        offset += size + handle;
    }
    if (!result.empty()) result.back().size = std::max(0.0f, along(orientation_, geometry.size) - result.back().offset);
    return result;
}

void Splitter::arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const {
    const std::vector<Span> sizes = spans(geometry);
    const float cross = across(orientation_, geometry.size);
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        if (!present(slots_[i].widget)) continue;
        out.push_back({slots_[i].widget, geometry.child(compose(orientation_, sizes[i].offset, 0.0f),
                                                        compose(orientation_, sizes[i].size, cross))});
    }
}

int Splitter::handleAt(const Geometry& geometry, glm::vec2 point) const {
    const std::vector<Span> sizes = spans(geometry);
    const glm::vec2 local = geometry.local(point);
    const float position = along(orientation_, local);
    const float cross = across(orientation_, local);
    if (cross < 0.0f || cross > across(orientation_, geometry.size)) return -1;
    const float slack = 3.0f;
    for (std::size_t i = 0; i + 1 < sizes.size(); ++i) {
        const float start = sizes[i].offset + sizes[i].size;
        if (position >= start - slack && position <= start + handleSize() + slack) return static_cast<int>(i);
    }
    return -1;
}

void Splitter::setValue(std::size_t index, float value) {
    SplitterSlot& slot = slots_[index];
    if (slot.onSlotResized_) slot.onSlotResized_(value);
    if (!slot.value_.bound()) slot.value_.set(value);
}

Reply Splitter::onMouseDown(const Geometry& geometry, const PointerEvent& event) {
    if (event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    dragged_ = handleAt(geometry, event.position);
    if (dragged_ < 0) return Reply::unhandled();
    return Reply::handled().captureMouse(shared_from_this());
}

Reply Splitter::onMouseMove(const Geometry& geometry, const PointerEvent& event) {
    if (dragged_ < 0) {
        hovered_ = handleAt(geometry, event.position);
        return Reply::unhandled();
    }
    const std::vector<Span> sizes = spans(geometry);
    const auto first = static_cast<std::size_t>(dragged_);
    const float combined = sizes[first].size + sizes[first + 1].size;
    if (combined <= MIN_SLOT * 2.0f) return Reply::handled();
    const float position = along(orientation_, geometry.local(event.position));
    const float size = std::clamp(position - sizes[first].offset - handleSize() * 0.5f, MIN_SLOT, combined - MIN_SLOT);
    const float values = slots_[first].value_.get() + slots_[first + 1].value_.get();
    const float value = values * size / combined;
    setValue(first, value);
    setValue(first + 1, values - value);
    return Reply::handled();
}

Reply Splitter::onMouseUp(const Geometry&, const PointerEvent& event) {
    if (dragged_ < 0 || event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    dragged_ = -1;
    if (onFinished_) onFinished_();
    return Reply::handled().releaseMouseCapture();
}

std::optional<cinder::platform::CursorShape> Splitter::onCursorQuery(const Geometry& geometry,
                                                                     const PointerEvent& event) const {
    if (dragged_ >= 0 || handleAt(geometry, event.position) >= 0) {
        return orientation_ == Orientation::Horizontal ? cinder::platform::CursorShape::ResizeHorizontal
                                                       : cinder::platform::CursorShape::ResizeVertical;
    }
    return std::nullopt;
}

int Splitter::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                       const PaintStyle& style, bool enabled) const {
    const SplitterStyle& look = Application::get().theme().get<SplitterStyle>(style_);
    const std::vector<Span> sizes = spans(geometry);
    const float cross = across(orientation_, geometry.size);
    for (std::size_t i = 0; i + 1 < sizes.size(); ++i) {
        const float start = sizes[i].offset + sizes[i].size;
        const Rect rect = geometry.localRect(compose(orientation_, start, 0.0f), compose(orientation_, look.handleSize, cross));
        const bool hot = dragged_ == static_cast<int>(i) || (dragged_ < 0 && hovered_ == static_cast<int>(i) && isHovered());
        (hot ? look.handleHovered : look.handle).paint(list, layer, rect, style, geometry.scale);
    }
    return paintChildren(args, geometry, list, layer, style, enabled);
}

}
