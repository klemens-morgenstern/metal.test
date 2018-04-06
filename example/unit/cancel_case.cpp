#include <metal/unit>

//[cancel_case
void test_case()
{
    METAL_CRITICAL(METAL_ASSERT(0));
}

int main(int argc, char * argv[])
{
    METAL_CALL(func, "my test case");
    METAL_EXPECT(0);
    return METAL_REPORT();
}
//]
