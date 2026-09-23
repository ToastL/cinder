#include "ui/widgets/TextField.hpp"

#include "text/Utf8.hpp"
#include "ui/core/ElementList.hpp"
#include "ui/framework/Application.hpp"

#include <algorithm>
#include <cmath>

namespace cinder::ui {

namespace keys = cinder::platform::keys;

namespace {

constexpr float CARET_WIDTH = 1.0f;
constexpr double BLINK_PERIOD = 1.0;

bool wordCharacter(char32_t codepoint) {
    return codepoint >= 0x80 || (codepoint >= U'a' && codepoint <= U'z') || (codepoint >= U'A' && codepoint <= U'Z')
            || (codepoint >= U'0' && codepoint <= U'9') || codepoint == U'_';
}

bool wordKey(const KeyEvent& event) {
#if defined(__APPLE__)
    return event.alt();
#else
    return event.control();
#endif
}

bool lineKey(const KeyEvent& event) {
#if defined(__APPLE__)
    return event.super();
#else
    return false;
#endif
}

std::string singleLine(std::string text) {
    for (char& c : text) {
        if (c == '\n' || c == '\r' || c == '\t') c = ' ';
    }
    return text;
}

}

void TextField::construct(const Args& args) {
    text_ = args.text_;
    hint_ = args.hintText_;
    readOnly_ = args.isReadOnly_;
    style_ = args.style_;
    font_ = args.font_;
    onChanged_ = args.onTextChanged_;
    onCommitted_ = args.onTextCommitted_;
    onKey_ = args.onKeyDownHandler_;
    clearOnCommit_ = args.clearFocusOnCommit_;
    revertOnEscape_ = args.revertOnEscape_;
    selectAllOnFocus_ = args.selectAllOnFocus_;
}

FontInfo TextField::font() const {
    if (font_) return *font_;
    return Application::get().theme().get<TextFieldStyle>(style_).font;
}

const cinder::text::CaretStops& TextField::stops() const {
    const std::string shown = text();
    const FontInfo info = font();
    if (shown != stopsText_ || !(info == stopsFont_) || stops_.offsets.empty()) {
        stopsText_ = shown;
        stopsFont_ = info;
        const cinder::text::ShapedText shaped =
                cinder::text::shape(Application::get().fonts().get(info.face), shown, info.size);
        stops_ = cinder::text::caretStops(shaped, shown);
    }
    return stops_;
}

std::size_t TextField::offsetAt(const Geometry& geometry, glm::vec2 point) const {
    return stops().nearest(geometry.local(point).x + scroll_);
}

void TextField::setText(const std::string& text) {
    if (!text_.bound()) text_.set(text);
    if (hasFocus()) {
        buffer_ = text;
        original_ = text;
        caret_ = anchor_ = buffer_.size();
    }
}

void TextField::selectAll() {
    anchor_ = 0;
    caret_ = text().size();
}

std::string TextField::selectedText() const {
    const std::size_t low = std::min(caret_, anchor_);
    const std::size_t high = std::max(caret_, anchor_);
    const std::string shown = text();
    if (low >= shown.size()) return {};
    return shown.substr(low, high - low);
}

void TextField::remember(Edit kind) {
    if (!(kind == Edit::Typing && lastEdit_ == Edit::Typing) && !(kind == Edit::Deleting && lastEdit_ == Edit::Deleting)) {
        undo_.push_back(State{buffer_, caret_, anchor_});
    }
    redo_.clear();
    lastEdit_ = kind;
}

void TextField::changed() {
    blink_ = Application::get().time();
    if (onChanged_) onChanged_(buffer_);
}

void TextField::move(std::size_t to, bool extend) {
    caret_ = std::min(to, buffer_.size());
    if (!extend) anchor_ = caret_;
    lastEdit_ = Edit::None;
    blink_ = Application::get().time();
}

void TextField::replaceSelection(std::string_view inserted, Edit kind) {
    remember(kind);
    const std::size_t low = std::min(caret_, anchor_);
    const std::size_t high = std::max(caret_, anchor_);
    buffer_.replace(low, high - low, inserted);
    caret_ = anchor_ = low + inserted.size();
    changed();
}

void TextField::erase(std::size_t from, std::size_t to) {
    const std::size_t low = std::min(from, to);
    const std::size_t high = std::max(from, to);
    if (low == high) return;
    remember(Edit::Deleting);
    buffer_.erase(low, high - low);
    caret_ = anchor_ = low;
    changed();
}

void TextField::undo() {
    if (undo_.empty()) return;
    redo_.push_back(State{buffer_, caret_, anchor_});
    const State state = undo_.back();
    undo_.pop_back();
    buffer_ = state.text;
    caret_ = state.caret;
    anchor_ = state.anchor;
    lastEdit_ = Edit::Other;
    changed();
}

void TextField::redo() {
    if (redo_.empty()) return;
    undo_.push_back(State{buffer_, caret_, anchor_});
    const State state = redo_.back();
    redo_.pop_back();
    buffer_ = state.text;
    caret_ = state.caret;
    anchor_ = state.anchor;
    lastEdit_ = Edit::Other;
    changed();
}

void TextField::commit(TextCommit how) {
    if (!text_.bound()) text_.set(buffer_);
    original_ = buffer_;
    if (onCommitted_) onCommitted_(buffer_, how);
}

Reply TextField::onFocusReceived(const Geometry&, const FocusEvent& event) {
    if (event.cause != FocusCause::Mouse || buffer_ != text_.get() || original_ != buffer_) {
        buffer_ = text_.get();
        original_ = buffer_;
        caret_ = anchor_ = buffer_.size();
    }
    if (selectAllOnFocus_ && event.cause != FocusCause::Mouse) selectAll();
    undo_.clear();
    redo_.clear();
    lastEdit_ = Edit::None;
    blink_ = Application::get().time();
    return Reply::unhandled();
}

void TextField::onFocusLost(const FocusEvent&) {
    if (!suppressCommit_ && buffer_ != original_) commit(TextCommit::FocusLost);
    suppressCommit_ = false;
    selecting_ = false;
    anchor_ = caret_;
}

Reply TextField::onMouseDown(const Geometry& geometry, const PointerEvent& event) {
    if (event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    Reply reply = Reply::handled().captureMouse(shared_from_this());
    if (!hasFocus()) {
        buffer_ = text_.get();
        original_ = buffer_;
        reply.setFocus(shared_from_this());
    }
    const std::size_t offset = offsetAt(geometry, event.position);
    const bool extend = event.shift() && hasFocus();
    caret_ = std::min(offset, buffer_.size());
    if (!extend) anchor_ = caret_;
    if (selectAllOnFocus_ && !hasFocus()) {
        anchor_ = 0;
        caret_ = buffer_.size();
    }
    selecting_ = true;
    lastEdit_ = Edit::None;
    return reply;
}

Reply TextField::onMouseDoubleClick(const Geometry& geometry, const PointerEvent& event) {
    if (event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    std::size_t start = std::min(offsetAt(geometry, event.position), buffer_.size());
    std::size_t end = start;
    while (start > 0 && wordCharacter(cinder::text::decodeAt(buffer_, cinder::text::previousBoundary(buffer_, start)))) {
        start = cinder::text::previousBoundary(buffer_, start);
    }
    while (end < buffer_.size() && wordCharacter(cinder::text::decodeAt(buffer_, end))) {
        end = cinder::text::nextBoundary(buffer_, end);
    }
    anchor_ = start;
    caret_ = end;
    return Reply::handled();
}

Reply TextField::onMouseMove(const Geometry& geometry, const PointerEvent& event) {
    if (!selecting_) return Reply::unhandled();
    caret_ = std::min(offsetAt(geometry, event.position), buffer_.size());
    return Reply::handled();
}

Reply TextField::onMouseUp(const Geometry&, const PointerEvent& event) {
    if (!selecting_ || event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    selecting_ = false;
    return Reply::handled().releaseMouseCapture();
}

Reply TextField::onKeyDown(const Geometry&, const KeyEvent& event) {
    if (!hasFocus()) return Reply::unhandled();
    if (onKey_ && onKey_(event)) return Reply::handled();

    namespace utf8 = cinder::text;
    const bool readOnly = readOnly_.get();
    const bool word = wordKey(event);
    const bool line = lineKey(event);
    const std::size_t low = std::min(caret_, anchor_);
    const std::size_t high = std::max(caret_, anchor_);

    switch (event.key) {
        case keys::LEFT:
            if (hasSelection() && !event.shift() && !word && !line) move(low, false);
            else move(line ? 0 : (word ? utf8::previousWord(buffer_, caret_) : utf8::previousBoundary(buffer_, caret_)), event.shift());
            return Reply::handled();
        case keys::RIGHT:
            if (hasSelection() && !event.shift() && !word && !line) move(high, false);
            else move(line ? buffer_.size() : (word ? utf8::nextWord(buffer_, caret_) : utf8::nextBoundary(buffer_, caret_)), event.shift());
            return Reply::handled();
        case keys::HOME:
            move(0, event.shift());
            return Reply::handled();
        case keys::END:
            move(buffer_.size(), event.shift());
            return Reply::handled();
        case keys::BACKSPACE:
            if (readOnly) return Reply::handled();
            if (hasSelection()) replaceSelection("", Edit::Deleting);
            else erase(line ? 0 : (word ? utf8::previousWord(buffer_, caret_) : utf8::previousBoundary(buffer_, caret_)), caret_);
            return Reply::handled();
        case keys::DELETE:
            if (readOnly) return Reply::handled();
            if (hasSelection()) replaceSelection("", Edit::Deleting);
            else erase(caret_, word ? utf8::nextWord(buffer_, caret_) : utf8::nextBoundary(buffer_, caret_));
            return Reply::handled();
        case keys::ENTER:
        case keys::KEYPAD_ENTER:
            commit(TextCommit::Enter);
            if (clearOnCommit_) {
                suppressCommit_ = true;
                return Reply::handled().clearFocus();
            }
            undo_.clear();
            redo_.clear();
            return Reply::handled();
        case keys::ESCAPE:
            if (!revertOnEscape_) return Reply::unhandled();
            buffer_ = original_;
            caret_ = anchor_ = buffer_.size();
            changed();
            if (onCommitted_) onCommitted_(original_, TextCommit::Cleared);
            suppressCommit_ = true;
            return Reply::handled().clearFocus();
        default:
            break;
    }

    if (event.primary()) {
        if (event.key == keys::letter('a')) {
            anchor_ = 0;
            caret_ = buffer_.size();
            return Reply::handled();
        }
        if (event.key == keys::letter('c')) {
            if (hasSelection()) Application::get().platform().setClipboard(selectedText());
            return Reply::handled();
        }
        if (event.key == keys::letter('x')) {
            if (hasSelection()) {
                Application::get().platform().setClipboard(selectedText());
                if (!readOnly) replaceSelection("", Edit::Other);
            }
            return Reply::handled();
        }
        if (event.key == keys::letter('v')) {
            if (!readOnly) replaceSelection(singleLine(Application::get().platform().clipboard()), Edit::Other);
            return Reply::handled();
        }
        if (event.key == keys::letter('z')) {
            if (!readOnly) event.shift() ? redo() : undo();
            return Reply::handled();
        }
        if (event.key == keys::letter('y')) {
            if (!readOnly) redo();
            return Reply::handled();
        }
        return Reply::unhandled();
    }

    const bool printable = (event.key >= keys::SPACE && event.key <= 96) || event.key == keys::INSERT;
    return printable && !event.control() ? Reply::handled() : Reply::unhandled();
}

Reply TextField::onKeyChar(const Geometry&, const CharEvent& event) {
    if (!hasFocus() || readOnly_.get()) return Reply::unhandled();
    if (event.character < 0x20 || event.character == 0x7F) return Reply::unhandled();
    if (event.control() || event.super()) return Reply::unhandled();
    std::string typed;
    cinder::text::appendUtf8(typed, event.character);
    replaceSelection(typed, Edit::Typing);
    return Reply::handled();
}

std::optional<cinder::platform::CursorShape> TextField::onCursorQuery(const Geometry&, const PointerEvent&) const {
    return cinder::platform::CursorShape::IBeam;
}

glm::vec2 TextField::computeDesiredSize(float) const {
    Application& app = Application::get();
    const std::string shown = text();
    run_.shape(app.fonts(), shown.empty() ? hint_.get() : shown, font());
    return {run_.size().x + CARET_WIDTH * 2.0f, run_.lineHeight()};
}

int TextField::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                           const PaintStyle& style, bool enabled) const {
    Application& app = Application::get();
    const TextFieldStyle& look = app.theme().get<TextFieldStyle>(style_);
    const FontInfo info = font();
    const std::string shown = text();
    const bool hinting = shown.empty();
    run_.shape(app.fonts(), hinting ? hint_.get() : shown, info);
    const float height = run_.lineHeight();
    const float top = std::round((geometry.size.y - height) * 0.5f);
    const bool focused = hasFocus();

    if (focused && !hinting) {
        const float caretX = stops().positionOf(caret_);
        const float width = geometry.size.x - CARET_WIDTH;
        if (caretX - scroll_ > width) scroll_ = caretX - width;
        if (caretX - scroll_ < 0.0f) scroll_ = caretX;
        scroll_ = std::clamp(scroll_, 0.0f, std::max(0.0f, run_.size().x - width));
    } else if (!focused) {
        scroll_ = 0.0f;
    }

    list.pushClip(geometry.rect());
    if (focused && hasSelection() && !hinting) {
        const float from = stops().positionOf(std::min(caret_, anchor_)) - scroll_;
        const float to = stops().positionOf(std::max(caret_, anchor_)) - scroll_;
        list.box(layer, geometry.localRect({from, top}, {to - from, height}), BoxStyle{style.apply(look.selection)});
    }

    Color color = hinting ? look.hint : look.color;
    color = style.apply(color);
    if (!enabled) color.a *= 0.4f;
    run_.paint(list, layer + 1, geometry.absolute({-scroll_, top}), geometry.scale, color, app.atlas());

    const bool blinkOn = std::fmod(args.time - blink_, BLINK_PERIOD) < BLINK_PERIOD * 0.5;
    if (focused && !readOnly_.get() && blinkOn) {
        const float x = std::round((hinting ? 0.0f : stops().positionOf(caret_)) - scroll_);
        list.box(layer + 2, geometry.localRect({x, top}, {CARET_WIDTH, height}), BoxStyle{style.apply(look.caret)});
    }
    list.popClip();
    return layer + 2;
}

void TextBox::construct(const Args& args) {
    style_ = args.style_;
    minWidth_ = args.minDesiredWidth_;
    editable_ = make<TextField>()
                        .text(args.text_)
                        .hintText(args.hintText_)
                        .font(args.font_)
                        .onTextChanged(args.onTextChanged_)
                        .onTextCommitted(args.onTextCommitted_)
                        .onKeyDownHandler(args.onKeyDownHandler_)
                        .clearFocusOnCommit(args.clearFocusOnCommit_)
                        .revertOnEscape(args.revertOnEscape_)
                        .selectAllOnFocus(args.selectAllOnFocus_)
                        .isReadOnly(args.isReadOnly_);
    childSlot_.widget = editable_;
}

glm::vec2 TextBox::computeDesiredSize(float) const {
    const TextBoxStyle& look = Application::get().theme().get<TextBoxStyle>(style_);
    glm::vec2 size = editable_->desiredSize() + look.padding.total();
    size.x = std::max(size.x, minWidth_.get());
    return size;
}

Reply TextBox::onMouseDown(const Geometry&, const PointerEvent& event) {
    if (event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    return Reply::handled().setFocus(editable_);
}

int TextBox::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                              const PaintStyle& style, bool enabled) const {
    const TextBoxStyle& look = Application::get().theme().get<TextBoxStyle>(style_);
    const Brush& brush = !enabled ? look.disabled
            : (editable_->hasFocus() ? look.focused : (isHovered() ? look.hovered : look.normal));
    brush.paint(list, layer, geometry.rect(), style, geometry.scale);

    const glm::vec2 inset = look.padding.topLeft();
    const Geometry inner = geometry.child(inset, glm::max(glm::vec2(0.0f), geometry.size - look.padding.total()));
    return editable_->paint(args, inner, list, layer + 1, style, enabled);
}

}
