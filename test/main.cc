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

#ifdef TEST_VERBOSE

#define ASSERT(expr, message)                           \
    std::cout << "RUNNING: " << #expr;                  \
    if (!(expr)){ is_failed = true;                     \
        std::cerr << " - FAILED.\n" << __FILE__ << ":"  \
            << __LINE__ << " " << message << '\n';}     \
    else std::cout << " - PASSED.\n";

#define REQUIRE(expr)                                   \
    std::cout << "RUNNING: " << #expr;                  \
    if (!(expr)){ is_failed = true;                     \
        std::cerr << " - FAILED." << __FILE__ << ":"    \
            << __LINE__ << '\n';}                       \
    else std::cout << " - PASSED.\n";

#else
#define ASSERT(expr, message)                           \
    if (!(expr)){ is_failed = true;                     \
        std::cerr << "RUNNING: " << #expr               \
            << " - FAILED.\n" << __FILE__ << ":"        \
            << __LINE__ << " " << message << '\n';}     \

#define REQUIRE(expr)                                   \
    if (!(expr)){ is_failed = true;                     \
        std::cerr << "RUNNING: " << #expr               \
            << " - FAILED." << __FILE__ << ":"          \
            << __LINE__ << '\n';}                       \

#endif // TEST_VERBOSE

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
    REQUIRE(std::is_trivially_copy_constructible_v<optional<int>>);
    REQUIRE(std::is_trivially_copy_assignable_v<optional<int>>);
    REQUIRE(std::is_trivially_move_constructible_v<optional<int>>);
    REQUIRE(std::is_trivially_move_assignable_v<optional<int>>);
    REQUIRE(std::is_trivially_destructible_v<optional<int>>);

    // string is not trivial
    using std::string;
    REQUIRE(!std::is_trivially_copy_constructible_v<optional<string>>);
    REQUIRE(!std::is_trivially_copy_assignable_v<optional<string>>);
    REQUIRE(!std::is_trivially_move_constructible_v<optional<string>>);
    REQUIRE(!std::is_trivially_move_assignable_v<optional<string>>);
    REQUIRE(!std::is_trivially_destructible_v<optional<string>>);

    TYPE_GENERATOR(trivial, =default, =default, =default, =default, =default);

    REQUIRE(std::is_trivially_copy_constructible_v<optional<trivial>>);
    REQUIRE(std::is_trivially_copy_assignable_v<optional<trivial>>);
    REQUIRE(std::is_trivially_move_constructible_v<optional<trivial>>);
    REQUIRE(std::is_trivially_move_assignable_v<optional<trivial>>);
    REQUIRE(std::is_trivially_destructible_v<optional<trivial>>);

    TYPE_GENERATOR(nontrivial, {}, {}, {return *this;}, {return *this;}, {});

    REQUIRE(!std::is_trivially_copy_constructible_v<optional<nontrivial>>);
    REQUIRE(!std::is_trivially_copy_assignable_v<optional<nontrivial>>);
    REQUIRE(!std::is_trivially_move_constructible_v<optional<nontrivial>>);
    REQUIRE(!std::is_trivially_move_assignable_v<optional<nontrivial>>);
    REQUIRE(!std::is_trivially_destructible_v<optional<nontrivial>>);
}

TEST(testTypeProperties)
{
    using namespace pd;

    REQUIRE(std::is_copy_constructible_v<optional<int>>);
    REQUIRE(std::is_copy_assignable_v<optional<int>>);
    REQUIRE(std::is_move_constructible_v<optional<int>>);
    REQUIRE(std::is_move_assignable_v<optional<int>>);
    REQUIRE(std::is_destructible_v<optional<int>>);
    
    using std::string;
    REQUIRE(std::is_copy_constructible_v<optional<string>>);
    REQUIRE(std::is_copy_assignable_v<optional<string>>);
    REQUIRE(std::is_move_constructible_v<optional<string>>);
    REQUIRE(std::is_move_assignable_v<optional<string>>);
    REQUIRE(std::is_destructible_v<optional<string>>);

    TYPE_GENERATOR(defaultType, =default, =default, =default, =default, =default);
    REQUIRE(std::is_copy_constructible_v<optional<defaultType>>);
    REQUIRE(std::is_copy_assignable_v<optional<defaultType>>);
    REQUIRE(std::is_move_constructible_v<optional<defaultType>>);
    REQUIRE(std::is_move_assignable_v<optional<defaultType>>);
    REQUIRE(std::is_destructible_v<optional<defaultType>>);

    TYPE_GENERATOR(onlyDestructorType, =delete, =delete, =delete, =delete, =default);
    REQUIRE(!std::is_copy_constructible_v<optional<onlyDestructorType>>);
    REQUIRE(!std::is_copy_assignable_v<optional<onlyDestructorType>>);
    REQUIRE(!std::is_move_constructible_v<optional<onlyDestructorType>>);
    REQUIRE(!std::is_move_assignable_v<optional<onlyDestructorType>>);
    REQUIRE(std::is_destructible_v<optional<onlyDestructorType>>);

    TYPE_GENERATOR(nonDefaultType, {}, {}, {return *this;}, {return *this;}, {});
    REQUIRE(std::is_copy_constructible_v<optional<nonDefaultType>>);
    REQUIRE(std::is_copy_assignable_v<optional<nonDefaultType>>);
    REQUIRE(std::is_move_constructible_v<optional<nonDefaultType>>);
    REQUIRE(std::is_move_assignable_v<optional<nonDefaultType>>);
    REQUIRE(std::is_destructible_v<optional<nonDefaultType>>);

    TYPE_GENERATOR(randomType, =delete, =default, =delete, =default, =default);
    REQUIRE(!std::is_copy_constructible_v<optional<randomType>>);
    REQUIRE(!std::is_copy_assignable_v<optional<randomType>>);
    REQUIRE(std::is_move_constructible_v<optional<randomType>>);
    REQUIRE(std::is_move_assignable_v<optional<randomType>>);
    REQUIRE(std::is_destructible_v<optional<randomType>>);

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
