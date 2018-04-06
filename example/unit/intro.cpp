//[intro
#include <metal/unit.hpp>

int main(int argc, char *argv[])
{
    int i = 41;
    i++;

    METAL_ASSERT_EQUAL(i, 42);
    return METAL_REPORT();
}
//]
