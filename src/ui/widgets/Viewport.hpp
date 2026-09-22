#pragma once

#include "ui/core/Args.hpp"
#include "ui/core/CompoundWidget.hpp"
#include "ui/core/TextureRef.hpp"

#include <memory>
#include <optional>

namespace cinder::ui {

class Viewport;

class ViewportClient {
public:
    virtual ~ViewportClient() = default;

    virtual TextureRef texture() const { return TextureRef::viewport(); }
    virtual void onArrange(const Geometry& geometry) {}
    virtual int onPaint(const Geometry& geometry, ElementList& list, int layer) { return layer; }
    virtual void tick(const Geometry& geometry, double time, float deltaTime) {}

    virtual Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) { return Reply::unhandled(); }
    virtual Reply onMouseUp(const Geometry& geometry, const PointerEvent& event) { return Reply::unhandled(); }
    virtual Reply onMouseMove(const Geometry& geometry, const PointerEvent& event) { return Reply::unhandled(); }
    virtual Reply onMouseDoubleClick(const Geometry& geometry, const PointerEvent& event) {
        return onMouseDown(geometry, event);
    }
    virtual Reply onMouseWheel(const Geometry& geometry, const PointerEvent& event) { return Reply::unhandled(); }
    virtual void onMouseEnter(const Geometry& geometry, const PointerEvent& event) {}
    virtual void onMouseLeave(const PointerEvent& event) {}
    virtual void onMouseCaptureLost() {}
    virtual Reply onKeyDown(const Geometry& geometry, const KeyEvent& event) { return Reply::unhandled(); }
    virtual Reply onKeyUp(const Geometry& geometry, const KeyEvent& event) { return Reply::unhandled(); }
    virtual Reply onKeyChar(const Geometry& geometry, const CharEvent& event) { return Reply::unhandled(); }
    virtual void onFocusReceived(const FocusEvent& event) {}
    virtual void onFocusLost(const FocusEvent& event) {}
    virtual std::optional<cinder::platform::CursorShape> cursor(const Geometry& geometry,
                                                                const PointerEvent& event) const {
        return std::nullopt;
    }

protected:
    std::shared_ptr<Widget> widget() const { return widget_.lock(); }

private:
    friend class Viewport;
    std::weak_ptr<Widget> widget_;
};

class Viewport : public CompoundWidget {
public:
    struct Args : ::cinder::ui::Args<Args, Viewport> {
        UI_ARG(std::shared_ptr<ViewportClient>, client)
        UI_ARG(bool, opaque, true)
        UI_ARG(bool, focusable, true)
        UI_CONTENT(content)
    };

    void construct(const Args& args);

    const std::shared_ptr<ViewportClient>& client() const { return client_; }

    bool supportsKeyboardFocus() const override { return focusable_; }
    void tick(const Geometry& geometry, double time, float deltaTime) override;

    Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseUp(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseMove(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseDoubleClick(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseWheel(const Geometry& geometry, const PointerEvent& event) override;
    void onMouseEnter(const Geometry& geometry, const PointerEvent& event) override;
    void onMouseLeave(const PointerEvent& event) override;
    void onMouseCaptureLost() override;
    Reply onKeyDown(const Geometry& geometry, const KeyEvent& event) override;
    Reply onKeyUp(const Geometry& geometry, const KeyEvent& event) override;
    Reply onKeyChar(const Geometry& geometry, const CharEvent& event) override;
    Reply onFocusReceived(const Geometry& geometry, const FocusEvent& event) override;
    void onFocusLost(const FocusEvent& event) override;
    std::optional<cinder::platform::CursorShape> onCursorQuery(const Geometry& geometry,
                                                               const PointerEvent& event) const override;

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override { return glm::vec2(0.0f); }
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    std::shared_ptr<ViewportClient> client_;
    bool opaque_ = true;
    bool focusable_ = true;
};

}
