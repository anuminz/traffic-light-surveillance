#include "test_framework.hpp"

int main() {
    int failed = 0;
    for (const auto& c : tf::registry()) {
        try {
            c.fn();
            std::cout << "[ PASS ] " << c.name << "\n";
        } catch (const tf::Failure& f) {
            ++failed;
            std::cout << "[ FAIL ] " << c.name << "\n         " << f.msg << "\n";
        } catch (const std::exception& e) {
            ++failed;
            std::cout << "[ FAIL ] " << c.name << "\n         exception: " << e.what() << "\n";
        }
    }
    std::cout << "\n" << (tf::registry().size() - failed) << "/" << tf::registry().size()
              << " tests passed\n";
    return failed == 0 ? 0 : 1;
}
