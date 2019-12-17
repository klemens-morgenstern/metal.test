/**
 * @file   test/overload.cpp
 * @date   03.05.2017
 * @author Klemens D. Morgenstern
 *



 */

#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>

#include <metal/calltrace.hpp>

using namespace metal;

template<typename T>
bool test(T t)
{
    return detail::func_cast(t) != nullptr;
}

void a() {}
void a(int) {}

struct foo
{
    void a() {}
    void a(int) const {}
    void a(double) volatile {}
    void a(void *) const volatile {}

    void b() {}
    void b() const {}
    void b() volatile {}
    void b() const volatile  {}

    void c() &  {}
    void c() const &  {}
    void c() volatile &  {}
    void c() const volatile &  {}
    void c() && {}
    void c() const && {}
    void c() volatile && {}
    void c() const volatile && {}

    void d() &  {}
    void d() const &  {}
    void d() volatile &  {}
    void d() const volatile &  {}
    void e() && {}
    void e() const && {}
    void e() volatile && {}
    void e() const volatile && {}

    void f() {}
};

BOOST_AUTO_TEST_CASE(fn_test)
{
    BOOST_CHECK(!test(any_fn));
    BOOST_CHECK(test(fn<void()>(&a)));
    BOOST_CHECK(test(fn<void(int)>(&a)));
    BOOST_CHECK(test(fn<void()>(&foo::a)));
    BOOST_CHECK(test(fn<void(int)>(&foo::a)));
    BOOST_CHECK(test(fn<void(double)>(&foo::a)));
    BOOST_CHECK(test(fn<void(void*)>(&foo::a)));
    BOOST_CHECK(test(&foo::f));

    BOOST_CHECK(test(mem_fn<>(&foo::b)));
    BOOST_CHECK(test(mem_fn_c<>(&foo::b)));
    BOOST_CHECK(test(mem_fn_v<>(&foo::b)));
    BOOST_CHECK(test(mem_fn_cv<>(&foo::b)));

#if __GNUC__ >= 10
    //disable because this is bug in GCC
    BOOST_CHECK(test(mem_fn_lvalue<>(&foo::c)));
    BOOST_CHECK(test(mem_fn_c_lvalue<>(&foo::c)));
    BOOST_CHECK(test(mem_fn_v_lvalue<>(&foo::c)));
    BOOST_CHECK(test(mem_fn_cv_lvalue<>(&foo::c)));
    BOOST_CHECK(test(mem_fn_rvalue<>(&foo::c)));
    BOOST_CHECK(test(mem_fn_c_rvalue<>(&foo::c)));
    BOOST_CHECK(test(mem_fn_v_rvalue<>(&foo::c)));
    BOOST_CHECK(test(mem_fn_cv_rvalue<>(&foo::c)));
#endif

    BOOST_CHECK(test(mem_fn<>(&foo::d)));
    BOOST_CHECK(test(mem_fn_c<>(&foo::d)));
    BOOST_CHECK(test(mem_fn_v<>(&foo::d)));
    BOOST_CHECK(test(mem_fn_cv<>(&foo::d)));

    BOOST_CHECK(test(mem_fn<>(&foo::e)));
    BOOST_CHECK(test(mem_fn_c<>(&foo::e)));
    BOOST_CHECK(test(mem_fn_v<>(&foo::e)));
    BOOST_CHECK(test(mem_fn_cv<>(&foo::e)));
}

struct bar
{
    static void a() {}
    void b() {}
    void c() const {}
    void d() volatile {}
    void e() const volatile {}
    void f() & {}
    void g() const & {}
    void h() volatile & {}
    void i() const volatile & {}
    void j() && {}
    void k() const && {}
    void l() volatile && {}
    void m() const volatile && {}
};

extern "C"
{
    int __metal_set_calltrace  (struct metal_calltrace_ * ct) {return 1;}
    int __metal_reset_calltrace(struct metal_calltrace_ * ct) {return 1;}
}

BOOST_AUTO_TEST_CASE(ct_decl)
{
    calltrace<13> ct (&bar::a, &bar::b, &bar::c, &bar::d, &bar::e, &bar::f, &bar::g, &bar::h, &bar::i, &bar::j, &bar::k, &bar::l, &bar::m);
}

