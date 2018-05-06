/**
 * @file   compile_test.cpp
 * @date   09.04.2018
 * @author Klemens D. Morgenstern
 *
 */

#include <metal/serial.hpp>
#include <cstdio>
#include <memory>
#include <iostream>

void test_func()
{
    METAL_SERIAL_EXPECT_NO_EXECUTE();
    METAL_SERIAL_CHECKPOINT();
    METAL_SERIAL_ASSERT_NO_EXECUTE();
    METAL_SERIAL_ASSERT_EQUAL(21,2);
    METAL_SERIAL_EXPECT_NO_EXECUTE();
    METAL_SERIAL_EXPECT_EQUAL(12,12);
}

FILE * file_ptr;

int main(int argc, char ** argv)
{
    if (argc > 1)
        file_ptr = std::fopen(argv[1], "w");
    else
        file_ptr = stdout;


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
    const char cstr[] = "foo-str";
    METAL_SERIAL_PRINTF("%c %d %d %s %s",
                        BYTE('a'),
                        INT(42),
                        UINT(12),
                        STR("test-string"),
                        MEMORY(cstr, 7));


    bool condition = true;

    METAL_SERIAL_ASSERT(condition);
    METAL_SERIAL_EXPECT(true);

    METAL_SERIAL_ASSERT_MESSAGE(condition, "Some message");
    METAL_SERIAL_EXPECT_MESSAGE(condition, "Another message");

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

    METAL_SERIAL_TEST_EXIT();

    std::fclose(file_ptr);
    return 0;
}

void  write_metal_serial(char c)
{
    std::putc(c, file_ptr);
}
