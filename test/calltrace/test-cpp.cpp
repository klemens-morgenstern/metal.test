/**
 * @file   test/test-cpp.cpp
 * @date   02.05.2017
 * @author Klemens D. Morgenstern
 *

 */

#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>

#include <metal/calltrace.hpp>

using namespace metal;

//to test it only goes off on the specified functions
void unwatched() {};

void foo() {}
void bar() {}
void foobar() {foo(); bar();}


BOOST_AUTO_TEST_CASE(simple)
{
    calltrace<0> ct_empty(&foo);

    calltrace<2> ct      (&foobar, &foo, &bar);
    calltrace<2> ct_fail (&foobar, &bar, &foo);

    calltrace<1> ct_short(&foobar, &foo);
    calltrace<3> ct_long (&foobar, &foo, &bar, &foo);

    BOOST_CHECK(!ct);
    BOOST_CHECK(!ct.errored());
    BOOST_CHECK(ct.inited());
    BOOST_CHECK(!ct.complete());

    BOOST_CHECK(!ct_fail);
    BOOST_CHECK(!ct_fail.errored());
    BOOST_CHECK(ct_fail.inited());
    BOOST_CHECK(!ct_fail.complete());

    unwatched();
    foobar();
    unwatched();

    BOOST_CHECK(ct);
    BOOST_CHECK(!ct.errored());
    BOOST_CHECK(ct.complete());

    BOOST_CHECK(!ct_fail);
    BOOST_CHECK(ct_fail.errored());
    BOOST_CHECK(ct_fail.complete());

    BOOST_CHECK(!ct_short);
    BOOST_CHECK(ct_short.errored());
    BOOST_CHECK(ct_short.complete());

    BOOST_CHECK(!ct_long);
    BOOST_CHECK(ct_long.errored());
    BOOST_CHECK(ct_long.complete());

    BOOST_CHECK(ct_empty);
}

void foo_switch(bool switch_)
{
    foo();
    bar();
    if (switch_)
        foo();
    else
        bar();
}

BOOST_AUTO_TEST_CASE(repetition)
{
    calltrace<3> ct     (&foo_switch, &foo, &bar, &bar);
    calltrace<3> ct_once(&foo_switch, 1, &foo, &bar, &bar);
    calltrace<3> ct_skip(&foo_switch, 0, 1, &foo, &bar, &foo);
    calltrace<3> ct_rep (&foo_switch, 2, &foo, &bar, &bar);
    calltrace<3> ct_flex(&foo_switch, 2, &foo, &bar, any_fn);

    BOOST_CHECK(ct.inited());
    BOOST_CHECK(ct_once.inited());
    BOOST_CHECK(ct_skip.inited());
    BOOST_CHECK(ct_rep.inited());
    BOOST_CHECK(ct_flex.inited());

    unwatched();
    foo_switch(false);

    BOOST_CHECK(ct);
    unwatched();
    foo_switch(true);
    unwatched();

    BOOST_CHECK(ct.complete());
    BOOST_CHECK(ct_once.complete());
    BOOST_CHECK(ct_skip.complete());
    BOOST_CHECK(ct_rep.complete());
    BOOST_CHECK(ct_flex.complete());

    BOOST_CHECK(!ct);
    BOOST_CHECK(ct_once);
    BOOST_CHECK(ct_skip);
    BOOST_CHECK(!ct_rep);
    BOOST_CHECK(ct_flex);
}

void func()
{
    foo_switch(true);
    foobar();
    foo_switch(false);
}

BOOST_AUTO_TEST_CASE(nested)
{
    calltrace<3> ct_func(&func, &foo_switch, &foobar, &foo_switch);
    calltrace<3> ct_switch(&foo_switch, 2, &foo, &bar, any_fn);

    calltrace<2> ct_fb(&foobar, &foo, &bar);

    calltrace<3> ct_1(&foo_switch, 1, 0, &foo, &bar, &foo);
    calltrace<3> ct_2(&foo_switch, 1, 1, &foo, &bar, &bar);

    BOOST_CHECK(ct_func.inited());
    BOOST_CHECK(ct_switch.inited());
    BOOST_CHECK(ct_fb.inited());
    BOOST_CHECK(ct_1.inited());
    BOOST_CHECK(ct_2.inited());
    unwatched();
    func();
    unwatched();

    BOOST_CHECK(ct_func);
    BOOST_CHECK(ct_switch);
    BOOST_CHECK(ct_fb);
    BOOST_CHECK(ct_1);
    BOOST_CHECK(ct_2);
}

BOOST_AUTO_TEST_CASE(ovl)
{
    calltrace<1> ct_0(&foo, &bar);
    calltrace<1> ct_1(&foo, &bar);
    calltrace<1> ct_2(&foo, &bar);
    calltrace<1> ct_3(&foo, &bar);
    calltrace<1> ct_4(&foo, &bar);
    calltrace<1> ct_5(&foo, &bar);
    calltrace<1> ct_6(&foo, &bar);
    calltrace<1> ct_7(&foo, &bar);
    calltrace<1> ct_8(&foo, &bar);
    calltrace<1> ct_9(&foo, &bar);
    calltrace<1> ct_A(&foo, &bar);
    calltrace<1> ct_B(&foo, &bar);
    calltrace<1> ct_C(&foo, &bar);
    calltrace<1> ct_D(&foo, &bar);
    calltrace<1> ct_E(&foo, &bar);
    calltrace<1> ct_F(&foo, &bar);
    calltrace<1> ct_10(&foo, &bar);

    BOOST_CHECK(ct_0.inited());
    BOOST_CHECK(ct_1.inited());
    BOOST_CHECK(ct_2.inited());
    BOOST_CHECK(ct_3.inited());
    BOOST_CHECK(ct_4.inited());
    BOOST_CHECK(ct_5.inited());
    BOOST_CHECK(ct_6.inited());
    BOOST_CHECK(ct_7.inited());
    BOOST_CHECK(ct_8.inited());
    BOOST_CHECK(ct_9.inited());
    BOOST_CHECK(ct_A.inited());
    BOOST_CHECK(ct_B.inited());
    BOOST_CHECK(ct_C.inited());
    BOOST_CHECK(ct_D.inited());
    BOOST_CHECK(ct_E.inited());
    BOOST_CHECK(ct_F.inited());

    BOOST_CHECK(!ct_10.inited());
}

void recurse(int cnt)
{
    if (cnt > 0)
        recurse(cnt - 1);
}

BOOST_AUTO_TEST_CASE(recursion)
{
    calltrace<1> ct0(&recurse, 1, 0, &recurse);
    calltrace<1> ct1(&recurse, 1, 1, &recurse);
    calltrace<1> ct2(&recurse, 1, 2, &recurse);
    calltrace<1> ct3(&recurse, 1, 3 ,&recurse);

    recurse(4);
    unwatched();

    BOOST_CHECK(ct0);
    BOOST_CHECK(ct1);
    BOOST_CHECK(ct2);
    BOOST_CHECK(ct3);
}
