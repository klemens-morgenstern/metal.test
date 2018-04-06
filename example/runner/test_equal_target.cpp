
//[test_equal_target_def
void test_equal(bool condition, const char * lhs_name, const char * rhs_name, const char* file, int line) {}
#define TEST_EQUAL(lhs, rhs)  test_equal(lhs == rhs, #lhs, #rhs, __FILE__, __LINE__)
//]

//[test_equal_target_main
int main(int argc, char*argv[])
{
    int x =  1;
    int y = -1;
    TEST_EQUAL(x, y);
    //yields test_equal(x==y, "x", "y", "test_equal_target.cpp", 13);
    return 0;
}
//]
