#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace cinder::scene {

class PropValue;

using PropSeq = std::vector<PropValue>;
using PropRec = std::map<std::string, PropValue>;

class PropValue {
public:
    using Storage = std::variant<std::int64_t, double, std::string, bool, PropSeq, PropRec>;

    PropValue() : value_(std::int64_t{0}) {}
    explicit PropValue(Storage value) : value_(std::move(value)) {}

    static PropValue integer(std::int64_t value) { return PropValue(Storage(value)); }
    static PropValue number(double value) { return PropValue(Storage(value)); }
    static PropValue text(std::string value) { return PropValue(Storage(std::move(value))); }
    static PropValue flag(bool value) { return PropValue(Storage(value)); }
    static PropValue seq(PropSeq value) { return PropValue(Storage(std::move(value))); }
    static PropValue rec(PropRec value) { return PropValue(Storage(std::move(value))); }

    const Storage& value() const { return value_; }

    template <class T>
    bool is() const { return std::holds_alternative<T>(value_); }

    template <class T>
    const T& as() const { return std::get<T>(value_); }

private:
    Storage value_;
};

class PropBag {
public:
    virtual ~PropBag() = default;
    virtual PropRec readBag() const = 0;
    virtual void writeBag(const PropRec& values) = 0;
    virtual void patchBag(const PropRec& values) = 0;
};

}
