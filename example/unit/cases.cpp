#include <metal/unit.hpp>

//[case_example
void test_cases_1()
{
    METAL_ASSERT_EQUAL(42, 42);
}

void test_cases_2()
{
    METAL_ASSERT_NOT_EQUAL(42, 42);
}

int main(int argc, char *argv[])
{
    METAL_EXPECT_EQUAL(42,0);
    METAL_CALL(&test_cases_1, "Test Case 1");
    METAL_CALL(&test_cases_2, "Test Case 2");
    return METAL_REPORT();
}
//]
