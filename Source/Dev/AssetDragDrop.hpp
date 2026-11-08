#pragma once

#include "Dev/AssetCatalog.hpp"
#include "UI/Core/DragDrop.hpp"

#include <memory>
#include <string>

namespace cinder::Dev {

class AssetDragDrop : public cinder::UI::DragDropOperation {
public:
    AssetDragDrop(std::string path, AssetKind kind);

    const std::string& path() const { return path_; }
    AssetKind kind() const { return kind_; }
    std::shared_ptr<cinder::UI::Widget> decorator() const override { return decorator_; }

    static const AssetDragDrop* carried(const cinder::UI::DragDropEvent& event, AssetKind kind);

private:
    std::string path_;
    AssetKind kind_;
    std::shared_ptr<cinder::UI::Widget> decorator_;
};

}
