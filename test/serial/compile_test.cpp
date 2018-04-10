/**
 * @file   compile_test.cpp
 * @date   09.04.2018
 * @author Klemens D. Morgenstern
 *
 */

#include <metal/serial.hpp>

void test_func()
{
    METAL_SERIAL_EXPECT_NO_EXECUTE();
    METAL_SERIAL_CHECKPOINT();
    METAL_SERIAL_ASSERT_NO_EXECUTE();
    METAL_SERIAL_ASSERT_EQUAL(21,2);
    METAL_SERIAL_EXPECT_NO_EXECUTE();
    METAL_SERIAL_EXPECT_EQUAL(12,12);
    METAL_SERIAL_ASSERT_EXECUTE();
}

int main()
{
    METAL_SERIAL_INIT();
    int i = 42;

    if (false)
    {
        _METAL_SERIAL_WRITE_BYTE('a');
        _METAL_SERIAL_WRITE_INT(42);
        _METAL_SERIAL_WRITE_STR("test-string");
        _METAL_SERIAL_WRITE_PTR(&main);
        _METAL_SERIAL_WRITE_MEMORY(&i, sizeof(i));
    }

    METAL_SERIAL_PRINTF("%i %i %s %p %i",
                        BYTE('a'),
                        INT(42),
                        STR("test-string"),
                        PTR(&main),
                        MEMORY(&i, sizeof(i)));

    bool condition = true;

    METAL_SERIAL_ASSERT(condition);
    METAL_SERIAL_EXPECT(condition);

    METAL_SERIAL_ASSERT_MESSAGE(condition);
    METAL_SERIAL_EXPECT_MESSAGE(condition);

    METAL_SERIAL_CALL(test_func);

    METAL_SERIAL_ASSERT_NOT_EQUAL(12, i);
    METAL_SERIAL_EXPECT_NOT_EQUAL(32, i);

    METAL_ASSERT_GE(23, i);
    METAL_EXPECT_GE(12, i);

    METAL_ASSERT_LE(i++, 11);
    METAL_EXPECT_LE(23, ++i);

    METAL_ASSERT_GREATER(i, 12);
    METAL_EXPECT_GREATER(i, 32);

    METAL_ASSERT_LESSER(i++, 92);
    METAL_EXPECT_LESSER(++i, 12);

    METAL_EXIT(0);

    return 0;
}