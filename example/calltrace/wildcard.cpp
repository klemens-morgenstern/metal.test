#include <metal/calltrace.hpp>
#include <cassert>


//[wildcard
struct foo
{
    foo();
    void bar();
    ~foo();
};

void func()
{
    foo f;
    f.bar();
}

void test_func()
{
    metal::test::calltrace<3> ct
        {
         &func,
         metal::test::any_fn, /*<The constructor call>*/
         &foo::bar,
         metal::test::any_fn  /*<The destructor call>*/
        };

    func();

    assert(ct);
}
//]

