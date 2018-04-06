
#include <array>

#include <metal/unit.hpp>

int main(int argc, char * argv[])
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


    return METAL_REPORT();
}
