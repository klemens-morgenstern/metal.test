
#include <metal/unit>

void func()
{
    METAL_CRITICAL(METAL_ASSERT(false));
}

int main(int argc, char * argv[])
{
    METAL_CALL(func, "my test case");
    METAL_EXPECT(false);
    return METAL_REPORT();
}
