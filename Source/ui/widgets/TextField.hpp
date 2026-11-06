#pragma once

#include "text/TextLayout.hpp"
#include "ui/core/CompoundWidget.hpp"
#include "ui/core/Args.hpp"
#include "ui/core/Delegates.hpp"
#include "ui/core/TextRun.hpp"

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace cinder::ui {

class TextField : public LeafWidget {
public:
    using KeyHandler = std::function<bool(const KeyEvent&)>;

    struct Args : ::cinder::ui::Args<Args, TextField> {
        UI_ATTR(std::string, text)
        UI_ATTR(std::string, hintText)
        UI_ARG(std::string, style, "TextField")
        UI_ARG(std::optional<FontInfo>, font)
        UI_EVENT(OnTextChanged, onTextChanged)
        UI_EVENT(OnTextCommitted, onTextCommitted)
        UI_EVENT(KeyHandler, onKeyDownHandler)
        UI_ARG(bool, clearFocusOnCommit, true)
        UI_ARG(bool, revertOnEscape, true)
        UI_ARG(bool, selectAllOnFocus)
        UI_ATTR(bool, isReadOnly)
    };

    void construct(const Args& args);

    std::string text() const { return hasFocus() ? buffer_ : text_.get(); }
    void setText(const std::string& text);
    void selectAll();
    std::size_t caret() const { return caret_; }
    std::size_t anchor() const { return anchor_; }
    bool hasSelection() const { return caret_ != anchor_; }
    std::string selectedText() const;

    bool supportsKeyboardFocus() const override { return true; }
    bool isEditingText() const override { return hasFocus() && !readOnly_.get(); }

    Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseDoubleClick(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseMove(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseUp(const Geometry& geometry, const PointerEvent& event) override;
    Reply onKeyDown(const Geometry& geometry, const KeyEvent& event) override;
    Reply onKeyChar(const Geometry& geometry, const CharEvent& event) override;
    Reply onFocusReceived(const Geometry& geometry, const FocusEvent& event) override;
    void onFocusLost(const FocusEvent& event) override;
    void onMouseCaptureLost() override { selecting_ = false; }
    std::optional<cinder::platform::CursorShape> onCursorQuery(const Geometry& geometry,
                                                               const PointerEvent& event) const override;

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    struct State {
        std::string text;
        std::size_t caret = 0;
        std::size_t anchor = 0;
    };

    enum class Edit { None, Typing, Deleting, Other };

    FontInfo font() const;
    const cinder::text::CaretStops& stops() const;
    std::size_t offsetAt(const Geometry& geometry, glm::vec2 point) const;
    void move(std::size_t to, bool extend);
    void replaceSelection(std::string_view inserted, Edit kind);
    void erase(std::size_t from, std::size_t to);
    void remember(Edit kind);
    void undo();
    void redo();
    void changed();
    void commit(TextCommit how);

    Attribute<std::string> text_;
    Attribute<std::string> hint_;
    Attribute<bool> readOnly_{false};
    std::string style_;
    std::optional<FontInfo> font_;
    OnTextChanged onChanged_;
    OnTextCommitted onCommitted_;
    KeyHandler onKey_;
    bool clearOnCommit_ = true;
    bool revertOnEscape_ = true;
    bool selectAllOnFocus_ = false;

    std::string buffer_;
    std::string original_;
    std::size_t caret_ = 0;
    std::size_t anchor_ = 0;
    std::vector<State> undo_;
    std::vector<State> redo_;
    Edit lastEdit_ = Edit::None;
    bool selecting_ = false;
    bool suppressCommit_ = false;
    double blink_ = 0.0;
    mutable float scroll_ = 0.0f;
    mutable TextRun run_;
    mutable std::string stopsText_;
    mutable FontInfo stopsFont_;
    mutable cinder::text::CaretStops stops_;
};

class TextBox : public CompoundWidget {
public:
    struct Args : ::cinder::ui::Args<Args, TextBox> {
        UI_ATTR(std::string, text)
        UI_ATTR(std::string, hintText)
        UI_ARG(std::string, style, "TextBox")
        UI_ARG(std::optional<FontInfo>, font)
        UI_EVENT(OnTextChanged, onTextChanged)
        UI_EVENT(OnTextCommitted, onTextCommitted)
        UI_EVENT(TextField::KeyHandler, onKeyDownHandler)
        UI_ARG(bool, clearFocusOnCommit, true)
        UI_ARG(bool, revertOnEscape, true)
        UI_ARG(bool, selectAllOnFocus)
        UI_ATTR(bool, isReadOnly)
        UI_ATTR(float, minDesiredWidth)
    };

    void construct(const Args& args);

    const std::shared_ptr<TextField>& field() const { return editable_; }
    std::string text() const { return editable_->text(); }
    void setText(const std::string& text) { editable_->setText(text); }

    Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) override;

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    std::shared_ptr<TextField> editable_;
    std::string style_;
    Attribute<float> minWidth_;
};

}
