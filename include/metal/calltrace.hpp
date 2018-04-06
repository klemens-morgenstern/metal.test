/**
 * @file   metal/test/calltrace.hpp
 * @date   29.07.2016
 * @author Klemens D. Morgenstern
 *




 This header provides the C++ version of the call trace definitions.
 */
#ifndef METAL_CALLTRACE_HPP_
#define METAL_CALLTRACE_HPP_

#include <climits>

#if defined(METAL_CALLTRACE_DOXYGEN)

///Symbol to avoid intrumentation by the calltrace
#define METAL_NO_INSTRUMENT

#else

#define METAL_NO_INSTRUMENT __attribute__((no_instrument_function))

#endif

namespace metal
{

extern "C" {

#include <metal/calltrace.def>

}

#pragma GCC diagnostic ignored "-Wpmf-conversions"

#if defined(METAL_CALLTRACE_DOXYGEN)

///The function to be implemented if a timestamp shall be added to the function calls.
timestamp_t timestamp();

///The unsigned int type that is returned by the timestamp function.
typedef detail::metal_timestamp_t timestamp_t;


#else

typedef metal_timestamp_t timestamp_t;

timestamp_t timestamp() asm("metal_timestamp");

#endif


namespace detail
{
struct any_fn_t
{
    constexpr any_fn_t() METAL_NO_INSTRUMENT {};

    constexpr METAL_NO_INSTRUMENT operator const void* () const {return nullptr;}
};
}

///Helper value to declare a match to any function.
constexpr static detail::any_fn_t any_fn;

/** Convenience type to bind to any function with the given signature to pass to the calltrace.
 * \note This does not take qualifier overloads in member function into account.
 * \section fn_example Example
\code{.cpp}
void foo(int);
void foo(double);

struct bar
{
   bar();
   void f(int);
   void f(double);
   ~bar();
};

bar b;

void func()
{
    foo(42);
    b.f(0.1);
}

calltrace ct{&func,
             fn<void(int)>(&foo), //binds to foo(int)
             fn<void(double)>(&bar::f) //binds to bar::f(double)
             };

\endcode
 */
template<typename T> struct fn;

#if !defined(METAL_CALLTRACE_DOXYGEN)

template<typename Return, typename ... Args>
struct fn<Return(Args...)>
{
    const void* ptr;

    METAL_NO_INSTRUMENT fn(Return (*ptr)(Args...)) : ptr(reinterpret_cast<const void*>(ptr)) {}

    template<typename Class>
    METAL_NO_INSTRUMENT fn(Return (Class::*ptr)(Args...)) : ptr(reinterpret_cast<const void*>(ptr)) {}

    template<typename Class>
    METAL_NO_INSTRUMENT fn(Return (Class::*ptr)(Args...) const )  : ptr(reinterpret_cast<const void*>(ptr)) {}

    template<typename Class>
    METAL_NO_INSTRUMENT fn(Return (Class::*ptr)(Args...) volatile ) : ptr(reinterpret_cast<const void*>(ptr)) {}

    template<typename Class>
    METAL_NO_INSTRUMENT fn(Return (Class::*ptr)(Args...) const volatile ) : ptr(reinterpret_cast<const void*>(ptr)) {}

    template<typename Class>
    METAL_NO_INSTRUMENT fn(Return (Class::*ptr)(Args...) &) : ptr(reinterpret_cast<const void*>(ptr)) {}

    template<typename Class>
    METAL_NO_INSTRUMENT fn(Return (Class::*ptr)(Args...) const &) : ptr(reinterpret_cast<const void*>(ptr)) {}

    template<typename Class>
    METAL_NO_INSTRUMENT fn(Return (Class::*ptr)(Args...) volatile &) : ptr(reinterpret_cast<const void*>(ptr)) {}

    template<typename Class>
    METAL_NO_INSTRUMENT fn(Return (Class::*ptr)(Args...) const volatile &) : ptr(reinterpret_cast<const void*>(ptr)) {}

    template<typename Class>
    METAL_NO_INSTRUMENT fn(Return (Class::*ptr)(Args...) &&) : ptr(reinterpret_cast<const void*>(ptr)) {}

    template<typename Class>
    METAL_NO_INSTRUMENT fn(Return (Class::*ptr)(Args...) const &&) : ptr(reinterpret_cast<const void*>(ptr)) {}

    template<typename Class>
    METAL_NO_INSTRUMENT fn(Return (Class::*ptr)(Args...) volatile &&) : ptr(reinterpret_cast<const void*>(ptr)) {}

    template<typename Class>
    METAL_NO_INSTRUMENT fn(Return (Class::*ptr)(Args...) const volatile &&) : ptr(reinterpret_cast<const void*>(ptr)) {}

};

#endif

/** Helper type to bind to a member function, either any without const/volatile qualifier or with a specific signature.
\section mem_fn_example Example
\code{.cpp}
struct my_class
{
    void foo();
    void foo() const;

    void bar();
    void bar() const;
    void bar(int);
    void bar(int) const;
};


calltrace ct{&some_func,
             mem_fn<>(&my_class::foo), //binds to my_class::foo()
             mem_fn<void()>(&my_class::bar), //binds to my_class::bar()
             mem_fn<void(int)>(&my_class::bar) //binds to my_class::bar(int)
             };
\endcode

 */
template<typename = void>
struct mem_fn;

#if !defined(METAL_CALLTRACE_DOXYGEN)

template<>
struct mem_fn<void>
{
    const void * ptr;

    template<typename Return, typename Class, typename ...Args>
    METAL_NO_INSTRUMENT mem_fn(Return(Class::*ptr)(Args...)) : ptr(reinterpret_cast<const void*>(ptr)) {}
    template<typename Return, typename Class, typename ...Args>
    METAL_NO_INSTRUMENT mem_fn(Return(Class::*ptr)(Args...) &) : ptr(reinterpret_cast<const void*>(ptr)) {}
    template<typename Return, typename Class, typename ...Args>
    METAL_NO_INSTRUMENT mem_fn(Return(Class::*ptr)(Args...) &&) : ptr(reinterpret_cast<const void*>(ptr)) {}
};

template<typename Return, typename ... Args>
struct mem_fn<Return(Args...)>
{
    const void * ptr;

    template<typename Class>
    METAL_NO_INSTRUMENT mem_fn(Return(Class::*ptr)(Args...))    : ptr(reinterpret_cast<const void*>(ptr)) {}
    template<typename Class>
    METAL_NO_INSTRUMENT mem_fn(Return(Class::*ptr)(Args...) &)  : ptr(reinterpret_cast<const void*>(ptr)) {}
    template<typename Class>
    METAL_NO_INSTRUMENT mem_fn(Return(Class::*ptr)(Args...) &&) : ptr(reinterpret_cast<const void*>(ptr)) {}
};

#endif

///Similar to \ref metal::test::mem_fn plus const qualification.
template<typename = void>
struct mem_fn_c;

#if !defined(METAL_CALLTRACE_DOXYGEN)

template<>
struct mem_fn_c<void>
{
    const void * ptr;

    template<typename Return, typename Class, typename ...Args>
    METAL_NO_INSTRUMENT mem_fn_c(Return(Class::*ptr)(Args...) const) : ptr(reinterpret_cast<const void*>(ptr)) {}
    template<typename Return, typename Class, typename ...Args>
    METAL_NO_INSTRUMENT mem_fn_c(Return(Class::*ptr)(Args...) const &) : ptr(reinterpret_cast<const void*>(ptr)) {}
    template<typename Return, typename Class, typename ...Args>
    METAL_NO_INSTRUMENT mem_fn_c(Return(Class::*ptr)(Args...) const &&) : ptr(reinterpret_cast<const void*>(ptr)) {}
};


template<typename Return, typename ... Args>
struct mem_fn_c<Return(Args...)>
{
    const void * ptr;

    template<typename Class>
    METAL_NO_INSTRUMENT mem_fn_c(Return(Class::*ptr)(Args...) const) : ptr(reinterpret_cast<const void*>(ptr)) {}
    template<typename Class>
    METAL_NO_INSTRUMENT mem_fn_c(Return(Class::*ptr)(Args...) const &) : ptr(reinterpret_cast<const void*>(ptr)) {}
    template<typename Class>
    METAL_NO_INSTRUMENT mem_fn_c(Return(Class::*ptr)(Args...) const &&) : ptr(reinterpret_cast<const void*>(ptr)) {}
};

#endif

///Similar to \ref metal::test::mem_fn plus volatile qualification.
template<typename = void>
struct mem_fn_v;

#if !defined(METAL_CALLTRACE_DOXYGEN)

template<>
struct mem_fn_v<void>
{
    const void * ptr;

    template<typename Return, typename Class, typename ...Args>
    METAL_NO_INSTRUMENT mem_fn_v(Return(Class::*ptr)(Args...) volatile) : ptr(reinterpret_cast<const void*>(ptr)) {}
    template<typename Return, typename Class, typename ...Args>
    METAL_NO_INSTRUMENT mem_fn_v(Return(Class::*ptr)(Args...) volatile &) : ptr(reinterpret_cast<const void*>(ptr)) {}
    template<typename Return, typename Class, typename ...Args>
    METAL_NO_INSTRUMENT mem_fn_v(Return(Class::*ptr)(Args...) volatile &&) : ptr(reinterpret_cast<const void*>(ptr)) {}
};


template<typename Return, typename ... Args>
struct mem_fn_v<Return(Args...)>
{
    const void * ptr;

    template<typename Class>
    METAL_NO_INSTRUMENT mem_fn_v(Return(Class::*ptr)(Args...) volatile)    : ptr(reinterpret_cast<const void*>(ptr)) {}
    template<typename Class>
    METAL_NO_INSTRUMENT mem_fn_v(Return(Class::*ptr)(Args...) volatile &)  : ptr(reinterpret_cast<const void*>(ptr)) {}
    template<typename Class>
    METAL_NO_INSTRUMENT mem_fn_v(Return(Class::*ptr)(Args...) volatile &&) : ptr(reinterpret_cast<const void*>(ptr)) {}
};

#endif

///Similar to \ref metal::test::mem_fn plus const volatile qualification.
template<typename = void>
struct mem_fn_cv;

#if !defined(METAL_CALLTRACE_DOXYGEN)

template<>
struct mem_fn_cv<void>
{
    const void * ptr;

    template<typename Return, typename Class, typename ...Args>
    METAL_NO_INSTRUMENT mem_fn_cv(Return(Class::*ptr)(Args...) const volatile)    : ptr(reinterpret_cast<const void*>(ptr)) {}
    template<typename Return, typename Class, typename ...Args>
    METAL_NO_INSTRUMENT mem_fn_cv(Return(Class::*ptr)(Args...) const volatile &)  : ptr(reinterpret_cast<const void*>(ptr)) {}
    template<typename Return, typename Class, typename ...Args>
    METAL_NO_INSTRUMENT mem_fn_cv(Return(Class::*ptr)(Args...) const volatile &&) : ptr(reinterpret_cast<const void*>(ptr)) {}
};


template<typename Return, typename ... Args>
struct mem_fn_cv<Return(Args...)>
{
    const void * ptr;

    template<typename Class>
    METAL_NO_INSTRUMENT mem_fn_cv(Return(Class::*ptr)(Args...) const volatile)    : ptr(reinterpret_cast<const void*>(ptr)) {}
    template<typename Class>
    METAL_NO_INSTRUMENT mem_fn_cv(Return(Class::*ptr)(Args...) const volatile &)  : ptr(reinterpret_cast<const void*>(ptr)) {}
    template<typename Class>
    METAL_NO_INSTRUMENT mem_fn_cv(Return(Class::*ptr)(Args...) const volatile &&) : ptr(reinterpret_cast<const void*>(ptr)) {}
};

#endif

///Similar to \ref metal::test::mem_fn plus lvalue qualification. \note Doesn't work with a gcc lower than 6.
template<typename = void>
struct mem_fn_lvalue;

#if !defined(METAL_CALLTRACE_DOXYGEN)

template<>
struct mem_fn_lvalue<void>
{
    const void * ptr;

    template<typename Return, typename Class, typename ...Args>
    METAL_NO_INSTRUMENT mem_fn_lvalue(Return(Class::*ptr)(Args...) &) : ptr(reinterpret_cast<const void*>(ptr)) {}
};


template<typename Return, typename ... Args>
struct mem_fn_lvalue<Return(Args...)>
{
    const void * ptr;

    template<typename Class>
    METAL_NO_INSTRUMENT mem_fn_lvalue(Return(Class::*ptr)(Args...) &) : ptr(reinterpret_cast<const void*>(ptr)) {}
};

#endif

///Similar to \ref metal::test::mem_fn plus const and lvalue qualification. \note Doesn't work with a gcc lower than 6.
template<typename = void>
struct mem_fn_c_lvalue;

#if !defined(METAL_CALLTRACE_DOXYGEN)

template<>
struct mem_fn_c_lvalue<void>
{
    const void * ptr;

    template<typename Return, typename Class, typename ...Args>
    METAL_NO_INSTRUMENT mem_fn_c_lvalue(Return(Class::*ptr)(Args...) const &) : ptr(reinterpret_cast<const void*>(ptr)) {}
};


template<typename Return, typename ... Args>
struct mem_fn_c_lvalue<Return(Args...)>
{
    const void * ptr;

    template<typename Class>
    METAL_NO_INSTRUMENT mem_fn_c_lvalue(Return(Class::*ptr)(Args...) const &) : ptr(reinterpret_cast<const void*>(ptr)) {}
};

#endif

///Similar to \ref metal::test::mem_fn plus volatile and lvalue qualification. \note Doesn't work with a gcc lower than 6.
template<typename = void>
struct mem_fn_v_lvalue;

#if !defined(METAL_CALLTRACE_DOXYGEN)

template<>
struct mem_fn_v_lvalue<void>
{
    const void * ptr;

    template<typename Return, typename Class, typename ...Args>
    METAL_NO_INSTRUMENT mem_fn_v_lvalue(Return(Class::*ptr)(Args...) volatile &) : ptr(reinterpret_cast<const void*>(ptr)) {}
};


template<typename Return, typename ... Args>
struct mem_fn_v_lvalue<Return(Args...)>
{
    const void * ptr;

    template<typename Class>
    METAL_NO_INSTRUMENT mem_fn_v_lvalue(Return(Class::*ptr)(Args...) volatile &) : ptr(reinterpret_cast<const void*>(ptr)) {}
};

#endif

///Similar to \ref metal::test::mem_fn plus const volatile and lvalue qualification. \note Doesn't work with a gcc lower than 6.
template<typename = void>
struct mem_fn_cv_lvalue;

#if !defined(METAL_CALLTRACE_DOXYGEN)

template<>
struct mem_fn_cv_lvalue<void>
{
    const void * ptr;

    template<typename Return, typename Class, typename ...Args>
    METAL_NO_INSTRUMENT mem_fn_cv_lvalue(Return(Class::*ptr)(Args...) const volatile &) : ptr(reinterpret_cast<const void*>(ptr)) {}
};


template<typename Return, typename ... Args>
struct mem_fn_cv_lvalue<Return(Args...)>
{
    const void * ptr;

    template<typename Class>
    METAL_NO_INSTRUMENT mem_fn_cv_lvalue(Return(Class::*ptr)(Args...) const volatile &) : ptr(reinterpret_cast<const void*>(ptr)) {}
};

#endif

///Similar to \ref metal::test::mem_fn plus rvalue qualification. \note Doesn't work with a gcc lower than 6.
template<typename = void>
struct mem_fn_rvalue;

#if !defined(METAL_CALLTRACE_DOXYGEN)

template<>
struct mem_fn_rvalue<void>
{
    const void * ptr;

    template<typename Return, typename Class, typename ...Args>
    METAL_NO_INSTRUMENT mem_fn_rvalue(Return(Class::*ptr)(Args...)) : ptr(reinterpret_cast<const void*>(ptr)) {}
};


template<typename Return, typename ... Args>
struct mem_fn_rvalue<Return(Args...)>
{
    const void * ptr;

    template<typename Class>
    METAL_NO_INSTRUMENT mem_fn_rvalue(Return(Class::*ptr)(Args...) &&) : ptr(reinterpret_cast<const void*>(ptr)) {}
};

#endif

///Similar to \ref metal::test::mem_fn plus const and rvalue qualification. \note Doesn't work with a gcc lower than 6.
template<typename = void>
struct mem_fn_c_rvalue;

#if !defined(METAL_CALLTRACE_DOXYGEN)

template<>
struct mem_fn_c_rvalue<void>
{
    const void * ptr;

    template<typename Return, typename Class, typename ...Args>
    METAL_NO_INSTRUMENT mem_fn_c_rvalue(Return(Class::*ptr)(Args...) const &&) : ptr(reinterpret_cast<const void*>(ptr)) {}
};


template<typename Return, typename ... Args>
struct mem_fn_c_rvalue<Return(Args...)>
{
    const void * ptr;

    template<typename Class>
    METAL_NO_INSTRUMENT mem_fn_c_rvalue(Return(Class::*ptr)(Args...) const &&) : ptr(reinterpret_cast<const void*>(ptr)) {}
};

#endif

///Similar to \ref metal::test::mem_fn plus volatile and rvalue qualification. \note Doesn't work with a gcc lower than 6.
template<typename = void>
struct mem_fn_v_rvalue;

#if !defined(METAL_CALLTRACE_DOXYGEN)

template<>
struct mem_fn_v_rvalue<void>
{
    const void * ptr;

    template<typename Return, typename Class, typename ...Args>
    METAL_NO_INSTRUMENT mem_fn_v_rvalue(Return(Class::*ptr)(Args...) volatile &&) : ptr(reinterpret_cast<const void*>(ptr)) {}
};


template<typename Return, typename ... Args>
struct mem_fn_v_rvalue<Return(Args...)>
{
    const void * ptr;

    template<typename Class>
    METAL_NO_INSTRUMENT mem_fn_v_rvalue(Return(Class::*ptr)(Args...) volatile &&) : ptr(reinterpret_cast<const void*>(ptr)) {}
};

#endif

///Similar to \ref metal::test::mem_fn plus const volatile and rvalue qualification. \note Doesn't work with a gcc lower than 6.
template<typename = void>
struct mem_fn_cv_rvalue;

#if !defined(METAL_CALLTRACE_DOXYGEN)

template<>
struct mem_fn_cv_rvalue<void>
{
    const void * ptr;

    template<typename Return, typename Class, typename ...Args>
    METAL_NO_INSTRUMENT mem_fn_cv_rvalue(Return(Class::*ptr)(Args...) const volatile &&) : ptr(reinterpret_cast<const void*>(ptr)) {}
};


template<typename Return, typename ... Args>
struct mem_fn_cv_rvalue<Return(Args...)>
{
    const void * ptr;

    template<typename Class>
    METAL_NO_INSTRUMENT mem_fn_cv_rvalue(Return(Class::*ptr)(Args...) const volatile &&) : ptr(reinterpret_cast<const void*>(ptr)) {}
};


namespace detail
{
    template<typename Return, typename ...Args>
    inline const void * METAL_NO_INSTRUMENT func_cast(Return(*t)(Args...))
    {
        return reinterpret_cast<const void*>(t);
    }

    template<typename Return, typename Class,  typename ...Args>
    inline const void * METAL_NO_INSTRUMENT func_cast(Return(Class::*t)(Args...))
    {
        return reinterpret_cast<const void*>(t);
    }

    template<typename Return, typename Class,  typename ...Args>
    inline const void * METAL_NO_INSTRUMENT func_cast(Return(Class::*t)(Args...) const)
    {
        return reinterpret_cast<const void*>(t);
    }

    template<typename Return, typename Class,  typename ...Args>
    inline const void * METAL_NO_INSTRUMENT func_cast(Return(Class::*t)(Args...) volatile )
    {
        return reinterpret_cast<const void*>(t);
    }

    template<typename Return, typename Class,  typename ...Args>
    inline const void * METAL_NO_INSTRUMENT func_cast(Return(Class::*t)(Args...) const volatile)
    {
        return reinterpret_cast<const void*>(t);
    }

    template<typename Return, typename Class,  typename ...Args>
    inline const void * METAL_NO_INSTRUMENT func_cast(Return(Class::*t)(Args...) &)
    {
        return reinterpret_cast<const void*>(t);
    }


    template<typename Return, typename Class,  typename ...Args>
    inline const void * METAL_NO_INSTRUMENT func_cast(Return(Class::*t)(Args...) const &)
    {
        return reinterpret_cast<const void*>(t);
    }

    template<typename Return, typename Class,  typename ...Args>
    inline const void * METAL_NO_INSTRUMENT func_cast(Return(Class::*t)(Args...) volatile &)
    {
        return reinterpret_cast<const void*>(t);
    }

    template<typename Return, typename Class,  typename ...Args>
    inline const void * METAL_NO_INSTRUMENT func_cast(Return(Class::*t)(Args...) const volatile &)
    {
        return reinterpret_cast<const void*>(t);
    }


    template<typename Return, typename Class,  typename ...Args>
    inline const void * METAL_NO_INSTRUMENT func_cast(Return(Class::*t)(Args...) &&)
    {
        return reinterpret_cast<const void*>(t);
    }


    template<typename Return, typename Class,  typename ...Args>
    inline const void * METAL_NO_INSTRUMENT func_cast(Return(Class::*t)(Args...) const &&)
    {
        return reinterpret_cast<const void*>(t);
    }

    template<typename Return, typename Class,  typename ...Args>
    inline const void * METAL_NO_INSTRUMENT func_cast(Return(Class::*t)(Args...) volatile &&)
    {
        return reinterpret_cast<const void*>(t);
    }

    template<typename Return, typename Class,  typename ...Args>
    inline const void * METAL_NO_INSTRUMENT func_cast(Return(Class::*t)(Args...) const volatile &&)
    {
        return reinterpret_cast<const void*>(t);
    }

    inline const void * METAL_NO_INSTRUMENT func_cast(const any_fn_t &t)
    {
        return nullptr;
    }

    template<typename T>
    inline const void * METAL_NO_INSTRUMENT func_cast(const fn<T> &t)
    {
        return t.ptr;
    }

    template<typename T>
    inline const void * METAL_NO_INSTRUMENT func_cast(const mem_fn<T> &t)
    {
        return t.ptr;
    }

    template<typename T>
    inline const void * METAL_NO_INSTRUMENT func_cast(const mem_fn_c<T> &t)
    {
        return t.ptr;
    }
    template<typename T>
    inline const void * METAL_NO_INSTRUMENT func_cast(const mem_fn_v<T> &t)
    {
        return t.ptr;
    }

    template<typename T>
    inline  const void * METAL_NO_INSTRUMENT func_cast(const mem_fn_cv<T> &t)
    {
        return t.ptr;
    }

    template<typename T>
    inline const void * METAL_NO_INSTRUMENT func_cast(const mem_fn_lvalue<T> &t)
    {
        return t.ptr;
    }

    template<typename T>
    inline const void * METAL_NO_INSTRUMENT func_cast(const mem_fn_c_lvalue<T> &t)
    {
        return t.ptr;
    }
    template<typename T>
    inline const void * METAL_NO_INSTRUMENT func_cast(const mem_fn_v_lvalue<T> &t)
    {
        return t.ptr;
    }

    template<typename T>
    inline const void * METAL_NO_INSTRUMENT func_cast(const mem_fn_cv_lvalue<T> &t)
    {
        return t.ptr;
    }

    template<typename T>
    inline const void * METAL_NO_INSTRUMENT func_cast(const mem_fn_rvalue<T> &t)
    {
        return t.ptr;
    }
    template<typename T>
    inline const void * METAL_NO_INSTRUMENT func_cast(const mem_fn_c_rvalue<T> &t)
    {
        return t.ptr;
    }
    template<typename T>
    inline const void * METAL_NO_INSTRUMENT func_cast(const mem_fn_v_rvalue<T> &t)
    {
        return t.ptr;
    }

    template<typename T>
    inline const void * METAL_NO_INSTRUMENT func_cast(const mem_fn_cv_rvalue<T> &t)
    {
        return t.ptr;
    }

}

#endif

/**The actual calltrace implementation.
 *
 * \tparam Size The number of the functions inside the trace body passed.

\section calltrace_example Example
\code{.cpp}
void bar(std::vector<int> & v);
void foo()
{
    std::vector<int> vec;
    vec.push_back(42);

    bar(vec);
    vec.resize(2);
};

int main(int argc, char* argv[])
{
    using namespace metal::test;
    calltrace ct
        {
            &foo, //starting point
            any_fn, //constructor, no way to get the address
            fn<void(int&&)>(&std::vector<int>::push_back), //the move push back
            &bar, //function not overloaded
            &std::vector<int>::size,
            any_fn //destructor
        };

    foo();
    assert(ct);
    return 0;
};
\endcode
 */
template<std::size_t Size>
class calltrace : metal_calltrace_
{
    const void*  _funcs[Size];
    bool _inited;
public:
    /**Construct an empty (i.e. asserting no calls) calltrace from the given function, including a repeat count and a skip.
     * \param func The function to trace
     * \param repeat The times the calltrace shall be repeated
     * \param skip The amount of calls that shall be ignore before activating the calltrace
     */
    template<typename Func>
    inline METAL_NO_INSTRUMENT calltrace(Func func, int repeat, int skip)
        : metal_calltrace_{detail::func_cast(func), _funcs, static_cast<int>(Size), repeat, skip, skip, 0, 0, 0, -1},
          _funcs{nullptr},
          _inited{__metal_set_calltrace(this) != 0}
    {
    }
    /**Construct an empty calltrace from the given function, including a repeat count, but no skip.
     * \param func The function to trace
     * \param repeat The times the calltrace shall be repeated
     */
    template<typename Func>
    inline METAL_NO_INSTRUMENT calltrace(Func func, int repeat) : calltrace(func, repeat, 0) {}

    /**Construct an empty calltrace from the given function, without a repeat count and skip.
     * \param func The function to trace
     */
    template<typename Func>
    inline METAL_NO_INSTRUMENT calltrace(Func func) : calltrace(func, 0, 0) {};


    /**Construct a calltrace from the given function, including a repeat count and a skip
     * \param func The function to trace
     * \param repeat The times the calltrace shall be repeated
     * \param skip The amount of calls that shall be ignore before activating the calltrace
     * \param args The expected function calls.
     */
    template<typename Func, typename ...Args>
    inline METAL_NO_INSTRUMENT calltrace(Func func, int repeat, int skip, Args... args)
        : metal_calltrace_{detail::func_cast(func), _funcs, static_cast<int>(Size), repeat, skip, skip, 0, 0, 0, -1},
          _funcs{detail::func_cast(args)...},
          _inited{__metal_set_calltrace(this) != 0}
    {
    }
    ///Deleted copy-ctor.
    calltrace(const calltrace &) = delete;

    /**Construct a calltrace from the given function, including a repeat count, but no skip.
     * \param func The function to trace
     * \param repeat The times the calltrace shall be repeated
     * \param args  The expected function calls.
     */
    template<typename Func, typename ...Args>
    inline METAL_NO_INSTRUMENT calltrace(Func func, int repeat, Args... args) : calltrace(func, repeat, 0, args...) {}

    /**Construct a calltrace from the given function, without a repeat count and skip.
     * \param func The function to trace
     * \param args The expected function calls.
     */
    template<typename Func, typename ...Args>
    inline METAL_NO_INSTRUMENT calltrace(Func func, Args ... args) : calltrace(func, 0, 0, args...) {};

    ///Check if the calltrace was inited, i..e added to the calltrace list.
    inline bool METAL_NO_INSTRUMENT inited  () const {return _inited;}
    ///Check if the calltrace had an error occur.
    inline bool METAL_NO_INSTRUMENT errored () const {return metal_calltrace_::errored != 0;}
    /**Check if the calltrace is completed, without checking the error count.
     * The behaviour depends on the repeat setting of the calltrace.
     * If the calltrace is set to repeat n-times it has to be at least repeated once,
     * while any other number will require the calltrace to be repeated exactly as set.
     */
    inline bool METAL_NO_INSTRUMENT complete() const
    {
        bool rep_res = repeat == 0 ? (repeated > 0) : (repeated >= repeat);
        return (current_position == 0) && rep_res;
    }
    ///Check if the calltrace was succesful, i.e. inited, completed and did not err.
    inline bool METAL_NO_INSTRUMENT success () const {return complete() && !errored() && inited();}
    ///Convenience overload for success.
    inline METAL_NO_INSTRUMENT operator bool() const {return success();}

    ///Destructor, removes the calltrace from the list
    METAL_NO_INSTRUMENT ~calltrace()
    {
        __metal_reset_calltrace(this);
    }
};


///Specialization for an empty calltrace
template<>
class calltrace<0> : metal_calltrace_
{
    const void*  _funcs = nullptr;
    bool _inited;
public:
    /**Construct an empty (i.e. asserting no calls) calltrace from the given function, including a repeat count and a skip.
     * \param func The function to trace
     * \param repeat The times the calltrace shall be repeated
     * \param skip The amount of calls that shall be ignore before activating the calltrace
     */
    template<typename Func>
    inline METAL_NO_INSTRUMENT calltrace(Func func, int repeat, int skip)
        : metal_calltrace_{detail::func_cast(func), &_funcs, 0, repeat, skip, skip, 0, 0, 0, -1},
          _inited{__metal_set_calltrace(this) != 0}
    {
    }
    /**Construct an empty calltrace from the given function, including a repeat count, but no skip.
     * \param func The function to trace
     * \param repeat The times the calltrace shall be repeated
     */
    template<typename Func>
    inline METAL_NO_INSTRUMENT calltrace(Func func, int repeat) : calltrace(func, repeat, 0) {}

    /**Construct an empty calltrace from the given function, without a repeat count and skip.
     * \param func The function to trace
     */
    template<typename Func>
    inline METAL_NO_INSTRUMENT calltrace(Func func) : calltrace(func, 0, 0) {};

    ///Check if the calltrace was inited, i..e added to the calltrace list.
    inline bool METAL_NO_INSTRUMENT inited  () const {return _inited;}
    ///Check if the calltrace had an error occur.
    inline bool METAL_NO_INSTRUMENT errored () const {return metal_calltrace_::errored != 0;}
    /**Check if the calltrace is completed, without checking the error count.
     * The behaviour depends on the repeat setting of the calltrace.
     * If the calltrace is set to repeat n-times it has to be at least repeated once,
     * while any other number will require the calltrace to be repeated exactly as set.
     */
    inline bool METAL_NO_INSTRUMENT complete() const
    {
        bool rep_res = repeat == 0 ? (repeated > 0) : (repeated >= repeat);
        return (current_position == 0) && rep_res;
    }
    ///Check if the calltrace was succesful, i.e. inited, completed and did not err.
    inline bool METAL_NO_INSTRUMENT success () const {return complete() && !errored() && inited();}
    ///Convenience overload for success.
    inline METAL_NO_INSTRUMENT operator bool() const {return success();}

    ///Destructor, removes the calltrace from the list
    METAL_NO_INSTRUMENT ~calltrace()
    {
        __metal_reset_calltrace(this);
    }
};



#pragma GCC diagnostic pop


}

#endif /* METAL_TEST_CALLTRACE_HPP_ */
