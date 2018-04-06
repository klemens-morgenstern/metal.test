/**
 * @file   /test/junit/json/cpp_test.cpp
 * @date   05.10.2016
 * @author Klemens D. Morgenstern
 *

 */

#include <array>
#include <metal/unit.hpp>

void equal()
{
    unsigned int i = -42;
    unsigned char j = -42;
    char k = -1;
    unsigned char l = 0xFF;

    METAL_ASSERT_EQUAL(i, j);
    METAL_EXPECT_EQUAL(l, k);
    METAL_ASSERT_EQUAL_BITWISE(l, k);
    METAL_EXPECT_EQUAL_BITWISE(i, j);

    std::array<int,   3> arr1 = {-1,0,1};
    std::array<short, 4> arr2 = {-1,0,1,2};

    METAL_ASSERT_EQUAL_RANGED(arr1.begin(), arr1.end(), arr2.begin(), arr2.end());
    METAL_EXPECT_EQUAL_RANGED(arr1.begin(), arr1.end(), arr2.begin(), arr2.end() - 1);

    arr1[0] = -2;

    METAL_ASSERT_EQUAL_BITWISE_RANGED(arr1.begin(), arr1.end(), arr2.begin(), arr2.end() - 1);
    METAL_EXPECT_EQUAL_BITWISE_RANGED(arr1.begin(), arr1.end(), arr2.begin(), arr2.end() - 1);
}

void no_throw()
{

}

int throw1()
{
    throw 42;
    return 42;
}

int throw2()
{
    throw 2.3;
    return 42;
}

void throw_test()
{

    METAL_ASSERT_NO_THROW(no_throw());
    METAL_EXPECT_NO_THROW(throw1());

    METAL_ASSERT_ANY_THROW(no_throw());
    METAL_EXPECT_ANY_THROW(throw2());

    METAL_ASSERT_THROW(METAL_ASSERT(throw1());, int, double);
    METAL_ASSERT_THROW(METAL_ASSERT(throw2());, int, double);
    METAL_CRITICAL(METAL_EXPECT_THROW(throw2(), void*));
}

void close()
{

    METAL_ASSERT_CLOSE(1., .9, .1);
    METAL_EXPECT_CLOSE(1., .9, .09);

    METAL_ASSERT_CLOSE_PERCENT(100, 90, 9);
    METAL_EXPECT_CLOSE_PERCENT(100, 90, 10);

    METAL_ASSERT_CLOSE_RELATIVE(2., 1.8, 0.1);
    METAL_EXPECT_CLOSE_RELATIVE(2., 1.5, 0.25);

    std::array<int, 3> a1 = {1,2,3};
    std::array<double, 3> a2 = {1.1, 1.8, 2.7};

    METAL_ASSERT_CLOSE_RANGED(a1.begin(), a1.end(), a2.begin(), a2.end(), 0.3);
    METAL_EXPECT_CLOSE_RANGED(a1.begin(), a1.end(), a2.begin(), a2.end(), 0.2);

    METAL_ASSERT_CLOSE_RELATIVE_RANGED(a1.begin(), a1.end(), a2.begin(), a2.end(), 0.1);
    METAL_EXPECT_CLOSE_RELATIVE_RANGED(a1.begin(), a1.end(), a2.begin(), a2.end(), 0.05);

    METAL_ASSERT_CLOSE_PERCENT_RANGED(a1.begin(), a1.end(), a2.begin(), a2.end(),  5);
    METAL_EXPECT_CLOSE_PERCENT_RANGED(a1.begin(), a1.end(), a2.begin(), a2.end(), 10);
}

void compare()
{
    METAL_ASSERT_LESSER(1, 2);
    METAL_EXPECT_LESSER(1, 1);

    METAL_ASSERT_GREATER(1, 1);
    METAL_EXPECT_GREATER(2, 1);

    std::array<int, 3> a1 = {1,2,3};


    METAL_ASSERT_GREATER_RANGED(a1.begin(), a1.end(), a1.rbegin(), a1.rend());
    METAL_EXPECT_GREATER_RANGED(a1.begin(), a1.end(), a1.begin(), a1.end());

    METAL_ASSERT_LESSER_RANGED(a1.begin(), a1.end(), a1.rbegin(), a1.rend());
    METAL_EXPECT_LESSER_RANGED(a1.begin(), a1.end(), a1.begin(), a1.end());

}

void ge()
{
    METAL_ASSERT_GE(1, 2);
    METAL_EXPECT_GE(1, 1);

    METAL_ASSERT_GE_BITWISE(0b101, 0b100);
    METAL_EXPECT_GE_BITWISE(0b101, 0b010);

    std::array<int, 3> a1 = {0b01,0b10,0b11};
    std::array<int, 3> a2 = {0b01,0b01,0b10};

    METAL_ASSERT_GE_RANGED(a1.begin(), a1.end(), a2.begin(), a2.end());
    METAL_EXPECT_GE_RANGED(a2.begin(), a2.end(), a1.begin(), a1.end());

    METAL_ASSERT_GE_BITWISE_RANGED(a1.begin(), a1.end(), a2.begin(), a2.end());
    METAL_EXPECT_GE_BITWISE_RANGED(a2.begin(), a2.end(), a1.begin(), a1.end());

}

void le()
{
    METAL_ASSERT_LE(1, 0);
    METAL_EXPECT_LE(1, 1);

    METAL_ASSERT_LE_BITWISE(0b101, 0b100);
    METAL_EXPECT_LE_BITWISE(0b101, 0b010);

    std::array<int, 3> a1 = {0b01,0b10,0b11};
    std::array<int, 3> a2 = {0b01,0b01,0b10};

    METAL_ASSERT_LE_RANGED(a1.begin(), a1.end(), a2.begin(), a2.end());
    METAL_EXPECT_LE_RANGED(a2.begin(), a2.end(), a1.begin(), a1.end());

    METAL_ASSERT_LE_BITWISE_RANGED(a1.begin(), a1.end(), a2.begin(), a2.end());
    METAL_EXPECT_LE_BITWISE_RANGED(a2.begin(), a2.end(), a1.begin(), a1.end());

    METAL_LOG("my log message");
    METAL_CHECKPOINT();
    METAL_ASSERT_MESSAGE(false, "Some message");
    METAL_EXPECT_MESSAGE(true,  "some other message");
    METAL_ASSERT_MESSAGE(true,  "Yet another message");
    METAL_EXPECT_MESSAGE(false, "and another one");

}

void not_equal()
{
    unsigned int i = -42;
    unsigned char j = -42;
    char k = -1;
    unsigned char l = 0xFF;

    METAL_ASSERT_NOT_EQUAL(i, j);
    METAL_EXPECT_NOT_EQUAL(l, k);
    METAL_ASSERT_NOT_EQUAL_BITWISE(l, k);
    METAL_EXPECT_NOT_EQUAL_BITWISE(i, j);

    std::array<int,   3> arr1 = {-1,0,1};
    std::array<short, 4> arr2 = {-1,0,1,2};

    METAL_ASSERT_NOT_EQUAL_RANGED(arr1.begin(), arr1.end(), arr2.begin(), arr2.end());
    METAL_EXPECT_NOT_EQUAL_RANGED(arr1.begin(), arr1.end(), arr2.begin(), arr2.end() - 1);

    arr1[0] = -2;

    METAL_ASSERT_NOT_EQUAL_BITWISE_RANGED(arr1.begin(), arr1.end(), arr2.begin(), arr2.end() - 1);
    METAL_EXPECT_NOT_EQUAL_BITWISE_RANGED(arr1.begin(), arr1.end(), arr2.begin(), arr2.end() - 1);
}

bool my_predicate(int i, int j) {return i == j;}

void predicate()
{
    METAL_ASSERT_PREDICATE(my_predicate, 4, 2);
    METAL_EXPECT_PREDICATE(my_predicate, 3, 3);
    METAL_ASSERT_PREDICATE(my_predicate, 1, 1);
    METAL_EXPECT_PREDICATE(my_predicate, 5, 2);
}

int main(int argc, char * argv[])
{
    //so we have a few free tests.
    METAL_ASSERT_EXECUTE();
    METAL_EXPECT_EXECUTE();

    METAL_CALL(&equal, "equal test");
    METAL_CALL(&throw_test, "exception test");

    METAL_ASSERT_NO_EXECUTE();
    METAL_EXPECT_NO_EXECUTE();

    METAL_CALL(&close, "close");
    METAL_CALL(&compare, "compare");
    METAL_CALL(&ge, "ge");
    METAL_CALL(&le, "le");
    METAL_CALL(&not_equal, "not_equal");
    METAL_CALL(&predicate, "predicate");

    return METAL_REPORT();
}
