#pragma once

#include <memory>

namespace cinder::ui {

class Widget;
class DragDropOperation;

class Reply {
public:
    static Reply handled() { return Reply(true); }
    static Reply unhandled() { return Reply(false); }

    bool isHandled() const { return handled_; }

    Reply& captureMouse(std::shared_ptr<Widget> widget) {
        captor_ = std::move(widget);
        return *this;
    }

    Reply& releaseMouseCapture() {
        releaseCapture_ = true;
        return *this;
    }

    Reply& setFocus(std::shared_ptr<Widget> widget) {
        focus_ = std::move(widget);
        focusChange_ = true;
        return *this;
    }

    Reply& clearFocus() {
        focus_.reset();
        focusChange_ = true;
        return *this;
    }

    Reply& beginDragDrop(std::shared_ptr<DragDropOperation> operation) {
        dragDrop_ = std::move(operation);
        return *this;
    }

    Reply& detectDrag(std::shared_ptr<Widget> widget, int button) {
        dragDetector_ = std::move(widget);
        dragButton_ = button;
        return *this;
    }

    const std::shared_ptr<Widget>& captor() const { return captor_; }
    bool releasesCapture() const { return releaseCapture_; }
    bool changesFocus() const { return focusChange_; }
    const std::shared_ptr<Widget>& focus() const { return focus_; }
    const std::shared_ptr<Widget>& dragDetector() const { return dragDetector_; }
    int dragButton() const { return dragButton_; }
    const std::shared_ptr<DragDropOperation>& dragDrop() const { return dragDrop_; }

private:
    explicit Reply(bool handled) : handled_(handled) {}

    bool handled_ = false;
    bool releaseCapture_ = false;
    bool focusChange_ = false;
    int dragButton_ = -1;
    std::shared_ptr<Widget> captor_;
    std::shared_ptr<Widget> focus_;
    std::shared_ptr<Widget> dragDetector_;
    std::shared_ptr<DragDropOperation> dragDrop_;
};

}
