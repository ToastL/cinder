#include <doctest/doctest.h>

#include "ui_harness.hpp"

#include "ui/widgets/Canvas.hpp"
#include "ui/widgets/TextField.hpp"

#include <string>
#include <vector>

using namespace cinder::ui;
using uitest::Harness;
namespace keys = cinder::platform::keys;
namespace modifiers = cinder::platform::modifiers;

namespace {

#if defined(__APPLE__)
constexpr std::uint8_t WORD = modifiers::ALT;
#else
constexpr std::uint8_t WORD = modifiers::CONTROL;
#endif

struct Field {
    std::shared_ptr<TextField> field;
    std::vector<std::pair<std::string, TextCommit>> commits;
    std::vector<std::string> changes;
    Harness ui;

    explicit Field(std::string text, bool clearOnCommit = true)
        : ui(make<Canvas>() + Canvas::slot().offset(Margin(0.0f, 0.0f, 300.0f, 24.0f))
                                      [make<TextField>()
                                               .assign(field)
                                               .text(std::move(text))
                                               .clearFocusOnCommit(clearOnCommit)
                                               .onTextChanged([this](const std::string& now) { changes.push_back(now); })
                                               .onTextCommitted([this](const std::string& now, TextCommit how) {
                                                   commits.emplace_back(now, how);
                                               })]) {}

    void focusEnd() {
        ui.click(290.0f, 12.0f);
        REQUIRE(field->hasFocus());
    }
};

}

TEST_CASE("typing inserts at the caret and Enter commits and lets go of focus") {
    Field f("cinder");
    f.focusEnd();
    CHECK(f.field->caret() == 6);
    f.ui.type(U" engine");
    CHECK(f.field->text() == "cinder engine");
    CHECK(f.changes.back() == "cinder engine");
    f.ui.key(keys::ENTER);
    REQUIRE(f.commits.size() == 1);
    CHECK(f.commits[0].first == "cinder engine");
    CHECK(f.commits[0].second == TextCommit::Enter);
    CHECK_FALSE(f.field->hasFocus());
    CHECK(f.field->text() == "cinder engine");
}

TEST_CASE("Escape reverts the edit and clicking away commits it") {
    Field reverted("a");
    reverted.focusEnd();
    reverted.ui.type(U"bc");
    reverted.ui.key(keys::ESCAPE);
    CHECK(reverted.field->text() == "a");
    REQUIRE(reverted.commits.size() == 1);
    CHECK(reverted.commits[0].second == TextCommit::Cleared);

    Field moved("a");
    moved.focusEnd();
    moved.ui.type(U"b");
    moved.ui.click(350.0f, 200.0f);
    REQUIRE(moved.commits.size() == 1);
    CHECK(moved.commits[0].first == "ab");
    CHECK(moved.commits[0].second == TextCommit::FocusLost);

    Field untouched("a");
    untouched.focusEnd();
    untouched.ui.click(350.0f, 200.0f);
    CHECK(untouched.commits.empty());
}

TEST_CASE("arrows move by character, by word and to the ends, and shift selects") {
    Field f("hello big world");
    f.focusEnd();
    f.ui.key(keys::LEFT, WORD);
    CHECK(f.field->caret() == 10);
    f.ui.key(keys::LEFT, WORD | modifiers::SHIFT);
    CHECK(f.field->caret() == 6);
    CHECK(f.field->selectedText() == "big ");
    f.ui.key(keys::RIGHT);
    CHECK_FALSE(f.field->hasSelection());
    CHECK(f.field->caret() == 10);
    f.ui.key(keys::HOME);
    CHECK(f.field->caret() == 0);
    f.ui.key(keys::RIGHT, modifiers::SHIFT);
    f.ui.key(keys::RIGHT, modifiers::SHIFT);
    CHECK(f.field->selectedText() == "he");
    f.ui.type(U"J");
    CHECK(f.field->text() == "Jllo big world");
}

TEST_CASE("backspace and delete remove characters, words and selections") {
    Field f("one two\xC3\xA9");
    f.focusEnd();
    f.ui.key(keys::BACKSPACE);
    CHECK(f.field->text() == "one two");
    f.ui.key(keys::BACKSPACE, WORD);
    CHECK(f.field->text() == "one ");
    f.ui.key(keys::HOME);
    f.ui.key(keys::DELETE);
    CHECK(f.field->text() == "ne ");
    f.ui.key(keys::letter('a'), modifiers::PRIMARY);
    f.ui.key(keys::DELETE);
    CHECK(f.field->text().empty());
}

TEST_CASE("copy, cut and paste use the clipboard, and paste stays on one line") {
    Field f("alpha beta");
    f.focusEnd();
    f.ui.key(keys::LEFT, WORD | modifiers::SHIFT);
    f.ui.key(keys::letter('c'), modifiers::PRIMARY);
    CHECK(f.ui.platform.clipboard() == "beta");
    f.ui.key(keys::letter('x'), modifiers::PRIMARY);
    CHECK(f.field->text() == "alpha ");
    f.ui.platform.setClipboard("gamma\ndelta");
    f.ui.key(keys::letter('v'), modifiers::PRIMARY);
    CHECK(f.field->text() == "alpha gamma delta");
}

TEST_CASE("undo steps back over typing runs and redo returns") {
    Field f("");
    f.focusEnd();
    f.ui.type(U"abc");
    f.ui.key(keys::LEFT);
    f.ui.type(U"X");
    CHECK(f.field->text() == "abXc");
    f.ui.key(keys::letter('z'), modifiers::PRIMARY);
    CHECK(f.field->text() == "abc");
    f.ui.key(keys::letter('z'), modifiers::PRIMARY);
    CHECK(f.field->text().empty());
    f.ui.key(keys::letter('z'), modifiers::PRIMARY | modifiers::SHIFT);
    CHECK(f.field->text() == "abc");
}

TEST_CASE("a field that keeps focus on commit is ready for the next line") {
    Field f("", false);
    f.focusEnd();
    f.ui.type(U"print(1)");
    f.ui.key(keys::ENTER);
    CHECK(f.field->hasFocus());
    REQUIRE(f.commits.size() == 1);
    f.field->setText("");
    f.ui.type(U"x");
    CHECK(f.field->text() == "x");
}

TEST_CASE("a click places the caret under the pointer and a double click selects the word") {
    Field f("hello world");
    f.focusEnd();
    f.ui.click(2.0f, 12.0f);
    CHECK(f.field->caret() == 0);
    f.ui.platform.advance(1.0);
    f.ui.click(60.0f, 12.0f);
    f.ui.platform.advance(0.1);
    f.ui.click(60.0f, 12.0f);
    CHECK(f.field->selectedText() == "world");
}
