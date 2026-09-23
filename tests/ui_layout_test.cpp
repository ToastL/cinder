#include <doctest/doctest.h>

#include "ui_harness.hpp"

#include "ui/widgets/Border.hpp"
#include "ui/widgets/BoxPanel.hpp"
#include "ui/widgets/Canvas.hpp"
#include "ui/widgets/Label.hpp"
#include "ui/widgets/ScrollBox.hpp"
#include "ui/widgets/SizeBox.hpp"
#include "ui/widgets/Splitter.hpp"

using namespace cinder::ui;
using uitest::Harness;
using uitest::Probe;

TEST_CASE("a horizontal box gives auto slots their width and shares the rest by fill") {
    std::shared_ptr<Probe> first;
    std::shared_ptr<Probe> second;
    std::shared_ptr<Probe> third;
    std::shared_ptr<Widget> root = make<HorizontalBox>()
            + HorizontalBox::slot().autoWidth().padding(Margin(10.0f, 0.0f))[make<Probe>().assign(first).size({50.0f, 20.0f})]
            + HorizontalBox::slot().fill(1.0f)[make<Probe>().assign(second)]
            + HorizontalBox::slot().fill(3.0f).vAlign(VAlign::Center)[make<Probe>().assign(third).size({5.0f, 40.0f})];
    Harness ui(root);

    CHECK(ui.rectOf(first) == Rect::fromSize({10.0f, 0.0f}, {50.0f, 300.0f}));
    CHECK(ui.rectOf(second) == Rect::fromSize({70.0f, 0.0f}, {82.5f, 300.0f}));
    CHECK(ui.rectOf(third) == Rect::fromSize({152.5f, 130.0f}, {247.5f, 40.0f}));
    CHECK(root->desiredSize() == glm::vec2(70.0f + 20.0f + 5.0f, 40.0f));
}

TEST_CASE("a vertical box honours max height and skips collapsed children") {
    std::shared_ptr<Probe> top;
    std::shared_ptr<Probe> gone;
    std::shared_ptr<Probe> rest;
    Harness ui(make<VerticalBox>()
               + VerticalBox::slot().maxHeight(40.0f)[make<Probe>().assign(top)]
               + VerticalBox::slot().autoHeight()[make<Probe>().assign(gone).visibility(Visibility::Collapsed)]
               + VerticalBox::slot()[make<Probe>().assign(rest)]);

    CHECK(ui.rectOf(top).height() == doctest::Approx(40.0f));
    CHECK_FALSE(ui.painted(gone));
    CHECK(ui.rectOf(rest).min.y == doctest::Approx(40.0f));
    CHECK(ui.rectOf(rest).height() == doctest::Approx(150.0f));
}

TEST_CASE("a hidden widget keeps its space but is neither painted nor hit") {
    std::shared_ptr<Probe> hidden;
    std::shared_ptr<Probe> after;
    Harness ui(make<VerticalBox>()
               + VerticalBox::slot().autoHeight()[make<Probe>().assign(hidden).visibility(Visibility::Hidden)]
               + VerticalBox::slot().autoHeight()[make<Probe>().assign(after)]);
    CHECK_FALSE(ui.painted(hidden));
    CHECK(ui.rectOf(after).min.y == doctest::Approx(20.0f));
}

TEST_CASE("a size box overrides and limits, and aligns what it holds") {
    std::shared_ptr<Probe> inner;
    std::shared_ptr<SizeBox> box;
    Harness ui(make<Overlay>()
               + Overlay::slot().hAlign(HAlign::Left).vAlign(VAlign::Top)
                         [make<SizeBox>().assign(box).widthOverride(100.0f).minDesiredHeight(60.0f)
                                  .hAlign(HAlign::Center).vAlign(VAlign::Bottom)[make<Probe>().assign(inner)]]);
    CHECK(box->desiredSize() == glm::vec2(100.0f, 60.0f));
    CHECK(ui.rectOf(inner) == Rect::fromSize({40.0f, 40.0f}, {20.0f, 20.0f}));
}

TEST_CASE("a border pads its content and a scaler scales it") {
    std::shared_ptr<Probe> inner;
    std::shared_ptr<Widget> root = make<Border>().padding(Margin(4.0f, 8.0f))
            [make<Scaler>().dpiScale(2.0f)[make<Probe>().assign(inner).size({10.0f, 10.0f})]];
    Harness ui(root);
    const Geometry geometry = ui.geometryOf(inner);
    CHECK(geometry.scale == doctest::Approx(2.0f));
    CHECK(geometry.position == glm::vec2(4.0f, 8.0f));
    CHECK(geometry.size == glm::vec2(196.0f, 142.0f));
    CHECK(root->desiredSize() == glm::vec2(28.0f, 36.0f));
}

TEST_CASE("a canvas places anchored slots, point and stretched") {
    std::shared_ptr<Probe> centred;
    std::shared_ptr<Probe> stretched;
    std::shared_ptr<Probe> sized;
    Harness ui(make<Canvas>()
               + Canvas::slot().anchors({0.5f, 0.5f}).alignment({0.5f, 0.5f}).offset(Margin(0.0f, 0.0f, 80.0f, 20.0f))
                         [make<Probe>().assign(centred)]
               + Canvas::slot().anchors({0.0f, 1.0f}, {1.0f, 1.0f}).offset(Margin(10.0f, -30.0f, 10.0f, 20.0f))
                         [make<Probe>().assign(stretched)]
               + Canvas::slot().anchors({1.0f, 0.0f}).alignment({1.0f, 0.0f}).autoSize(true)
                         [make<Probe>().assign(sized).size({30.0f, 12.0f})]);
    CHECK(ui.rectOf(centred) == Rect::fromSize({160.0f, 140.0f}, {80.0f, 20.0f}));
    CHECK(ui.rectOf(stretched) == Rect::fromSize({10.0f, 270.0f}, {380.0f, 20.0f}));
    CHECK(ui.rectOf(sized) == Rect::fromSize({370.0f, 0.0f}, {30.0f, 12.0f}));
}

TEST_CASE("a splitter divides by value between its handles") {
    std::shared_ptr<Probe> left;
    std::shared_ptr<Probe> right;
    Harness ui(make<Splitter>()
               + Splitter::slot().value(1.0f)[make<Probe>().assign(left)]
               + Splitter::slot().value(3.0f)[make<Probe>().assign(right)]);
    CHECK(ui.rectOf(left).width() == doctest::Approx(99.0f));
    CHECK(ui.rectOf(right).min.x == doctest::Approx(103.0f));
    CHECK(ui.rectOf(right).max.x == doctest::Approx(400.0f));
}

TEST_CASE("a label measures its text by the font's line height") {
    std::shared_ptr<Label> label;
    Harness ui(make<Overlay>() + Overlay::slot().hAlign(HAlign::Left).vAlign(VAlign::Top)
                                         [make<Label>().assign(label).text("Properties")]);
    const glm::vec2 one = label->desiredSize();
    CHECK(one.x > 40.0f);
    CHECK(one.y >= 15.0f);
    label->setText("Properties\nExplorer");
    ui.frame();
    CHECK(label->desiredSize().y == doctest::Approx(one.y * 2.0f));
}

TEST_CASE("a scroll box clips to its viewport, shows a bar and scrolls by the wheel") {
    std::shared_ptr<ScrollBox> scroll;
    auto box = make<ScrollBox>().assign(scroll);
    for (int i = 0; i < 20; ++i) box + ScrollBox::slot()[make<Probe>().size({50.0f, 30.0f}).handles(false)];
    Harness ui(box);

    CHECK(scroll->scrollMax() == doctest::Approx(600.0f - 300.0f));
    ui.move(100.0f, 100.0f);
    ui.wheel(-2.0f);
    ui.frame();
    CHECK(scroll->scrollOffset() == doctest::Approx(2.0f * ScrollBox::WHEEL_STEP));
    ui.wheel(50.0f);
    CHECK(scroll->scrollOffset() == doctest::Approx(0.0f));
    scroll->scrollToEnd();
    ui.frame();
    CHECK(scroll->atEnd());
}
