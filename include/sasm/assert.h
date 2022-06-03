#pragma once

#include <exception>
#include <sstream>

#undef assert
#define assert(condition)           \
    sasm::assert_f(condition,       \
                   #condition,      \
                   __FUNCTION__,    \
                   __FILE__,        \
                   __LINE__)

namespace sasm {

struct assertion_exception : public std::exception {
    std::string message;
    explicit assertion_exception(const char* msg,
                                 const char* func,
                                 const char* file,
                                 int line)
    {
        try {
            std::ostringstream oss;
            oss << "Assertion failed: " << msg
                << ", function " << func
                << ", file " << file
                << ", line " << line;
            message = oss.str();
        } catch (...) {
            message.clear();
        }
    }

    virtual const char* what() const noexcept override {
        if (message.empty()) return "assertion_exception";
        return message.c_str();
    }
};

static void assert_f(bool condition,
              const char *msg,
              const char *func,
              const char *file,
              int line) {
    if (!condition) {
        throw assertion_exception(msg, func, file, line);
    }
}

}
