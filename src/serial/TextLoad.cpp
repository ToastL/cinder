#include "serial/TextLoad.hpp"

#include <cctype>
#include <cstdlib>

namespace cinder::serial {
namespace {

std::string trimmed(std::string_view line) {
    std::size_t begin = 0;
    std::size_t end = line.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(line[begin]))) begin++;
    while (end > begin && std::isspace(static_cast<unsigned char>(line[end - 1]))) end--;
    return std::string(line.substr(begin, end - begin));
}

std::vector<std::string> tokenize(const std::string& line, bool& quoted) {
    std::vector<std::string> out;
    std::size_t i = 0;

    while (i < line.size()) {
        const char c = line[i];
        if (std::isspace(static_cast<unsigned char>(c))) {
            i++;
            continue;
        }

        if (c != '"') {
            const std::size_t start = i;
            while (i < line.size() && !std::isspace(static_cast<unsigned char>(line[i]))) i++;
            out.push_back(line.substr(start, i - start));
            continue;
        }

        std::string value;
        i++;
        while (i < line.size() && line[i] != '"') {
            const char d = line[i++];
            if (d != '\\' || i >= line.size()) {
                value.push_back(d);
                continue;
            }
            const char e = line[i++];
            switch (e) {
                case 'n': value.push_back('\n'); break;
                case 'r': value.push_back('\r'); break;
                case 't': value.push_back('\t'); break;
                default:
                    if (e < '0' || e > '9' || i + 1 >= line.size()) {
                        value.push_back(e);
                    } else {
                        value.push_back(static_cast<char>(std::atoi(line.substr(i - 1, 3).c_str())));
                        i += 2;
                    }
            }
        }
        i++;
        if (out.size() == 1) quoted = true;
        out.push_back(value);
    }
    return out;
}

cinder::scene::PropValue scalar(const std::string& token, bool quoted) {
    using cinder::scene::PropValue;

    if (quoted) return PropValue::text(token);
    if (token == "true") return PropValue::flag(true);
    if (token == "false") return PropValue::flag(false);

    const bool real = token.find_first_of(".eE") != std::string::npos;
    try {
        if (real) return PropValue::number(std::stod(token));
        return PropValue::integer(std::stoll(token));
    } catch (const std::exception&) {
        return PropValue::text(token);
    }
}

cinder::scene::PropValue fieldValue(const std::vector<std::string>& tokens, bool quoted) {
    if (tokens.size() == 1) return scalar(tokens[0], quoted);

    cinder::scene::PropSeq values;
    values.reserve(tokens.size());
    for (const std::string& token : tokens) values.push_back(scalar(token, false));
    return cinder::scene::PropValue::seq(std::move(values));
}

}

const TextLoad::Node& TextLoad::emptyNode() {
    static const Node empty;
    return empty;
}

const TextLoad::Node* TextLoad::Node::block(std::string_view key) const {
    for (const std::unique_ptr<Node>& child : blocks) {
        if (child->name == key) return child.get();
    }
    return nullptr;
}

TextLoad TextLoad::parse(const std::string& source) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (true) {
        const std::size_t split = source.find('\n', start);
        if (split == std::string::npos) {
            lines.push_back(source.substr(start));
            break;
        }
        lines.push_back(source.substr(start, split - start));
        start = split + 1;
    }

    TextLoad archive;
    archive.root_ = std::make_unique<Node>();
    std::size_t at = 0;
    body(*archive.root_, lines, at);
    archive.stack_.push_back(archive.root_.get());
    return archive;
}

void TextLoad::body(Node& into, const std::vector<std::string>& lines, std::size_t& at) {
    while (at < lines.size()) {
        const std::string line = trimmed(lines[at]);
        at++;
        if (line.empty()) continue;
        if (line == "}") return;

        bool quoted = false;
        std::vector<std::string> tokens = tokenize(line, quoted);
        if (tokens.empty()) continue;

        if (tokens.back() == "{") {
            auto child = std::make_unique<Node>();
            child->name = tokens.front();
            body(*child, lines, at);
            into.blocks.push_back(std::move(child));
        } else {
            Field field;
            field.tokens.assign(tokens.begin() + 1, tokens.end());
            field.quoted = quoted;
            into.fields[tokens.front()] = std::move(field);
        }
    }
}

bool TextLoad::enterRecord(std::string_view name) {
    const Node* node = stack_.back()->block(name);
    if (node == nullptr) return false;
    stack_.push_back(node);
    return true;
}

void TextLoad::leaveRecord() { stack_.pop_back(); }

int TextLoad::enterArray(std::string_view name, int count) {
    const Node* node = stack_.back()->block(name);
    arrays_.push_back(node == nullptr ? &emptyNode() : node);
    return node == nullptr ? 0 : static_cast<int>(node->blocks.size());
}

void TextLoad::enterItem(int index, std::string_view type) {
    stack_.push_back(arrays_.back()->blocks[static_cast<std::size_t>(index)].get());
}

std::string TextLoad::itemType(int index) {
    return arrays_.back()->blocks[static_cast<std::size_t>(index)]->name;
}

void TextLoad::leaveItem() { stack_.pop_back(); }

void TextLoad::leaveArray() { arrays_.pop_back(); }

const TextLoad::Field* TextLoad::field(std::string_view name) const {
    auto found = stack_.back()->fields.find(std::string(name));
    if (found == stack_.back()->fields.end() || found->second.tokens.empty()) return nullptr;
    return &found->second;
}

int TextLoad::integer(std::string_view name, int value, int fallback) {
    const Field* found = field(name);
    return found == nullptr ? fallback : static_cast<int>(std::stod(found->tokens[0]));
}

bool TextLoad::flag(std::string_view name, bool value, bool fallback) {
    const Field* found = field(name);
    return found == nullptr ? fallback : found->tokens[0] == "true";
}

std::string TextLoad::text(std::string_view name, std::string_view value,
                           std::string_view fallback) {
    const Field* found = field(name);
    return found == nullptr ? std::string(fallback) : found->tokens[0];
}

void TextLoad::vector(std::string_view name, float* values, const float* fallback, int arity) {
    const Field* found = field(name);
    for (int i = 0; i < arity; ++i) {
        const bool missing = found == nullptr || static_cast<std::size_t>(i) >= found->tokens.size();
        values[i] = missing ? fallback[i] : std::stof(found->tokens[static_cast<std::size_t>(i)]);
    }
}

cinder::scene::PropRec TextLoad::bag(std::string_view name, const cinder::scene::PropRec& values) {
    const Node* node = stack_.back()->block(name);
    return node == nullptr ? cinder::scene::PropRec{} : rec(*node);
}

cinder::scene::PropRec TextLoad::rec(const Node& node) {
    cinder::scene::PropRec out;
    for (const auto& [key, field] : node.fields) {
        if (!field.tokens.empty()) out.emplace(key, fieldValue(field.tokens, field.quoted));
    }
    for (const std::unique_ptr<Node>& block : node.blocks) {
        out.insert_or_assign(block->name, cinder::scene::PropValue::rec(rec(*block)));
    }
    return out;
}

}
