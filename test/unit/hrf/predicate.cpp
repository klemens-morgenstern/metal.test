
#include <metal/unit>

bool my_predicate(int i, int j) {return i == j;}

int main(int argc, char * argv[])
{
    METAL_ASSERT_PREDICATE(my_predicate, 4, 2);
    METAL_EXPECT_PREDICATE(my_predicate, 3, 3);
    METAL_ASSERT_PREDICATE(my_predicate, 1, 1);
    METAL_EXPECT_PREDICATE(my_predicate, 5, 2);
    return METAL_REPORT();
}
