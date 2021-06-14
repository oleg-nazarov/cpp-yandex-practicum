#pragma once

#include <iostream>
#include <string>

using namespace std::string_literals;

template <typename A, typename B>
void AssertEqualImpl(const A a, const B b, const std::string& a_str, const std::string& b_str,
                     const std::string& file_name, const int line_number, const std::string& func_name,
                     const std::string& hint) {
    if (a != b) {
        std::cerr << std::boolalpha;
        std::cerr << file_name << "("s << line_number << "): "s << func_name << ": "s;
        std::cerr << "ASSERT_EQUAL("s << a_str << ", "s << b_str << ") failed: "s;
        std::cerr << a << " != "s << b << "."s;

        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint;
        }

        std::abort();
    }
}
#define ASSERT_EQUAL(a, b) AssertEqualImpl(a, b, #a, #b, __FILE__, __LINE__, __FUNCTION__, "")
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl(a, b, #a, #b, __FILE__, __LINE__, __FUNCTION__, hint)

void AssertImpl(const bool value, const std::string& value_str, const std::string& file_name,
                const int line_number, const std::string& func_name, const std::string& hint) {
    if (!value) {
        std::cerr << file_name << "("s << line_number << "): "s << func_name << ": "s;
        std::cerr << "ASSERT("s << value_str << ") failed."s;

        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint;
        }

        std::abort();
    }
}
#define ASSERT(value) AssertImpl(value, #value, __FILE__, __LINE__, __FUNCTION__, "")
#define ASSERT_HINT(value, hint) AssertImpl(value, #value, __FILE__, __LINE__, __FUNCTION__, hint)

#define RUN_TEST(func) \
    func();            \
    std::cerr << #func << ": OK." << std::endl;
