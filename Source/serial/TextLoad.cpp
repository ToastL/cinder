#include "serial/TextLoad.hpp"

#include "platform/Log.hpp"
#include "platform/Strings.hpp"
#include "serial/NumberText.hpp"
#include "serial/TextSave.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace cinder::serial {
namespace {

struct Token {
    std::string text;
    bool quoted = false;
};

bool isDigit(char c) { return c >= '0' && c <= '9'; }

std::vector<Token> tokenize(const std::string& line) {
    std::vector<Token> out;
    std::size_t i = 0;

    while (i < line.size()) {
        const char c = line[i];
        if (cinder::platform::isBlank(c)) {
            i++;
            continue;
        }

        if (c != '"') {
            const std::size_t start = i;
            while (i < line.size() && !cinder::platform::isBlank(line[i])) i++;
            out.push_back(Token{line.substr(start, i - start), false});
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
                    if (!isDigit(e) || i + 1 >= line.size() || !isDigit(line[i]) || !isDigit(line[i + 1])) {
                        value.push_back(e);
                    } else {
                        const int code = (e - '0') * 100 + (line[i] - '0') * 10 + (line[i + 1] - '0');
                        value.push_back(static_cast<char>(code));
                        i += 2;
                    }
            }
        }
        i++;
        out.push_back(Token{std::move(value), true});
    }
    return out;
}

cinder::scene::PropValue scalar(const std::string& token, bool quoted) {
    using cinder::scene::PropValue;

    if (quoted) return PropValue::text(token);
    if (token == "true") return PropValue::flag(true);
    if (token == "false") return PropValue::flag(false);

    if (token.find_first_of(".eE") != std::string::npos) {
        if (const std::optional<double> real = parseReal(token)) return PropValue::number(*real);
    } else if (const std::optional<std::int64_t> whole = parseInteger(token)) {
        return PropValue::integer(*whole);
    }
    return PropValue::text(token);
}

cinder::scene::PropValue fieldValue(const std::vector<std::string>& tokens, const std::vector<bool>& quoted) {
    if (tokens.size() == 1) return scalar(tokens[0], quoted[0]);

    cinder::scene::PropSeq values;
    values.reserve(tokens.size());
    for (std::size_t i = 0; i < tokens.size(); ++i) values.push_back(scalar(tokens[i], quoted[i]));
    return cinder::scene::PropValue::seq(std::move(values));
}

std::optional<int> wholeNumber(const std::string& token) {
    if (const std::optional<std::int64_t> whole = parseInteger(token)) {
        if (*whole >= std::numeric_limits<int>::min() && *whole <= std::numeric_limits<int>::max()) {
            return static_cast<int>(*whole);
        }
        return std::nullopt;
    }
    const std::optional<double> real = parseReal(token);
    if (!real || !std::isfinite(*real) || *real < static_cast<double>(std::numeric_limits<int>::min())
        || *real > static_cast<double>(std::numeric_limits<int>::max())) {
        return std::nullopt;
    }
    return static_cast<int>(*real);
}

[[noreturn]] void malformed(std::size_t line, const char* what) {
    throw std::runtime_error("line " + std::to_string(line) + ": " + what);
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
    body(*archive.root_, lines, at, false);
    archive.stack_.push_back(archive.root_.get());
    return archive;
}

bool TextLoad::body(Node& into, const std::vector<std::string>& lines, std::size_t& at, bool nested) {
    while (at < lines.size()) {
        const std::string line(cinder::platform::trimmed(lines[at]));
        at++;
        if (line.empty()) continue;
        if (line == "}") {
            if (!nested) malformed(at, "unexpected '}'");
            return true;
        }

        std::vector<Token> tokens = tokenize(line);
        if (tokens.empty()) continue;

        if (tokens.back().text == "{" && !tokens.back().quoted) {
            auto child = std::make_unique<Node>();
            child->name = tokens.front().text;
            const std::size_t opened = at;
            if (!body(*child, lines, at, true)) malformed(opened, "block is never closed");
            into.blocks.push_back(std::move(child));
        } else {
            Field field;
            for (std::size_t k = 1; k < tokens.size(); ++k) {
                field.tokens.push_back(std::move(tokens[k].text));
                field.quoted.push_back(tokens[k].quoted);
            }
            const std::string& key = tokens.front().text;
            if (into.fields.count(key) == 0) into.order.push_back(key);
            into.fields[key] = std::move(field);
        }
    }
    return false;
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
    if (found == nullptr) return fallback;
    if (const std::optional<int> whole = wholeNumber(found->tokens[0])) return *whole;
    cinder::platform::logError("[serial] '%s' is not an integer: %s\n", std::string(name).c_str(),
                               found->tokens[0].c_str());
    return fallback;
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
        if (missing) {
            values[i] = fallback[i];
            continue;
        }
        const std::string& token = found->tokens[static_cast<std::size_t>(i)];
        if (const std::optional<float> parsed = parseFloat(token)) {
            values[i] = *parsed;
        } else {
            cinder::platform::logError("[serial] '%s' is not a number: %s\n", std::string(name).c_str(), token.c_str());
            values[i] = fallback[i];
        }
    }
}

cinder::scene::PropRec TextLoad::bag(std::string_view name, const cinder::scene::PropRec& values) {
    const Node* node = stack_.back()->block(name);
    return node == nullptr ? cinder::scene::PropRec{} : rec(*node);
}

cinder::scene::RawFields TextLoad::raw(const cinder::scene::RawFields& values) {
    cinder::scene::RawFields out;
    const Node& node = *stack_.back();
    for (const std::string& key : node.order) {
        const Field& found = node.fields.at(key);
        std::string text;
        for (std::size_t i = 0; i < found.tokens.size(); ++i) {
            if (i > 0) text.push_back(' ');
            text += found.quoted[i] ? TextSave::quoted(found.tokens[i]) : found.tokens[i];
        }
        out.emplace_back(key, std::move(text));
    }
    return out;
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
