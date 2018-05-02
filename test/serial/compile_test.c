/**
 * @file   compile_test.c
 * @date   09.04.2018
 * @author Klemens D. Morgenstern
 *
 */

#include <metal/serial.h>
#include <stdio.h>

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

FILE * file_ptr;

int main(int argc, char ** args)
{
    if (argc > 1)
        file_ptr = fopen(args[1], "w");
    else
        file_ptr = stdout;

    METAL_SERIAL_INIT();
    int i = 42;

    if (0)
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

    int condition = 42;

    METAL_SERIAL_ASSERT(condition);
    METAL_SERIAL_EXPECT(condition);

    METAL_SERIAL_ASSERT_MESSAGE(condition);
    METAL_SERIAL_EXPECT_MESSAGE(condition);

    METAL_SERIAL_CALL(test_func);

    METAL_SERIAL_ASSERT_NOT_EQUAL(12, i);
    METAL_SERIAL_EXPECT_NOT_EQUAL(32, i);

    METAL_SERIAL_ASSERT_GE(23, i);
    METAL_SERIAL_EXPECT_GE(12, i);

    METAL_SERIAL_ASSERT_LE(i++, 11);
    METAL_SERIAL_EXPECT_LE(23, ++i);

    METAL_SERIAL_ASSERT_GREATER(i, 12);
    METAL_SERIAL_EXPECT_GREATER(i, 32);

    METAL_SERIAL_ASSERT_LESSER(i++, 92);
    METAL_SERIAL_EXPECT_LESSER(++i, 12);

    METAL_SERIAL_EXIT(0);

    fclose(file_ptr);
    return 0;
}

void  write_metal_serial(char c)
{
    putc(c, file_ptr);
}