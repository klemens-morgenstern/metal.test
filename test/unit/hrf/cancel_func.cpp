
#include <metal/unit.hpp>

void func()
{
    METAL_CRITICAL(METAL_ASSERT(false));
}

int main(int argc, char * argv[])
{
    func();
    METAL_EXPECT(false);
    return METAL_REPORT();
}
