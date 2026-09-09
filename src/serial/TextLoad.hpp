#pragma once

#include "serial/Archive.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace cinder::serial {

class TextLoad : public Archive {
public:
    static TextLoad parse(const std::string& source);

    bool loading() const override { return true; }

    bool enterRecord(std::string_view name) override;
    void leaveRecord() override;

    int enterArray(std::string_view name, int count) override;
    void enterItem(int index, std::string_view type) override;
    std::string itemType(int index) override;
    void leaveItem() override;
    void leaveArray() override;

    int integer(std::string_view name, int value, int fallback) override;
    bool flag(std::string_view name, bool value, bool fallback) override;
    std::string text(std::string_view name, std::string_view value,
                     std::string_view fallback) override;
    void vector(std::string_view name, float* values, const float* fallback, int arity) override;

    cinder::scene::PropRec bag(std::string_view name, const cinder::scene::PropRec& values) override;

private:
    struct Field {
        std::vector<std::string> tokens;
        bool quoted = false;
    };

    struct Node {
        std::string name;
        std::unordered_map<std::string, Field> fields;
        std::vector<std::unique_ptr<Node>> blocks;

        const Node* block(std::string_view key) const;
    };

    TextLoad() = default;

    const Field* field(std::string_view name) const;
    static cinder::scene::PropRec rec(const Node& node);
    static const Node& emptyNode();
    static void body(Node& into, const std::vector<std::string>& lines, std::size_t& at);

    std::unique_ptr<Node> root_;
    std::vector<const Node*> stack_;
    std::vector<const Node*> arrays_;
};

}
