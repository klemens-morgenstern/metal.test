/**
 * @file   metal/test/unit.hpp
 * @date   29.07.2016
 * @author Klemens D. Morgenstern
 *




 This header provides the C++ version of the test macros.
 */
#ifndef METAL_UNIT_HPP_
#define METAL_UNIT_HPP_


#include <metal/unit.def>

#if (__cplusplus < 201103L )
#error "C++11 is required"
#endif


#define METAL_ERROR()       !__metal_status
#define METAL_STATUS()      +__metal_status
#define METAL_ERRORED()     +__metal_errored
#define METAL_IS_CRITICAL() +__metal_critical
#define METAL_CALL(Function, Message) __metal_call(Function, Message, __FILE__, __LINE__);

#define METAL_REPORT() __metal_report()

#define METAL_BITWISE_EXPR(Lhs, Rhs, Oper, Chain)                                          \
        (__METAL_BITWISE_SIZE(Lhs, Rhs) == 1)   ? __METAL_BITWISE_8(Lhs, Rhs, Oper, Chain)  : \
        ((__METAL_BITWISE_SIZE(Lhs, Rhs) == 2)  ? __METAL_BITWISE_16(Lhs, Rhs, Oper, Chain) : \
         ((__METAL_BITWISE_SIZE(Lhs, Rhs) == 4) ? __METAL_BITWISE_32(Lhs, Rhs, Oper, Chain) : \
            __METAL_BITWISE_64(Lhs, Rhs,  Oper, Chain) ) )

#define METAL_BITWISE(Level, Lhs, Rhs, Oper, Chain, OperId) \
__metal_impl(Level, OperId, METAL_BITWISE_EXPR(Lhs, Rhs, Oper, Chain), 1, #Lhs, #Rhs, #Oper, __FILE__, __LINE__);

#define METAL_RANGE_ENTER(Level, LhsDistance, RhsDistance, Message) \
__metal_impl(Level, __metal_oper_enter_ranged,  LhsDistance == RhsDistance, 0, Message, #LhsDistance, #RhsDistance, __FILE__, __LINE__);

#define METAL_RANGE_EXIT(Level,  Status, Message) \
__metal_impl(Level, __metal_oper_exit_ranged, Status, 0, Message, 0, 0,__FILE__, __LINE__);

#define METAL_RANGED(Level, LhsBegin, LhsEnd, RhsBegin, RhsEnd, MACRO )                                                \
{                                                                                                                   \
    int status = 1;                                                                                                 \
    std::size_t LhsDistance = LhsEnd - LhsBegin;                                                                    \
    std::size_t RhsDistance = RhsEnd - RhsBegin;                                                                    \
    METAL_RANGE_ENTER(Level, LhsDistance, RhsDistance, "{[" #LhsBegin ", " #LhsEnd "], [" #RhsBegin ", " #RhsEnd "]}");\
                                                                                                                    \
    status = METAL_STATUS();                                                                                           \
                                                                                                                    \
    auto RhsItr = RhsBegin;                                                                                         \
    auto LhsItr = LhsBegin;                                                                                         \
    while ((RhsItr != RhsEnd) && (LhsItr != LhsEnd))                                                                \
    {                                                                                                               \
        MACRO ;                                                                                                     \
        status &= METAL_STATUS();                                                                                      \
        RhsItr ++;                                                                                                  \
        LhsItr ++;                                                                                                  \
    }                                                                                                               \
                                                                                                                    \
    METAL_RANGE_EXIT(Level, status, "{[" #RhsBegin ", " #RhsEnd "] , [" #LhsBegin ", " #LhsEnd "]}");                  \
    __metal_status = status;                                                                                           \
}


#define METAL_STATIC_ASSERT(Condition, Message) \
static_assert(Condition, "\n" METAL_LOCATION_STR() " static assertion failed: " Message "\n");

//Operations
#define METAL_LOG(Message) __metal_impl(__metal_level_expect, __metal_oper_log, 1, 0, Message, 0, 0, __FILE__, __LINE__);
#define METAL_CHECKPOINT() __metal_impl(__metal_level_expect, __metal_oper_checkpoint,        1, 0,       0, 0, 0, __FILE__, __LINE__);
#define METAL_ASSERT_MESSAGE(Condition, Message) __metal_impl(__metal_level_assert, __metal_oper_message, Condition, 0, Message, 0, 0, __FILE__, __LINE__);
#define METAL_EXPECT_MESSAGE(Condition, Message) __metal_impl(__metal_level_expect, __metal_oper_message, Condition, 0, Message, 0, 0, __FILE__, __LINE__);

#define METAL_ASSERT(Condition) __metal_impl(__metal_level_assert, __metal_oper_plain, Condition, 0, #Condition, 0, 0, __FILE__, __LINE__);
#define METAL_EXPECT(Condition) __metal_impl(__metal_level_expect, __metal_oper_plain, Condition, 0, #Condition, 0, 0, __FILE__, __LINE__);

#define METAL_ASSERT_PREDICATE(Function, Args...) __metal_impl(__metal_level_assert, __metal_oper_predicate, Function(Args), 0, #Function, #Args, 0, __FILE__, __LINE__);
#define METAL_EXPECT_PREDICATE(Function, Args...) __metal_impl(__metal_level_expect, __metal_oper_predicate, Function(Args), 0, #Function, #Args, 0, __FILE__, __LINE__);
#define METAL_STATIC_ASSERT_PREDICATE(Function, Args...) METAL_STATIC_ASSERT(Function(Args), #Function "(" #Args ")");

#define METAL_ASSERT_EQUAL(Lhs, Rhs) __metal_impl(__metal_level_assert, __metal_oper_equal, Lhs == Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_EXPECT_EQUAL(Lhs, Rhs) __metal_impl(__metal_level_expect, __metal_oper_equal, Lhs == Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_ASSERT_EQUAL_BITWISE(Lhs, Rhs) METAL_BITWISE(__metal_level_assert, Lhs, Rhs, ==, &&, __metal_oper_equal)
#define METAL_EXPECT_EQUAL_BITWISE(Lhs, Rhs) METAL_BITWISE(__metal_level_expect, Lhs, Rhs, ==, &&, __metal_oper_equal)

#define METAL_STATIC_ASSERT_EQUAL(Lhs, Rhs) METAL_STATIC_ASSERT(Lhs == Rhs, #Lhs " == " #Rhs)
#define METAL_STATIC_ASSERT_EQUAL_BITWISE(Lhs, Rhs) METAL_STATIC_ASSERT(METAL_BITWISE_EXPR(Rhs, Lhs, ==, &&), " [bitwise] " #Rhs " == " #Lhs)

#define METAL_ASSERT_NOT_EQUAL(Lhs, Rhs) __metal_impl(__metal_level_assert, __metal_oper_not_equal, Lhs != Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_EXPECT_NOT_EQUAL(Lhs, Rhs) __metal_impl(__metal_level_expect, __metal_oper_not_equal, Lhs != Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_ASSERT_NOT_EQUAL_BITWISE(Lhs, Rhs) METAL_BITWISE(__metal_level_assert, Lhs, Rhs, != , ||, __metal_oper_not_equal);
#define METAL_EXPECT_NOT_EQUAL_BITWISE(Lhs, Rhs) METAL_BITWISE(__metal_level_expect, Lhs, Rhs, != , ||, __metal_oper_not_equal);

#define METAL_STATIC_ASSERT_NOT_EQUAL(Lhs, Rhs) METAL_STATIC_ASSERT(Rhs != Lhs, #Lhs " != " #Rhs)
#define METAL_STATIC_ASSERT_NOT_EQUAL_BITWISE(Lhs, Rhs) METAL_STATIC_ASSERT(METAL_BITWISE_EXPR(Lhs, Rhs, !=, ||), " [bitwise] " #Lhs " != " #Rhs)

#define METAL_ASSERT_CLOSE(Lhs, Rhs, Tolerance) __metal_impl(__metal_level_assert, __metal_oper_close, (Rhs <= (Lhs + Tolerance)) && (Rhs >= (Lhs - Tolerance)), 0, #Lhs, #Rhs, #Tolerance, __FILE__, __LINE__);
#define METAL_EXPECT_CLOSE(Lhs, Rhs, Tolerance) __metal_impl(__metal_level_expect, __metal_oper_close, (Rhs <= (Lhs + Tolerance)) && (Rhs >= (Lhs - Tolerance)), 0, #Lhs, #Rhs, #Tolerance, __FILE__, __LINE__);
#define METAL_STATIC_ASSERT_CLOSE(Lhs, Rhs, Tolerance) \
    METAL_STATIC_ASSERT((Lhs <= (Rhs + Tolerance)) && (Lhs >= (Rhs - Tolerance)) , #Lhs " == " #Rhs " +/- " #Tolerance)

#define METAL_ASSERT_CLOSE_RELATIVE(Lhs, Rhs, Tolerance) __metal_impl(__metal_level_assert, __metal_oper_close_rel, (Rhs <= (Lhs * (1. + Tolerance))) && (Rhs >= (Lhs * (1. - Tolerance))), 0, #Lhs, #Rhs, #Tolerance, __FILE__, __LINE__);
#define METAL_EXPECT_CLOSE_RELATIVE(Lhs, Rhs, Tolerance) __metal_impl(__metal_level_expect, __metal_oper_close_rel, (Rhs <= (Lhs * (1. + Tolerance))) && (Rhs >= (Lhs * (1. - Tolerance))), 0, #Lhs, #Rhs, #Tolerance, __FILE__, __LINE__);
#define METAL_STATIC_ASSERT_CLOSE_RELATIVE(Lhs, Rhs, Tolerance) \
    METAL_STATIC_ASSERT((Lhs <= (Rhs * (1. + Tolerance))) && (Lhs >= (Rhs * (1. - Tolerance))) , #Lhs " == " #Rhs " +/- " #Tolerance " ~")

#define METAL_ASSERT_CLOSE_PERCENT(Lhs, Rhs, Tolerance) __metal_impl(__metal_level_assert, __metal_oper_close_perc, (Rhs <= (Lhs * (1. + ( Tolerance / 100.)))) && (Rhs >= (Lhs * (1. - (Tolerance / 100.)))), 0, #Lhs, #Rhs, #Tolerance, __FILE__, __LINE__);
#define METAL_EXPECT_CLOSE_PERCENT(Lhs, Rhs, Tolerance) __metal_impl(__metal_level_expect, __metal_oper_close_perc, (Rhs <= (Lhs * (1. + ( Tolerance / 100.)))) && (Rhs >= (Lhs * (1. - (Tolerance / 100.)))), 0, #Lhs, #Rhs, #Tolerance, __FILE__, __LINE__);
#define METAL_STATIC_ASSERT_CLOSE_PERCENT(Lhs, Rhs, Tolerance) \
    METAL_STATIC_ASSERT((Rhs <= (Lhs * (1. + (Tolerance / 100.)))) && (Rhs >= (Lhs * (1. - ( Tolerance / 100.)))) , #Lhs " == " #Rhs " +/- " #Tolerance "%")

#define METAL_ASSERT_GE(Lhs, Rhs) __metal_impl(__metal_level_assert, __metal_oper_ge, Lhs >= Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_EXPECT_GE(Lhs, Rhs) __metal_impl(__metal_level_expect, __metal_oper_ge, Lhs >= Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_ASSERT_GE_BITWISE(Lhs, Rhs) METAL_BITWISE(__metal_level_assert, Lhs, Rhs, >=, &&, __metal_oper_ge)
#define METAL_EXPECT_GE_BITWISE(Lhs, Rhs) METAL_BITWISE(__metal_level_expect, Lhs, Rhs, >=, &&, __metal_oper_ge)
#define METAL_STATIC_ASSERT_GE(Lhs, Rhs) METAL_STATIC_ASSERT(Lhs >= Rhs, #Lhs " >= " #Rhs)
#define METAL_STATIC_ASSERT_GE_BITWISE(Lhs, Rhs) METAL_STATIC_ASSERT(METAL_BITWISE_EXPR(Lhs, Rhs, >=, &&), " [bitwise] " #Lhs " >= " #Rhs)

#define METAL_ASSERT_LE(Lhs, Rhs) __metal_impl(__metal_level_assert, __metal_oper_le, Lhs <= Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_EXPECT_LE(Lhs, Rhs) __metal_impl(__metal_level_expect, __metal_oper_le, Lhs <= Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_ASSERT_LE_BITWISE(Lhs, Rhs) METAL_BITWISE(__metal_level_assert, Lhs, Rhs, <=, &&, __metal_oper_le)
#define METAL_EXPECT_LE_BITWISE(Lhs, Rhs) METAL_BITWISE(__metal_level_expect, Lhs, Rhs, <=, &&, __metal_oper_le)
#define METAL_STATIC_ASSERT_LE(Lhs, Rhs) METAL_STATIC_ASSERT(Lhs <= Rhs, #Lhs " <= " #Rhs)
#define METAL_STATIC_ASSERT_LE_BITWISE(Lhs, Rhs) METAL_STATIC_ASSERT(METAL_BITWISE_EXPR(Lhs, Rhs, <=, &&), " [bitwise] " #Lhs " <= " #Rhs)

#define METAL_ASSERT_GREATER(Lhs, Rhs) __metal_impl(__metal_level_assert, __metal_oper_greater, Lhs > Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_EXPECT_GREATER(Lhs, Rhs) __metal_impl(__metal_level_expect, __metal_oper_greater, Lhs > Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_STATIC_ASSERT_GREATER(Lhs, Rhs) METAL_STATIC_ASSERT(Lhs > Rhs, #Lhs " > " #Rhs)

#define METAL_ASSERT_LESSER(Lhs, Rhs) __metal_impl(__metal_level_assert, __metal_oper_lesser, Lhs < Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_EXPECT_LESSER(Lhs, Rhs) __metal_impl(__metal_level_expect, __metal_oper_lesser, Lhs < Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_STATIC_ASSERT_LESSER(Lhs, Rhs) METAL_STATIC_ASSERT(Lhs < Rhs, #Lhs " < " #Rhs)

#define METAL_ASSERT_EQUAL_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd) METAL_RANGED(__metal_level_assert, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_ASSERT_EQUAL(*LhsItr, *RhsItr))
#define METAL_EXPECT_EQUAL_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd) METAL_RANGED(__metal_level_expect, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_EXPECT_EQUAL(*LhsItr, *RhsItr))
#define METAL_ASSERT_EQUAL_BITWISE_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd) METAL_RANGED(__metal_level_assert, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_ASSERT_EQUAL_BITWISE(*LhsItr, *RhsItr))
#define METAL_EXPECT_EQUAL_BITWISE_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd) METAL_RANGED(__metal_level_expect, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_EXPECT_EQUAL_BITWISE(*LhsItr, *RhsItr))

#define METAL_ASSERT_NOT_EQUAL_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd) METAL_RANGED(__metal_level_assert, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_ASSERT_NOT_EQUAL(*LhsItr, *RhsItr))
#define METAL_EXPECT_NOT_EQUAL_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd) METAL_RANGED(__metal_level_expect, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_EXPECT_NOT_EQUAL(*LhsItr, *RhsItr))
#define METAL_ASSERT_NOT_EQUAL_BITWISE_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd) METAL_RANGED(__metal_level_assert, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_ASSERT_NOT_EQUAL_BITWISE(*LhsItr, *RhsItr))
#define METAL_EXPECT_NOT_EQUAL_BITWISE_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd) METAL_RANGED(__metal_level_expect, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_EXPECT_NOT_EQUAL_BITWISE(*LhsItr, *RhsItr))

#define METAL_ASSERT_CLOSE_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd, Tolerance) METAL_RANGED(__metal_level_assert, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_ASSERT_CLOSE(*LhsItr, *RhsItr, Tolerance))
#define METAL_EXPECT_CLOSE_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd, Tolerance) METAL_RANGED(__metal_level_expect, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_EXPECT_CLOSE(*LhsItr, *RhsItr, Tolerance))

#define METAL_ASSERT_CLOSE_RELATIVE_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd, Tolerance) METAL_RANGED(__metal_level_assert, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_ASSERT_CLOSE_RELATIVE(*LhsItr, *RhsItr, Tolerance))
#define METAL_EXPECT_CLOSE_RELATIVE_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd, Tolerance) METAL_RANGED(__metal_level_expect, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_EXPECT_CLOSE_RELATIVE(*LhsItr, *RhsItr, Tolerance))

#define METAL_ASSERT_CLOSE_PERCENT_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd, Tolerance) METAL_RANGED(__metal_level_assert, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_ASSERT_CLOSE_PERCENT(*LhsItr, *RhsItr, Tolerance))
#define METAL_EXPECT_CLOSE_PERCENT_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd, Tolerance) METAL_RANGED(__metal_level_expect, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_EXPECT_CLOSE_PERCENT(*LhsItr, *RhsItr, Tolerance))

#define METAL_ASSERT_GE_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd) METAL_RANGED(__metal_level_assert, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_ASSERT_GE(*LhsItr, *RhsItr))
#define METAL_EXPECT_GE_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd) METAL_RANGED(__metal_level_expect, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_EXPECT_GE(*LhsItr, *RhsItr))
#define METAL_ASSERT_GE_BITWISE_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd) METAL_RANGED(__metal_level_assert, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_ASSERT_GE_BITWISE(*LhsItr, *RhsItr))
#define METAL_EXPECT_GE_BITWISE_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd) METAL_RANGED(__metal_level_expect, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_EXPECT_GE_BITWISE(*LhsItr, *RhsItr))

#define METAL_ASSERT_LE_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd) METAL_RANGED(__metal_level_assert, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_ASSERT_LE(*LhsItr, *RhsItr))
#define METAL_EXPECT_LE_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd) METAL_RANGED(__metal_level_expect, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_EXPECT_LE(*LhsItr, *RhsItr))
#define METAL_ASSERT_LE_BITWISE_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd) METAL_RANGED(__metal_level_assert, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_ASSERT_LE_BITWISE(*LhsItr, *RhsItr))
#define METAL_EXPECT_LE_BITWISE_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd) METAL_RANGED(__metal_level_expect, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_EXPECT_LE_BITWISE(*LhsItr, *RhsItr))

#define METAL_ASSERT_GREATER_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd) METAL_RANGED(__metal_level_assert, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_ASSERT_GREATER(*LhsItr, *RhsItr))
#define METAL_EXPECT_GREATER_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd) METAL_RANGED(__metal_level_expect, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_EXPECT_GREATER(*LhsItr, *RhsItr))

#define METAL_ASSERT_LESSER_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd) METAL_RANGED(__metal_level_assert, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_ASSERT_LESSER(*LhsItr, *RhsItr))
#define METAL_EXPECT_LESSER_RANGED(LhsBegin, LhsEnd, RhsBegin, RhsEnd) METAL_RANGED(__metal_level_expect, LhsBegin, LhsEnd, RhsBegin, RhsEnd, METAL_EXPECT_LESSER(*LhsItr, *RhsItr))

#define __METAL_EXCEPTION(Name, All) catch ( Name & ) { __metal_impl(__metal_level_expect, __metal_oper_exception, 1, 0, #Name, All, 0, __FILE__, __LINE__); }
#define __METAL_EXCEPTIONS_1(All, Arg1)          __METAL_EXCEPTION(Arg1, All)
#define __METAL_EXCEPTIONS_2(All, Arg1, Args...) __METAL_EXCEPTION(Arg1, All) __METAL_EXCEPTIONS_1(All, Args)
#define __METAL_EXCEPTIONS_3(All, Arg1, Args...) __METAL_EXCEPTION(Arg1, All) __METAL_EXCEPTIONS_2(All, Args)
#define __METAL_EXCEPTIONS_4(All, Arg1, Args...) __METAL_EXCEPTION(Arg1, All) __METAL_EXCEPTIONS_3(All, Args)
#define __METAL_EXCEPTIONS_5(All, Arg1, Args...) __METAL_EXCEPTION(Arg1, All) __METAL_EXCEPTIONS_4(All, Args)
#define __METAL_EXCEPTIONS_6(All, Arg1, Args...) __METAL_EXCEPTION(Arg1, All) __METAL_EXCEPTIONS_5(All, Args)
#define __METAL_EXCEPTIONS_7(All, Arg1, Args...) __METAL_EXCEPTION(Arg1, All) __METAL_EXCEPTIONS_6(All, Args)
#define __METAL_EXCEPTIONS_8(All, Arg1, Args...) __METAL_EXCEPTION(Arg1, All) __METAL_EXCEPTIONS_7(All, Args)
#define __METAL_EXCEPTIONS_9(All, Arg1, Args...) __METAL_EXCEPTION(Arg1, All) __METAL_EXCEPTIONS_8(All, Args)
#define __METAL_EXCEPTIONS_A(All, Arg1, Args...) __METAL_EXCEPTION(Arg1, All) __METAL_EXCEPTIONS_9(All, Args)
#define __METAL_EXCEPTIONS_B(All, Arg1, Args...) __METAL_EXCEPTION(Arg1, All) __METAL_EXCEPTIONS_A(All, Args)
#define __METAL_EXCEPTIONS_C(All, Arg1, Args...) __METAL_EXCEPTION(Arg1, All) __METAL_EXCEPTIONS_B(All, Args)
#define __METAL_EXCEPTIONS_D(All, Arg1, Args...) __METAL_EXCEPTION(Arg1, All) __METAL_EXCEPTIONS_C(All, Args)
#define __METAL_EXCEPTIONS_E(All, Arg1, Args...) __METAL_EXCEPTION(Arg1, All) __METAL_EXCEPTIONS_D(All, Args)
#define __METAL_EXCEPTIONS_F(All, Arg1, Args...) __METAL_EXCEPTION(Arg1, All) __METAL_EXCEPTIONS_E(All, Args)

#define __METAL_EXCEPTIONS(Args...) METAL_CONCAT(__METAL_EXCEPTIONS_, __METAL_PP_NARG(Args)) (#Args, Args)

#define METAL_ASSERT_THROW(Code, Exceptions...) try { Code ; __metal_impl(__metal_level_assert, __metal_oper_exception, 0, 0, "expected throw", #Exceptions, 0, __FILE__, __LINE__); } __METAL_EXCEPTIONS(Exceptions) catch(...) {__metal_impl(__metal_level_assert, __metal_oper_exception, 0, 0, "...", #Exceptions, 0, __FILE__, __LINE__); }
#define METAL_EXPECT_THROW(Code, Exceptions...) try { Code ; __metal_impl(__metal_level_expect, __metal_oper_exception, 0, 0, "expected throw", #Exceptions, 0, __FILE__, __LINE__); } __METAL_EXCEPTIONS(Exceptions) catch(...) {__metal_impl(__metal_level_expect, __metal_oper_exception, 0, 0, "...", #Exceptions, 0, __FILE__, __LINE__); }

#define METAL_ASSERT_ANY_THROW(Code) try { Code ; __metal_impl(__metal_level_assert, __metal_oper_any_exception, 0, 0, "expected throw", 0, 0, __FILE__, __LINE__); } catch(...) {__metal_impl(__metal_level_assert, __metal_oper_any_exception, 1, 0, "...", 0, 0, __FILE__, __LINE__); }
#define METAL_EXPECT_ANY_THROW(Code) try { Code ; __metal_impl(__metal_level_expect, __metal_oper_any_exception, 0, 0, "expected throw", 0, 0, __FILE__, __LINE__); } catch(...) {__metal_impl(__metal_level_expect, __metal_oper_any_exception, 1, 0, "...", 0, 0, __FILE__, __LINE__); }

#define METAL_ASSERT_NO_THROW(Code) try { Code ; __metal_impl(__metal_level_assert, __metal_oper_no_exception, 1, 0, "expected throw", 0, 0, __FILE__, __LINE__); } catch(...) {__metal_impl(__metal_level_assert, __metal_oper_no_exception, 0, 0, "...", 0, 0, __FILE__, __LINE__); }
#define METAL_EXPECT_NO_THROW(Code) try { Code ; __metal_impl(__metal_level_expect, __metal_oper_no_exception, 1, 0, "expected throw", 0, 0, __FILE__, __LINE__); } catch(...) {__metal_impl(__metal_level_expect, __metal_oper_no_exception, 0, 0, "...", 0, 0, __FILE__, __LINE__); }

#define METAL_ENTER_TRY() try {
    
#define METAL_ASSERT_THROW_EXIT(Exceptions...) __metal_impl(__metal_level_assert, __metal_oper_exception, 0, 0, "expected throw", #Exceptions, 0, __FILE__, __LINE__); } __METAL_EXCEPTIONS(Exceptions) catch(...) {__metal_impl(__metal_level_assert, __metal_oper_exception, 0, 0, "...", #Exceptions, 0, __FILE__, __LINE__); }
#define METAL_EXPECT_THROW_EXIT(Exceptions...) __metal_impl(__metal_level_expect, __metal_oper_exception, 0, 0, "expected throw", #Exceptions, 0, __FILE__, __LINE__); } __METAL_EXCEPTIONS(Exceptions) catch(...) {__metal_impl(__metal_level_expect, __metal_oper_exception, 0, 0, "...", #Exceptions, 0, __FILE__, __LINE__); }

#define METAL_ASSERT_ANY_THROW_EXIT() __metal_impl(__metal_level_assert, __metal_oper_any_exception, 0, 0, "expected throw", 0, 0, __FILE__, __LINE__); } catch(...) {__metal_impl(__metal_level_assert, __metal_oper_any_exception, 1, 0, "...", 0, 0, __FILE__, __LINE__); }
#define METAL_EXPECT_ANY_THROW_EXIT() __metal_impl(__metal_level_expect, __metal_oper_any_exception, 0, 0, "expected throw", 0, 0, __FILE__, __LINE__); } catch(...) {__metal_impl(__metal_level_expect, __metal_oper_any_exception, 1, 0, "...", 0, 0, __FILE__, __LINE__); }

#define METAL_ASSERT_NO_THROW_EXIT() __metal_impl(__metal_level_assert, __metal_oper_no_exception, 1, 0, "expected throw", 0, 0, __FILE__, __LINE__); } catch(...) {__metal_impl(__metal_level_assert, __metal_oper_no_exception, 0, 0, "...", 0, 0, __FILE__, __LINE__); }
#define METAL_EXPECT_NO_THROW_EXIT() __metal_impl(__metal_level_expect, __metal_oper_no_exception, 1, 0, "expected throw", 0, 0, __FILE__, __LINE__); } catch(...) {__metal_impl(__metal_level_expect, __metal_oper_no_exception, 0, 0, "...", 0, 0, __FILE__, __LINE__); }

#define METAL_ASSERT_NO_EXECUTE() __metal_impl(__metal_level_assert, __metal_oper_no_exec, 0, 0, "unexpected execution", 0, 0, __FILE__, __LINE__);
#define METAL_EXPECT_NO_EXECUTE() __metal_impl(__metal_level_expect, __metal_oper_no_exec, 0, 0, "unexpected execution", 0, 0, __FILE__, __LINE__);

#define METAL_ASSERT_EXECUTE() __metal_impl(__metal_level_assert, __metal_oper_exec, 1, 0, "expected execution", 0, 0, __FILE__, __LINE__);
#define METAL_EXPECT_EXECUTE() __metal_impl(__metal_level_expect, __metal_oper_exec, 1, 0, "expected execution", 0, 0, __FILE__, __LINE__);


#define METAL_CRITICAL(Check)  __metal_critical ++; Check ; __metal_critical--;
#define METAL_ENTER_CRITICAL() __metal_critical ++;
#define METAL_EXIT_CRITICAL()  __metal_critical --;


#if !defined (METAL_NO_IMPLEMENT_INCLUDE)
#include <metal/unit.ipp>
#endif

#endif /* METAL_TEST_BACKEND_HPP_ */
