//[timestamp_example_c
#include <metal/calltrace.h>
#include <time.h>

metal_timestamp_t metal_timestamp()
{
    return (metal_timestamp_t)time(0);
}

//]
