#include "serial/Json.hpp"

#include <charconv>
#include <stdexcept>
#include <utility>

namespace cinder::serial {
namespace {

using cinder::scene::PropRec;
using cinder::scene::PropSeq;
using cinder::scene::PropValue;

constexpr int MAX_DEPTH = 64;

void appendUtf8(std::string& out, char32_t code) {
    if (code < 0x80) {
        out.push_back(static_cast<char>(code));
    } else if (code < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (code >> 6)));
        out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
    } else if (code < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (code >> 12)));
        out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (code >> 18)));
        out.push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
    }
}

class Parser {
public:
    explicit Parser(std::string_view source) : source_(source) {
        if (source_.starts_with("\xEF\xBB\xBF")) at_ = 3;
    }

    PropValue document() {
        PropValue value = parseValue(0);
        skipSpace();
        if (at_ != source_.size()) fail("unexpected text after the document");
        return value;
    }

private:
    [[noreturn]] void fail(const char* what) const {
        int line = 1;
        int column = 1;
        for (std::size_t i = 0; i < at_ && i < source_.size(); ++i) {
            if (source_[i] == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
        }
        throw std::runtime_error("json " + std::to_string(line) + ":" + std::to_string(column) + ": "
                                 + what);
    }

    void skipSpace() {
        while (at_ < source_.size()) {
            const char c = source_[at_];
            if (c != ' ' && c != '\t' && c != '\n' && c != '\r') return;
            at_++;
        }
    }

    char peek() {
        skipSpace();
        return at_ < source_.size() ? source_[at_] : '\0';
    }

    bool digit() const { return at_ < source_.size() && source_[at_] >= '0' && source_[at_] <= '9'; }

    void digits() {
        while (digit()) at_++;
    }

    bool literal(std::string_view word) {
        if (source_.substr(at_, word.size()) != word) return false;
        at_ += word.size();
        return true;
    }

    PropValue parseValue(int depth) {
        if (depth > MAX_DEPTH) fail("nested too deeply");

        const char c = peek();
        if (c == '{') return parseObject(depth);
        if (c == '[') return parseArray(depth);
        if (c == '"') return PropValue::text(parseString());
        if (c == '-' || (c >= '0' && c <= '9')) return parseNumber();
        if (literal("true")) return PropValue::flag(true);
        if (literal("false")) return PropValue::flag(false);
        if (source_.substr(at_, 4) == "null") fail("null is not supported");
        fail(at_ < source_.size() ? "unexpected character" : "unexpected end of input");
    }

    PropValue parseObject(int depth) {
        at_++;
        PropRec out;
        if (peek() == '}') {
            at_++;
            return PropValue::rec(std::move(out));
        }

        while (true) {
            if (peek() != '"') fail("expected a quoted key");
            std::string key = parseString();
            if (peek() != ':') fail("expected ':' after a key");
            at_++;
            PropValue value = parseValue(depth + 1);
            out.insert_or_assign(std::move(key), std::move(value));

            const char next = peek();
            if (next == '}') {
                at_++;
                return PropValue::rec(std::move(out));
            }
            if (next != ',') fail("expected ',' or '}'");
            at_++;
        }
    }

    PropValue parseArray(int depth) {
        at_++;
        PropSeq out;
        if (peek() == ']') {
            at_++;
            return PropValue::seq(std::move(out));
        }

        while (true) {
            out.push_back(parseValue(depth + 1));

            const char next = peek();
            if (next == ']') {
                at_++;
                return PropValue::seq(std::move(out));
            }
            if (next != ',') fail("expected ',' or ']'");
            at_++;
        }
    }

    std::string parseString() {
        at_++;
        std::string out;
        while (true) {
            if (at_ >= source_.size()) fail("unterminated string");
            const char c = source_[at_];
            if (c == '"') {
                at_++;
                return out;
            }
            if (static_cast<unsigned char>(c) < 0x20) fail("control character in a string");
            at_++;
            if (c != '\\') {
                out.push_back(c);
                continue;
            }

            if (at_ >= source_.size()) fail("unterminated string");
            const char escape = source_[at_];
            switch (escape) {
                case '"':
                case '\\':
                case '/': out.push_back(escape); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u':
                    at_++;
                    appendUtf8(out, codepoint());
                    continue;
                default: fail("unknown escape");
            }
            at_++;
        }
    }

    unsigned hex4() {
        unsigned value = 0;
        for (int i = 0; i < 4; ++i) {
            if (at_ >= source_.size()) fail("unterminated \\u escape");
            const char c = source_[at_];
            unsigned nibble = 0;
            if (c >= '0' && c <= '9') nibble = static_cast<unsigned>(c - '0');
            else if (c >= 'a' && c <= 'f') nibble = static_cast<unsigned>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') nibble = static_cast<unsigned>(c - 'A' + 10);
            else fail("bad \\u escape");
            value = value << 4 | nibble;
            at_++;
        }
        return value;
    }

    char32_t codepoint() {
        const unsigned high = hex4();
        if (high < 0xD800 || high > 0xDFFF) return high;
        if (high > 0xDBFF || source_.substr(at_, 2) != "\\u") fail("unpaired surrogate");
        at_ += 2;
        const unsigned low = hex4();
        if (low < 0xDC00 || low > 0xDFFF) fail("unpaired surrogate");
        return 0x10000 + ((high - 0xD800) << 10) + (low - 0xDC00);
    }

    PropValue parseNumber() {
        const std::size_t start = at_;
        if (source_[at_] == '-') at_++;
        if (!digit()) fail("expected a digit");
        if (source_[at_] == '0') at_++;
        else digits();

        bool real = false;
        if (at_ < source_.size() && source_[at_] == '.') {
            real = true;
            at_++;
            if (!digit()) fail("expected a digit after '.'");
            digits();
        }
        if (at_ < source_.size() && (source_[at_] == 'e' || source_[at_] == 'E')) {
            real = true;
            at_++;
            if (at_ < source_.size() && (source_[at_] == '+' || source_[at_] == '-')) at_++;
            if (!digit()) fail("expected a digit in the exponent");
            digits();
        }

        const char* begin = source_.data() + start;
        const char* stop = source_.data() + at_;
        if (real) {
            double value = 0;
            auto [last, error] = std::from_chars(begin, stop, value);
            if (error != std::errc{} || last != stop) fail("number out of range");
            return PropValue::number(value);
        }

        std::int64_t value = 0;
        auto [last, error] = std::from_chars(begin, stop, value);
        if (error != std::errc{} || last != stop) fail("number out of range");
        return PropValue::integer(value);
    }

    std::string_view source_;
    std::size_t at_ = 0;
};

}

cinder::scene::PropValue parseJson(std::string_view source) { return Parser(source).document(); }

void JsonWriter::beginObject(std::string_view key) {
    next(key);
    out_.push_back('{');
    stack_.push_back(Level{false, true});
}

void JsonWriter::beginArray(std::string_view key) {
    next(key);
    out_.push_back('[');
    stack_.push_back(Level{true, true});
}

void JsonWriter::end() {
    const Level level = stack_.back();
    stack_.pop_back();
    if (!level.empty) {
        out_.push_back('\n');
        out_.append(stack_.size(), '\t');
    }
    out_.push_back(level.array ? ']' : '}');
    if (stack_.empty()) out_.push_back('\n');
}

void JsonWriter::text(std::string_view key, std::string_view value) {
    next(key);
    quote(value);
}

void JsonWriter::integer(std::string_view key, std::int64_t value) {
    next(key);
    char buffer[24];
    auto [stop, error] = std::to_chars(buffer, buffer + sizeof(buffer), value);
    out_.append(buffer, stop);
}

void JsonWriter::flag(std::string_view key, bool value) {
    next(key);
    out_ += value ? "true" : "false";
}

void JsonWriter::item(std::string_view value) { text({}, value); }

void JsonWriter::next(std::string_view key) {
    if (stack_.empty()) return;

    Level& level = stack_.back();
    if (!level.empty) out_.push_back(',');
    level.empty = false;
    out_.push_back('\n');
    out_.append(stack_.size(), '\t');
    if (!level.array) {
        quote(key);
        out_ += ": ";
    }
}

void JsonWriter::quote(std::string_view value) {
    static constexpr char HEX[] = "0123456789abcdef";

    out_.push_back('"');
    for (char c : value) {
        switch (c) {
            case '"': out_ += "\\\""; break;
            case '\\': out_ += "\\\\"; break;
            case '\b': out_ += "\\b"; break;
            case '\f': out_ += "\\f"; break;
            case '\n': out_ += "\\n"; break;
            case '\r': out_ += "\\r"; break;
            case '\t': out_ += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    out_ += "\\u00";
                    out_.push_back(HEX[(c >> 4) & 0xF]);
                    out_.push_back(HEX[c & 0xF]);
                } else {
                    out_.push_back(c);
                }
        }
    }
    out_.push_back('"');
}

}
