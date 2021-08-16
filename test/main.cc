#include <iostream>
#include <assert.h>

#include "../include/pd/optional.hh"

#ifndef NDEBUG
#   define ASSERT(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            std::terminate(); \
        } \
    } while (false)
#else
#   define ASSERT(condition, message) do { } while (false)
#endif

void test_assigment()
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
    test_assigment();
    return 0;
}
