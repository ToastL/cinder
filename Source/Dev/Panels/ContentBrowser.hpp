#pragma once

#include "Dev/AssetCatalog.hpp"
#include "Dev/ContentActions.hpp"
#include "UI/Core/Widget.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cinder::UI {
class Application;
class HorizontalBox;
class MenuBuilder;
class TileView;
class TreeView;
}

namespace cinder::Dev::Panels {

class ContentBrowser {
public:
    static constexpr std::size_t NAME_CHARS = 13;

    ContentBrowser(AssetCatalog& catalog, cinder::UI::Application& app);

    ContentBrowser(const ContentBrowser&) = delete;
    ContentBrowser& operator=(const ContentBrowser&) = delete;

    const std::shared_ptr<cinder::UI::Widget>& widget() const { return widget_; }
    const std::shared_ptr<cinder::UI::TreeView>& folders() const { return folders_; }
    const std::shared_ptr<cinder::UI::TileView>& tiles() const { return tiles_; }
    const std::shared_ptr<cinder::UI::HorizontalBox>& crumbs() const { return crumbs_; }

    void setActions(ContentActions actions) { actions_ = std::move(actions); }
    void update();

    const std::string& folder() const { return folder_; }
    void navigate(const std::string& folder);
    void reveal(const std::string& path);
    void setFilter(std::string filter) { filter_ = std::move(filter); }
    std::optional<std::string> selectedPath() const;

    void activate(std::uint64_t id);
    void requestDelete(const std::string& path);
    void newFolder(const std::string& parent);

    static std::string shortName(const std::string& name);

private:
    std::vector<std::uint64_t> items() const;
    std::shared_ptr<cinder::UI::Widget> tileFor(std::uint64_t id);
    std::shared_ptr<cinder::UI::Widget> thumbnailFor(const AssetEntry& entry) const;
    std::shared_ptr<cinder::UI::Widget> tileMenu(std::optional<std::uint64_t> id);
    std::shared_ptr<cinder::UI::Widget> folderMenu(std::optional<std::uint64_t> id);
    void rebuildCrumbs();
    void showInFinder(const std::string& path) const;
    void copyPath(const std::string& path) const;

    AssetCatalog& catalog_;
    cinder::UI::Application& app_;
    ContentActions actions_;
    std::shared_ptr<cinder::UI::Widget> widget_;
    std::shared_ptr<cinder::UI::TreeView> folders_;
    std::shared_ptr<cinder::UI::TileView> tiles_;
    std::shared_ptr<cinder::UI::HorizontalBox> crumbs_;
    std::string folder_;
    std::string filter_;
    std::uint64_t revision_ = 0;
    bool crumbsDirty_ = true;
};

}
