#pragma once

#include "UI/Core/Events.hpp"

#include <memory>

namespace cinder::UI {

class Widget;

class DragDropOperation {
public:
    virtual ~DragDropOperation() = default;

    virtual std::shared_ptr<Widget> decorator() const { return nullptr; }
    virtual void onDropped(bool accepted) {}
};

struct DragDropEvent : PointerEvent {
    std::shared_ptr<DragDropOperation> operation;
};

}
