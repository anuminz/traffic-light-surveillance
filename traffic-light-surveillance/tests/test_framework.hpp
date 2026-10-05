#pragma once
// Minimal dependency-free unit test framework.
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace tf {
struct Case { std::string name; std::function<void()> fn; };
struct Failure { std::string msg; };
inline std::vector<Case>& registry() { static std::vector<Case> r; return r; }
struct Registrar {
    Registrar(const char* n, std::function<void()> f) { registry().push_back({n, std::move(f)}); }
};
}  // namespace tf

#define TEST(name) \
    static void name(); \
    static tf::Registrar reg_##name(#name, name); \
    static void name()

#define CHECK(cond) \
    do { if (!(cond)) throw tf::Failure{std::string(__FILE__) + ":" + std::to_string(__LINE__) + \
         ": CHECK(" #cond ") failed"}; } while (0)
#define CHECK_EQ(a, b) CHECK((a) == (b))
