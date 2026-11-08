#include "Dev/Panels/ContentBrowser.hpp"

#include "Dev/AssetDragDrop.hpp"
#include "Dev/Process.hpp"
#include "Dev/Panels/EditorStyle.hpp"
#include "Platform/Assets.hpp"
#include "Platform/Log.hpp"
#include "UI/Framework/Application.hpp"
#include "UI/Widgets/Border.hpp"
#include "UI/Widgets/BoxPanel.hpp"
#include "UI/Widgets/Button.hpp"
#include "UI/Widgets/ComboBox.hpp"
#include "UI/Widgets/Label.hpp"
#include "UI/Widgets/Menu.hpp"
#include "UI/Widgets/SizeBox.hpp"
#include "UI/Widgets/Splitter.hpp"
#include "UI/Widgets/TextField.hpp"
#include "UI/Widgets/TileView.hpp"
#include "UI/Widgets/TreeView.hpp"

#include <exception>
#include <filesystem>
#include <string_view>

namespace cinder::Dev::Panels {

using namespace cinder::UI;
namespace keys = cinder::Platform::keys;

namespace {

constexpr float THUMBNAIL = 64.0f;

std::string_view kindColor(AssetKind kind) {
    switch (kind) {
        case AssetKind::Folder: return "Color.Warning";
        case AssetKind::Scene: return "Color.Success";
        case AssetKind::Prefab: return "Color.Primary";
        case AssetKind::Texture: return "Color.AxisX";
        case AssetKind::Other: return "Color.Outline";
    }
    return "Color.Outline";
}

std::string badge(const AssetEntry& entry) {
    if (entry.kind == AssetKind::Folder) return "Folder";
    std::string extension = std::filesystem::path(entry.name).extension().string();
    if (!extension.empty()) extension.erase(0, 1);
    for (char& c : extension) {
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    }
    return extension.empty() ? std::string("File") : extension;
}

std::string parentOf(const std::string& path) { return std::filesystem::path(path).parent_path().generic_string(); }

bool openable(AssetKind kind) { return kind == AssetKind::Scene || kind == AssetKind::Prefab; }

}

ContentBrowser::ContentBrowser(AssetCatalog& catalog, Application& app) : catalog_(catalog), app_(app) {
    catalog_.refresh();
    revision_ = catalog_.revision();

    auto folders = make<TreeView>()
            .assign(folders_)
            .treeItemsSource([] { return std::vector<ItemId>{AssetCatalog::ROOT}; })
            .onGetChildren([this](ItemId item) { return catalog_.subfolders(item); })
            .onGetParent([this](ItemId item) -> std::optional<ItemId> {
                const AssetEntry* entry = catalog_.byId(item);
                if (entry == nullptr || item == AssetCatalog::ROOT) return std::nullopt;
                return entry->parent;
            })
            .onGenerateRow([this](ItemId item) {
                return make<Label>().text([this, item] {
                    const AssetEntry* entry = catalog_.byId(item);
                    return entry != nullptr ? entry->name : std::string();
                });
            })
            .isItemSelected([this](ItemId item) { return filter_.empty() && AssetCatalog::idOf(folder_) == item; })
            .onSelectionChanged([this](std::optional<ItemId> item, SelectInfo) {
                if (!item) return;
                if (const AssetEntry* entry = catalog_.byId(*item)) navigate(entry->path);
            })
            .onContextMenuOpening([this](std::optional<ItemId> item) { return folderMenu(item); });

    auto tiles = make<TileView>()
            .assign(tiles_)
            .itemsSource([this] { return items(); })
            .onGenerateTile([this](ItemId item) { return tileFor(item); })
            .onMouseButtonDoubleClick([this](ItemId item) { activate(item); })
            .onContextMenuOpening([this](std::optional<ItemId> item) { return tileMenu(item); })
            .onDragDetected([this](ItemId item) -> std::shared_ptr<DragDropOperation> {
                const AssetEntry* entry = catalog_.byId(item);
                if (entry == nullptr || entry->kind == AssetKind::Folder) return nullptr;
                return std::make_shared<AssetDragDrop>(entry->path, entry->kind);
            })
            .onKeyDownHandler([this](const KeyEvent& event) {
                if (event.key != keys::DELETE_KEY && event.key != keys::BACKSPACE) return false;
                if (const std::optional<std::string> path = selectedPath()) requestDelete(*path);
                return true;
            });

    crumbs_ = make<HorizontalBox>();

    auto header = make<HorizontalBox>()
            + HorizontalBox::slot().autoWidth().padding(Margin(0.0f, 0.0f, 6.0f, 0.0f))
                  [make<ComboButton>()
                           .buttonStyle("Button.Small")
                           .hasDownArrow(false)
                           .onGetMenuContent([this] {
                               MenuBuilder menu;
                               menu.entry("New Folder", [this] { newFolder(folder_); });
                               return menu.build();
                           })[make<Label>().text("+").toolTipText("Add to this folder")]]
            + HorizontalBox::slot().fill(1.0f).vAlign(VAlign::Center)[crumbs_]
            + HorizontalBox::slot().autoWidth().vAlign(VAlign::Center)
                  [make<SizeBox>().widthOverride(180.0f)
                       [make<TextBox>().hintText("Search").text([this] { return filter_; }).onTextChanged([this](const std::string& text) {
                           filter_ = text;
                       })]];

    std::shared_ptr<Widget> right = make<VerticalBox>()
            + VerticalBox::slot().autoHeight()[titleBar(header, Margin(6.0f, 3.0f))]
            + VerticalBox::slot().fill(1.0f).padding(Margin(4.0f))[tiles];

    widget_ = make<Splitter>()
            + Splitter::slot().value(1.0f)[folders]
            + Splitter::slot().value(4.0f)[right];
}

std::vector<std::uint64_t> ContentBrowser::items() const {
    if (!filter_.empty()) return catalog_.search(filter_);
    return catalog_.contents(AssetCatalog::idOf(folder_));
}

std::string ContentBrowser::shortName(const std::string& name) {
    if (name.size() <= NAME_CHARS) return name;
    std::size_t cut = NAME_CHARS - 1;
    while (cut > 0 && (static_cast<unsigned char>(name[cut]) & 0xC0) == 0x80) --cut;
    return name.substr(0, cut) + "\xE2\x80\xA6";
}

std::shared_ptr<Widget> ContentBrowser::thumbnailFor(const AssetEntry& entry) const {
    if (entry.kind == AssetKind::Texture) {
        return make<Image>().image(Brush::picture(entry.path, glm::vec2(THUMBNAIL)));
    }
    const std::string colorName(kindColor(entry.kind));
    return make<Border>()
            .brush([colorName] {
                Color fill = Application::get().theme().color(colorName);
                fill.a = 0.35f;
                return Brush::rounded(fill, 6.0f, Application::get().theme().color(colorName), 1.0f);
            })
            .hAlign(HAlign::Center)
            .vAlign(VAlign::Center)
            [make<Label>().text(badge(entry)).textStyle("Label.Small")];
}

std::shared_ptr<Widget> ContentBrowser::tileFor(std::uint64_t id) {
    const AssetEntry* entry = catalog_.byId(id);
    if (entry == nullptr) return make<Spacer>();
    const bool folder = entry->kind == AssetKind::Folder;
    const std::string name = folder ? entry->name : std::filesystem::path(entry->name).stem().string();
    auto tile = make<VerticalBox>().toolTipText(entry->path + "\n" + std::string(AssetCatalog::label(entry->kind)))
                + VerticalBox::slot().autoHeight().hAlign(HAlign::Center).padding(Margin(0.0f, 6.0f, 0.0f, 4.0f))
                      [make<SizeBox>().widthOverride(THUMBNAIL).heightOverride(THUMBNAIL)[thumbnailFor(*entry)]]
                + VerticalBox::slot().autoHeight().hAlign(HAlign::Center).padding(Margin(2.0f, 0.0f))
                      [make<Label>().text(shortName(name))];
    if (!folder) {
        tile + VerticalBox::slot().autoHeight().hAlign(HAlign::Center)
                       [make<Label>()
                                .text(std::string(AssetCatalog::label(entry->kind)))
                                .textStyle("Label.Small")
                                .colorAndOpacity(themeColor("Color.ForegroundDim"))];
    }
    return tile;
}

std::optional<std::string> ContentBrowser::selectedPath() const {
    const std::vector<ItemId> selected = tiles_->selectedItems();
    if (selected.empty()) return std::nullopt;
    const AssetEntry* entry = catalog_.byId(selected.front());
    if (entry == nullptr) return std::nullopt;
    return entry->path;
}

void ContentBrowser::navigate(const std::string& folder) {
    const AssetEntry* entry = catalog_.byPath(folder);
    if (entry == nullptr || entry->kind != AssetKind::Folder) return;
    if (folder_ != entry->path) {
        folder_ = entry->path;
        tiles_->clearSelection();
        crumbsDirty_ = true;
    }
    folders_->reveal(entry->id);
}

void ContentBrowser::reveal(const std::string& path) {
    const AssetEntry* entry = catalog_.byPath(path);
    if (entry == nullptr) {
        catalog_.refresh();
        entry = catalog_.byPath(path);
        if (entry == nullptr) return;
    }
    filter_.clear();
    navigate(parentOf(entry->path));
    tiles_->setSelection(entry->id);
}

void ContentBrowser::activate(std::uint64_t id) {
    const AssetEntry* entry = catalog_.byId(id);
    if (entry == nullptr) return;
    if (entry->kind == AssetKind::Folder) {
        filter_.clear();
        navigate(entry->path);
        return;
    }
    if (!openable(entry->kind) || !actions_.openScene || !actions_.available()) return;
    try {
        actions_.openScene(cinder::Platform::contentPath(entry->path));
    } catch (const std::exception& error) {
        cinder::Platform::logError("[content] %s\n", error.what());
    }
}

void ContentBrowser::newFolder(const std::string& parent) {
    if (const std::optional<std::string> made = catalog_.createFolder(parent)) {
        navigate(parent);
        tiles_->setSelection(AssetCatalog::idOf(*made));
    } else {
        cinder::Platform::logError("[content] could not create a folder in Content/%s\n", parent.c_str());
    }
}

void ContentBrowser::requestDelete(const std::string& path) {
    if (path.empty() || !actions_.available()) return;
    const auto remove = [this, path] {
        std::string error;
        const std::filesystem::path keep = actions_.openPath ? actions_.openPath() : std::filesystem::path();
        if (catalog_.remove(path, keep, &error)) cinder::Platform::logInfo("[content] deleted %s\n", path.c_str());
        else cinder::Platform::logError("[content] %s\n", error.c_str());
    };
    if (!actions_.confirm) {
        remove();
        return;
    }
    const AssetEntry* entry = catalog_.byPath(path);
    const bool folder = entry != nullptr && entry->kind == AssetKind::Folder;
    actions_.confirm(folder ? "Delete Folder" : "Delete Asset",
                     "Delete " + path + (folder ? " and everything in it" : "")
                             + "? Scenes that name it will draw it missing. This cannot be undone.",
                     "Delete", remove);
}

void ContentBrowser::showInFinder(const std::string& path) const {
    std::filesystem::path target;
    try {
        target = path.empty() ? AssetCatalog::root() : cinder::Platform::contentPath(path);
    } catch (const std::exception&) {
        return;
    }
#if defined(__APPLE__)
    const std::vector<std::string> argv = {"open", "-R", target.string()};
#else
    const std::vector<std::string> argv = {"xdg-open", target.parent_path().string()};
#endif
    std::string error;
    if (std::unique_ptr<Process> process = Process::start(argv, error)) {
        process->wait();
    } else {
        cinder::Platform::logError("[content] %s\n", error.c_str());
    }
}

void ContentBrowser::copyPath(const std::string& path) const { app_.platform().setClipboard(path); }

std::shared_ptr<Widget> ContentBrowser::tileMenu(std::optional<std::uint64_t> id) {
    MenuBuilder menu;
    const AssetEntry* entry = id ? catalog_.byId(*id) : nullptr;
    const auto enabled = [this] { return actions_.available(); };
    if (entry == nullptr) {
        const std::string here = folder_;
        menu.entry("New Folder", [this, here] { newFolder(here); });
        menu.entry("Show in Finder", [this, here] { showInFinder(here); });
        return menu.build();
    }
    const std::string path = entry->path;
    const std::uint64_t item = entry->id;
    if (entry->kind == AssetKind::Folder || openable(entry->kind)) {
        menu.entry("Open", [this, item] { activate(item); }).enabledIf(enabled);
    }
    menu.entry("Show in Finder", [this, path] { showInFinder(path); });
    menu.entry("Copy Path", [this, path] { copyPath(path); });
    menu.separator();
    menu.entry("Delete...", [this, path] { requestDelete(path); }).enabledIf(enabled);
    return menu.build();
}

std::shared_ptr<Widget> ContentBrowser::folderMenu(std::optional<std::uint64_t> id) {
    const AssetEntry* entry = id ? catalog_.byId(*id) : catalog_.byId(AssetCatalog::ROOT);
    if (entry == nullptr) return nullptr;
    const std::string path = entry->path;
    MenuBuilder menu;
    menu.entry("New Folder", [this, path] { newFolder(path); });
    menu.entry("Show in Finder", [this, path] { showInFinder(path); });
    if (!path.empty()) {
        menu.entry("Copy Path", [this, path] { copyPath(path); });
        menu.separator();
        menu.entry("Delete...", [this, path] { requestDelete(path); }).enabledIf([this] { return actions_.available(); });
    }
    return menu.build();
}

void ContentBrowser::rebuildCrumbs() {
    crumbs_->clearChildren();
    std::vector<std::string> parts;
    std::string at;
    parts.push_back(at);
    for (const auto& part : std::filesystem::path(folder_)) {
        at = at.empty() ? part.generic_string() : at + "/" + part.generic_string();
        parts.push_back(at);
    }
    for (std::size_t i = 0; i < parts.size(); ++i) {
        const std::string target = parts[i];
        const std::string label = target.empty() ? std::string("Content") : std::filesystem::path(target).filename().string();
        if (i > 0) {
            crumbs_->addSlot(HorizontalBox::slot().autoWidth().vAlign(VAlign::Center).padding(Margin(2.0f, 0.0f))
                                     [make<Label>().text("/").colorAndOpacity(themeColor("Color.ForegroundDim"))]);
        }
        crumbs_->addSlot(HorizontalBox::slot().autoWidth().vAlign(VAlign::Center)
                                 [make<Button>().text(label).buttonStyle("Button.Small").onClicked([this, target] {
                                     filter_.clear();
                                     navigate(target);
                                     return Reply::handled();
                                 })]);
    }
    crumbsDirty_ = false;
}

void ContentBrowser::update() {
    catalog_.poll();
    if (catalog_.revision() != revision_) {
        revision_ = catalog_.revision();
        folders_->refresh();
        tiles_->refresh();
        std::string folder = folder_;
        while (!folder.empty() && catalog_.byPath(folder) == nullptr) folder = parentOf(folder);
        if (folder != folder_) {
            folder_ = folder;
            crumbsDirty_ = true;
        }
    }
    if (crumbsDirty_) rebuildCrumbs();
}

}
