//[timestamp_example
#include <metal/calltrace.hpp>
#include <chrono>

extern "C" { extern int __metal_calltrace_size; }

metal::test::timestamp_t metal::test::timestamp()
{
    int sz = __metal_calltrace_size;
    __metal_calltrace_size = 0;

    auto ts = std::chrono::system_clock::now()
                .time_since_epoch().count();

    __metal_calltrace_size = sz;
    return ts;
}
//]
