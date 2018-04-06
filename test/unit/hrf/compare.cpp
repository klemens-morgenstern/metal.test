
#include <array>

#include <metal/unit.hpp>

int main(int argc, char * argv[])
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


    return METAL_REPORT();
}
