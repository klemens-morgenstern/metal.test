#include <metal/unit.hpp>

int main(int argc, char *argv[])
{
    int x = 3;
    int y = 3;
    int i = 42;
    int j = -1;
//[equal
    METAL_ASSERT_EQUAL(x, y);
    METAL_EXPECT_EQUAL(i, j);
//]
//[static_equal
    METAL_STATIC_ASSERT_EQUAL(sizeof(int), 4u);
//]
#if defined (__cplusplus)
//[ranged_equal_cpp
    std::array<int,   3> arr1 = {-1,0,1};
    std::array<short, 4> arr2 = {-1,0,1,2};

    METAL_ASSERT_EQUAL_RANGED(arr1.begin(), arr1.end(), arr2.begin(), arr2.end());
//]
#else
//[ranged_equal_c
   int arr1[3] = {-1,0,1};
   int arr1[4] = {-1,0,1};
   METAL_EXPECT_EQUAL_RANGED(arr1, 3, arr2, 4);
//]
#endif
   char c = 0xFF;
   unsigned char u = 255;
//[bitwise_equal
   METAL_ASSERT_EQUAL_BITWISE(c, u);
//]
   return METAL_REPORT();
}

