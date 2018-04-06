
#include <array>

#include <metal/unit.hpp>

int main(int argc, char * argv[])
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


    return METAL_REPORT();
}
