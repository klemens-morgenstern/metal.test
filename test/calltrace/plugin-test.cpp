/**
 * @file   test/plugin-test.cpp
 * @date   10.05.2017
 * @author Klemens D. Morgenstern
 *



 */

#include <metal/calltrace.hpp>

#include <chrono>
#include <iostream>

#if defined(METAL_TEST_TIMESTAMP)

extern "C" { extern int __metal_calltrace_size; }

metal::metal_timestamp_t __attribute__((no_instrument_function)) metal::timestamp()
{
    int sz = __metal_calltrace_size;
    __metal_calltrace_size = 0;

    metal::metal_timestamp_t ts = std::chrono::system_clock::now().time_since_epoch().count();

    __metal_calltrace_size = sz;
    return ts;
}

#endif

template<typename T>
struct dummy_vector
{
    dummy_vector() {};

    void push_back(T &&) const {};
    void size() const {};
};

dummy_vector<int> dummy;

void bar(bool s) {if (s) dummy.size();}
void foo() {dummy.push_back(42);}
void foobar() {foo(); bar(false); foo(), bar(true);}


int main(int argc, char * argv[])
{
    using namespace metal;

    calltrace<4> ct      (&foobar, &foo, &bar, &foo, &bar);
    calltrace<4> ct_fail (&foobar, &bar, &foo, &bar, &foo);

    calltrace<1> ct_short(&foobar, &foo);
    calltrace<5> ct_long (&foobar, &foo, &bar, &foo, &bar, &foo);

    calltrace<0> ct_no_size  (&bar, 1, 0);
    calltrace<1> ct_size     (&bar, 1, 1, &dummy_vector<int>::size);
    calltrace<1> ct_push_back(&foo, 2, &dummy_vector<int>::push_back);

    foobar();

    int ret = 0;
    if (!ct)           ret += 0b0000001;
    if ( ct_fail)      ret += 0b0000010;
    if ( ct_short)     ret += 0b0000100;
    if ( ct_long)      ret += 0b0001000;
    if (!ct_no_size)   ret += 0b0010000;
    if (!ct_size)      ret += 0b0100000;
    if (!ct_push_back) ret += 0b1000000;

    return ret;
}

