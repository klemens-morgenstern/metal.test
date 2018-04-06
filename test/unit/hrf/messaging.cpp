
#include <metal/unit.hpp>

int main(int argc, char * argv[])
{
    METAL_LOG("my log message");
    METAL_CHECKPOINT();
    METAL_ASSERT_MESSAGE(false, "Some message");
    METAL_EXPECT_MESSAGE(true,  "some other message");
    METAL_ASSERT_MESSAGE(true,  "Yet another message");
    METAL_EXPECT_MESSAGE(false, "and another one");
    return METAL_REPORT();
}
