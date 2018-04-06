
#include <array>

#include <metal/unit.hpp>

int main(int argc, char * argv[])
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


    return METAL_REPORT();
}
