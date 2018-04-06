/**
 * @file   metal/test/calltrace.h
 * @date   02.05.2017
 * @author Klemens D. Morgenstern
 *




 This header provides the C version of the test macros.
 */
#ifndef METAL_CALLTRACE_H_
#define METAL_CALLTRACE_H_

#include <limits.h>
#include <metal/calltrace.def>

#if !defined(METAL_CALLTRACE_DOXYGEN)

#define METAL_NO_INSTRUMENT __attribute__((no_instrument_function))

#endif

#if defined(METAL_CALLTRACE_DOXYGEN)

///The function to be implemented if a timestamp shall be added to the function calls.
metal_timestamp_t metal_timestamp();

///The unsigned int type that is returned by the timestamp function.
typedef detail::metal_timestamp_t metal_timestamp_t;


/**The type carrying the necessary information for the calltrace.
 */
struct metal_calltrace
{
    ///The function which's subcalls are trace.
    const void * fn;

    ///The content, i.e. array of the calls
    const void ** content;
    ///The amount of functions traced
    const int content_size;

    ///How many function calls shall be traced
    const int repeat;
    ///How many function calls shall be ignored before tracing
    const int skip;
};

#else


typedef struct metal_calltrace_ metal_calltrace;

#endif

/**
 This function initializes and registers a calltrace.
 Note that there is a limited amount of calltraces that can be added. The default value is 16, but can be changed by defining
 `METAL_CALLTRACE_STACK_SIZE` while compiling calltrace.

 \section metal_calltrace_init_example Example
 \code{.cpp}

 #include <cassert>
 #include <metal/calltrace.h>

 void func() {foo(); bar();}

 int main(int argc, char *argv[])
 {
    const void * ct_arr[] = {&foo, &bar};
    metal_calltrace ct = {&func, ct_arr, 2, 0, 0};

    assert(metal_calltrace_init(&ct));

    func();
    assert(metal_calltrace_success(&ct));
    assert(metal_calltrace_deinit(&ct));
    return 0;
 }
 \endcode

 \param ct A pointer to the calltrace to be initialized.
 \return A value differnt from zero if it succeeded.

 */
static int METAL_NO_INSTRUMENT metal_calltrace_init(metal_calltrace * ct)
{
    ct->to_skip = ct->skip;
    ct->errored = 0;
    ct->repeated = 0;
    ct->current_position = 0;
    ct->start_depth = -1;

    return __metal_set_calltrace(ct);
}


/** This functions returns a value different from zero if the calltrace was completed.
 * The behaviour depends on the repeat setting of the calltrace.
 * If the calltrace is set to repeat n-times it has to be at least repeated once,
 * while any other number will require the calltrace to be repeated exactly as set.
 *
 * @param ct A pointer to the calltrace that shall be examined.
 * @return Unequal zero if successful.
 */
static int METAL_NO_INSTRUMENT metal_calltrace_complete(metal_calltrace * ct)
{
    int rep_res = ct->repeat == 0 ? (ct->repeated > 0 ) : (ct->repeated >= ct->repeat);
    return (ct->current_position == 0) && rep_res;
}


/** This functions returns a value different from zero if the calltrace was completed and did not err.
 *
 * @param ct A pointer to the calltrace that shall be examined.
 * @return Unequal zero if completed without error.
 */
static int METAL_NO_INSTRUMENT metal_calltrace_success (metal_calltrace * ct)
{
    return metal_calltrace_complete(ct) && !ct->errored;
}

/** This function deinitializes the calltrace and removes it from the stack.
 * @warning This function must be called before the calltrace struct is removed from the stack, otherwise it might casue memory leaks.
 *
 * @param ct The calltrace to be deinitialized
 * @return A value unequal to zero if the calltrace was succesful removed from the stack.
 */
static int METAL_NO_INSTRUMENT metal_calltrace_deinit(metal_calltrace * ct)
{
    return __metal_reset_calltrace(ct);
}


#endif /* METAL_TEST_CALLTRACE_H_ */
