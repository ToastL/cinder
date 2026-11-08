#pragma once

#include "UI/Core/Args.hpp"
#include "UI/Core/CompoundWidget.hpp"

#include <functional>

namespace cinder::UI {

class DropTarget : public CompoundWidget {
public:
    using CanAccept = std::function<bool(const DragDropEvent&)>;
    using Accept = std::function<void(const DragDropEvent&)>;

    struct Args : ::cinder::UI::Args<Args, DropTarget> {
        UI_EVENT(CanAccept, onCanAccept)
        UI_EVENT(Accept, onDrop)
        UI_CONTENT(content)
    };

    void construct(const Args& args);

    bool hovered() const { return hovered_; }

    void onDragLeave(const DragDropEvent& event) override;
    Reply onDragOver(const Geometry& geometry, const DragDropEvent& event) override;
    Reply onDrop(const Geometry& geometry, const DragDropEvent& event) override;

protected:
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    CanAccept canAccept_;
    Accept accept_;
    bool hovered_ = false;
};

}
