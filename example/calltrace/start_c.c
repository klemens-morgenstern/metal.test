#include <metal/calltrace.h>
#include <assert.h>

void foo2();
void bar2();

//[start_example_c

void foo()
{
    foo2();
}

void bar()
{
    bar2();
}

void func()
{
    foo();
    bar();
}

void test_func()
{
    const void * ptr[] = {&foo, &bar};
    metal_calltrace ct = {&func, ptr, 2, 0, 0};
    metal_calltrace_init(&ct);

    func();

    assert(metal_calltrace_success(&ct));

    metal_calltrace_deinit(&ct);
}
//]

//[start_example_ext_c
void test_func_ext()
{
    const void * ptr[] = {&foo, &bar};
    metal_calltrace ct = {&func, ptr, 2, 0, 0};
    metal_calltrace_init(&ct);

    const void * foo2_ptr[] = {&foo2};
    metal_calltrace ct_foo2 = {&foo, foo2_ptr, 1, 0, 0};
    metal_calltrace_init(&ct_foo2);

    const void * bar2_ptr[] = {&bar2};
    metal_calltrace ct_bar2 = {&bar, bar2_ptr, 1, 0, 0};
    metal_calltrace_init(&ct_bar2);

    func();

    assert(metal_calltrace_success(&ct));
    assert(metal_calltrace_success(&ct_foo2));
    assert(metal_calltrace_success(&ct_bar2));

    metal_calltrace_deinit(&ct);
    metal_calltrace_deinit(&ct_foo2);
    metal_calltrace_deinit(&ct_bar2);
}
//]
