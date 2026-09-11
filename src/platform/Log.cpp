#include "platform/Log.hpp"

#include <cstdarg>
#include <cstdio>
#include <utility>
#include <vector>

namespace cinder::platform {
namespace {

LogSink sink;

void emit(LogLevel level, const char* format, std::va_list args) {
    std::va_list measure;
    va_copy(measure, args);
    const int length = std::vsnprintf(nullptr, 0, format, measure);
    va_end(measure);

    if (length < 0) return;

    std::vector<char> text(static_cast<std::size_t>(length) + 1);
    std::vsnprintf(text.data(), text.size(), format, args);

    std::fputs(text.data(), level == LogLevel::Error ? stderr : stdout);
    if (!sink) return;

    std::size_t size = static_cast<std::size_t>(length);
    while (size > 0 && (text[size - 1] == '\n' || text[size - 1] == '\r')) --size;
    sink(level, std::string_view(text.data(), size));
}

}

void setLogSink(LogSink next) { sink = std::move(next); }

void logInfo(const char* format, ...) {
    std::va_list args;
    va_start(args, format);
    emit(LogLevel::Info, format, args);
    va_end(args);
}

void logError(const char* format, ...) {
    std::va_list args;
    va_start(args, format);
    emit(LogLevel::Error, format, args);
    va_end(args);
}

}
