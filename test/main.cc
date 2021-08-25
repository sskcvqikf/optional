#include <iostream>
#include <assert.h>
#include <string>

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

#define TYPE_GENERATOR(name, cc, mc, ca, ma, d)         \
    struct name                                         \
    {                                                   \
        name(const name&) cc;                           \
        name(name&&) mc;                                \
        name& operator=(const name&) ca;                \
        name& operator=(name&&) ma;                     \
        ~name() d;                                      \
    };

TEST(testAssigment)
{
    using namespace pd;
    int b = 54;
    optional<int> t1 {5};
    optional<int> t2 {b};
    optional<int> t3;
 
    ASSERT(t1 == 5, "t1 should be equal to 5");
    ASSERT(t2 == 54, "t2 should be equal to 54");
    ASSERT(t3 == nullopt, "t3 should be equal to nullopt");
    ASSERT(!t3, "!t3 should be false");

    t1 = t2;
    t3 = 53;

    ASSERT(t1 == 54, "t1 now should be equal to 54");
    ASSERT(*t3 == 53, "t3 now should be equal to 53");
    ASSERT(t1 == t2, "t1 now should be equal to t2");

    optional<short> t4 {40};
    t2 = 40;

    ASSERT(t4 == t2, "t4 should be equal to t2");
    ASSERT(*t4 == *t2, "*t4 should be equal to *t2");
    ASSERT(t2 != t1, "t2 shouldnt be equal to t1");
}

TEST(testTriviality)
{
    using namespace pd;
    // int is obviously must be trivial
    ASSERT(std::is_trivially_copy_constructible_v<optional<int>>, "optional<int> must be trivially copy constructible");
    ASSERT(std::is_trivially_copy_assignable_v<optional<int>>, "optional<int> must be trivially copy assignable");
    ASSERT(std::is_trivially_move_constructible_v<optional<int>>, "optional<int> must be trivially move constructible");
    ASSERT(std::is_trivially_move_assignable_v<optional<int>>, "optional<int> must be trivially move assignable");
    ASSERT(std::is_trivially_destructible_v<optional<int>>, "optional<int> must be trivially destructible");

    // string is not trivial
    using std::string;
    ASSERT(!std::is_trivially_copy_constructible_v<optional<string>>, "optional<string> must not be trivially copy consstructible");
    ASSERT(!std::is_trivially_copy_assignable_v<optional<string>>, "optional<string> must not be trivially copy assignable");
    ASSERT(!std::is_trivially_move_constructible_v<optional<string>>, "optional<string> must not be trivially move constructible");
    ASSERT(!std::is_trivially_move_assignable_v<optional<string>>, "optional<string> must not be trivially move assignable");
    ASSERT(!std::is_trivially_destructible_v<optional<string>>, "optional<string> must not be trivially destructible");

    TYPE_GENERATOR(trivial, =default, =default, =default, =default, =default);

    ASSERT(std::is_trivially_copy_constructible_v<optional<trivial>>, "optional<trivial> must be trivially copy consstructible");
    ASSERT(std::is_trivially_copy_assignable_v<optional<trivial>>, "optional<trivial> must be trivially copy assignable");
    ASSERT(std::is_trivially_move_constructible_v<optional<trivial>>, "optional<trivial> must be trivially move constructible");
    ASSERT(std::is_trivially_move_assignable_v<optional<trivial>>, "optional<trivial> must be trivially move assignable");
    ASSERT(std::is_trivially_destructible_v<optional<trivial>>, "optional<trivial> must be trivially destructible");

    TYPE_GENERATOR(nontrivial, {}, {}, {return *this;}, {return *this;}, {});

    ASSERT(!std::is_trivially_copy_constructible_v<optional<nontrivial>>, "optional<nontrivial> must not be nontrivially copy consstructible");
    ASSERT(!std::is_trivially_copy_assignable_v<optional<nontrivial>>, "optional<nontrivial> must not be nontrivially copy assignable");
    ASSERT(!std::is_trivially_move_constructible_v<optional<nontrivial>>, "optional<nontrivial> must not be nontrivially move constructible");
    ASSERT(!std::is_trivially_move_assignable_v<optional<nontrivial>>, "optional<nontrivial> must not be nontrivially move assignable");
    ASSERT(!std::is_trivially_destructible_v<optional<nontrivial>>, "optional<nontrivial> must not be nontrivially destructible");
}

TEST(testTypeProperties)
{
    using namespace pd;

    ASSERT(std::is_copy_constructible_v<optional<int>>, "optional<int> must be copy constructible");
    ASSERT(std::is_copy_assignable_v<optional<int>>, "optional<int> must be copy assignable");
    ASSERT(std::is_move_constructible_v<optional<int>>, "optional<int> must be move constructible");
    ASSERT(std::is_move_assignable_v<optional<int>>, "optional<int> must be move assignable");
    ASSERT(std::is_destructible_v<optional<int>>, "optional<int> must be destructible");
    
    using std::string;
    ASSERT(std::is_copy_constructible_v<optional<string>>, "optional<string> must be copy constructible");
    ASSERT(std::is_copy_assignable_v<optional<string>>, "optional<string> must be copy assignable");
    ASSERT(std::is_move_constructible_v<optional<string>>, "optional<string> must be move constructible");
    ASSERT(std::is_move_assignable_v<optional<string>>, "optional<string> must be move assignable");
    ASSERT(std::is_destructible_v<optional<string>>, "optional<string> must be destructible");

    TYPE_GENERATOR(defaultType, =default, =default, =default, =default, =default);
    ASSERT(std::is_copy_constructible_v<optional<defaultType>>, "optional<defaultType> must be copy constructible");
    ASSERT(std::is_copy_assignable_v<optional<defaultType>>, "optional<defaultType> must be copy assignable");
    ASSERT(std::is_move_constructible_v<optional<defaultType>>, "optional<defaultType> must be move constructible");
    ASSERT(std::is_move_assignable_v<optional<defaultType>>, "optional<defaultType> must be move assignable");
    ASSERT(std::is_destructible_v<optional<defaultType>>, "optional<defaultType> must be destructible");

    TYPE_GENERATOR(onlyDestructorType, =delete, =delete, =delete, =delete, =default);
    ASSERT(!std::is_copy_constructible_v<optional<onlyDestructorType>>, "optional<onlyDestructorType> must not be copy constructible");
    ASSERT(!std::is_copy_assignable_v<optional<onlyDestructorType>>, "optional<onlyDestructorType> must not be copy assignable");
    ASSERT(!std::is_move_constructible_v<optional<onlyDestructorType>>, "optional<onlyDestructorType> must not be move constructible");
    ASSERT(!std::is_move_assignable_v<optional<onlyDestructorType>>, "optional<onlyDestructorType> must not be move assignable");
    ASSERT(std::is_destructible_v<optional<onlyDestructorType>>, "optional<onlyDestructorType> must be destructible");

    TYPE_GENERATOR(nonDefaultType, {}, {}, {return *this;}, {return *this;}, {});
    ASSERT(std::is_copy_constructible_v<optional<nonDefaultType>>, "optional<nonDefaultType> must be copy constructible");
    ASSERT(std::is_copy_assignable_v<optional<nonDefaultType>>, "optional<nonDefaultType> must be copy assignable");
    ASSERT(std::is_move_constructible_v<optional<nonDefaultType>>, "optional<nonDefaultType> must be move constructible");
    ASSERT(std::is_move_assignable_v<optional<nonDefaultType>>, "optional<nonDefaultType> must be move assignable");
    ASSERT(std::is_destructible_v<optional<nonDefaultType>>, "optional<nonDefaultType> must be destructible");

    TYPE_GENERATOR(randomType, =delete, =default, =delete, =default, =default);
    ASSERT(!std::is_copy_constructible_v<optional<randomType>>, "optional<randomType> must be copy constructible");
    ASSERT(!std::is_copy_assignable_v<optional<randomType>>, "optional<randomType> must be copy assignable");
    ASSERT(std::is_move_constructible_v<optional<randomType>>, "optional<randomType> must be move constructible");
    ASSERT(std::is_move_assignable_v<optional<randomType>>, "optional<randomType> must be move assignable");
    ASSERT(std::is_destructible_v<optional<randomType>>, "optional<randomType> must be destructible");

}

int main()
{
    testAssigment();
    testTriviality();
    testTypeProperties();

    if (is_failed)
        exit(1);
    return 0;
}
