
#include <array>

#include <metal/unit.hpp>

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

int main(int argc, char * argv[])
{

    METAL_ASSERT_NO_THROW(no_throw());
    METAL_EXPECT_NO_THROW(throw1());

    METAL_ASSERT_ANY_THROW(no_throw());
    METAL_EXPECT_ANY_THROW(throw2());

    METAL_ASSERT_THROW(METAL_ASSERT(throw1());, int, double);
    METAL_ASSERT_THROW(METAL_ASSERT(throw2());, int, double);
    METAL_EXPECT_THROW(throw2(), void*);

    return METAL_REPORT();
}
