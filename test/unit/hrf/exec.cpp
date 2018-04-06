
#include <metal/unit.hpp>

int main(int argc, char * argv[])
{
    METAL_ASSERT_EXECUTE();
    METAL_EXPECT_EXECUTE();

    METAL_ASSERT_NO_EXECUTE();
    METAL_EXPECT_NO_EXECUTE();
    return METAL_REPORT();
}
