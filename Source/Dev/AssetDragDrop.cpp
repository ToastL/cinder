#include "Dev/AssetDragDrop.hpp"

#include "UI/Framework/Application.hpp"
#include "UI/Widgets/Border.hpp"
#include "UI/Widgets/Label.hpp"
#include "UI/Widgets/Menu.hpp"

#include <filesystem>
#include <utility>

namespace cinder::Dev {

using namespace cinder::UI;

AssetDragDrop::AssetDragDrop(std::string path, AssetKind kind) : path_(std::move(path)), kind_(kind) {
    const std::string label = std::string(AssetCatalog::label(kind_)) + "  "
                              + std::filesystem::path(path_).filename().string();
    decorator_ = make<Border>()
            .brush([] { return Application::get().theme().get<MenuStyle>("Menu").background; })
            .padding(Margin(8.0f, 4.0f))
            [make<Label>().text(label)];
}

const AssetDragDrop* AssetDragDrop::carried(const DragDropEvent& event, AssetKind kind) {
    const auto* operation = dynamic_cast<const AssetDragDrop*>(event.operation.get());
    return operation != nullptr && operation->kind() == kind ? operation : nullptr;
}

}
