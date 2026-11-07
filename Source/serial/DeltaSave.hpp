#pragma once

#include "serial/Archive.hpp"

#include <string>

namespace cinder::serial {

class DeltaSave : public Archive {
public:
    const std::string& text() const { return out_; }

    bool loading() const override { return false; }

    int integer(std::string_view name, int value, int fallback) override;
    bool flag(std::string_view name, bool value, bool fallback) override;
    void vector(std::string_view name, float* values, const float* fallback, int arity) override;

protected:
    virtual void field(std::string_view name, std::string_view value) = 0;

    std::string out_;
};

}
