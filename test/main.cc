#include <iostream>
#include <assert.h>

#include "../include/pd/optional.hh"

void* print_testname(const char* name)
{
    std::cout << " ----- RUNNING " << name
        << " ----- \n";
    return nullptr;
}

bool is_failed = false;

#define TEST(name)                                      \
    void name(void* = print_testname(#name))            \

#define ASSERT(expr, message)                           \
    std::cout << "RUNNING: " << #expr;                  \
    if (!(expr)){ is_failed = true;                     \
        std::cerr << " - FAILED.\n" << __FILE__ << ":"  \
            << __LINE__ << " " << message << '\n';}     \
    else std::cout << " - PASSED.\n";

#define ASSERT_THROW(expr, exc, message)                \
    std::cout << "RUNNING: " << #expr;                  \
    try {                                               \
        expr;                                           \
        is_failed = true;                               \
        std::cerr << " - FAILED." << " No exception.\n" \
        << __FILE__ << ":" << __LINE__ << " "           \
        << message << '\n';                             \
    } catch (exc&) {std::cout << " - PASSED.\n";}       \
      catch (...) { is_failed = true;                   \
          std::cerr << " - FAILED."                     \
          << " Other exception.\n" << __FILE__ << ":"   \
          << __LINE__ << " " << message << '\n';};      \

TEST(testAssigment)
{
    using namespace pd;
    optional<int> t1 = 5;
    optional<int> t2 = -124;
    optional<int> t3;
 
    ASSERT(t1 == 5, "t1 should be equal to 5");
    ASSERT(t2 == -124, "t2 should be equal to -124");
    ASSERT(t3 == nullopt, "t3 should be equal to nullopt");

    t1 = t2;
    t3 = 53;

    ASSERT(t1 == -124, "t1 now should be equal to -124");
    ASSERT(t3 == 53, "t3 now should be equal to 53");
    ASSERT(t1 == t2, "t1 now should be equal to t2");

}

int main()
{
    testAssigment();
    return 0;
}
