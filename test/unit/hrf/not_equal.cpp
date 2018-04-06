
#include <array>

#include <metal/unit.hpp>

int main(int argc, char * argv[])
{
    unsigned int i = -42;
    unsigned char j = -42;
    signed char k = -1;
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


    return METAL_REPORT();
}
