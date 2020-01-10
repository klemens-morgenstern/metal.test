/**
 * @file   metal/test/unit.h
 * @date   10.08.2016
 * @author Klemens D. Morgenstern
 *
 *  This header provides the C++ version of the test macros.
 */
#ifndef METAL_UNIT_H_
#define METAL_UNIT_H_

#include <metal/unit.def>
#include <stdint.h>
#include <stddef.h>


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
__metal_unit_impl(Level, OperId, METAL_BITWISE_EXPR(Lhs, Rhs, Oper, Chain), 1, #Lhs, #Rhs, #Oper, __FILE__, __LINE__);

#define METAL_RANGE_ENTER(Level, LhsDistance, RhsDistance, Message) \
__metal_unit_impl(Level, __metal_oper_enter_ranged,  LhsDistance == RhsDistance, 0, Message, #LhsDistance, #RhsDistance, __FILE__, __LINE__);

#define METAL_RANGE_EXIT(Level,  Status, Message) \
__metal_unit_impl(Level, __metal_oper_exit_ranged, Status, 0, Message, 0, 0,__FILE__, __LINE__);


#define METAL_RANGED(Level, Lhs, LhsSize, Rhs, RhsSize, MACRO )                                                             \
{                                                                                                                        \
    int status = 1;                                                                                                      \
    int i = 0;                                                                                                           \
    METAL_RANGE_ENTER(Level, LhsSize, RhsSize, "{" #Lhs "[0 : " #LhsSize "], " #Rhs "[0 : " #RhsSize "]}");                 \
    const size_t Size = LhsSize > RhsSize ? RhsSize : LhsSize;                                                           \
                                                                                                                         \
    status &= METAL_STATUS();                                                                                               \
                                                                                                                         \
    while (i < Size)                                                                                                     \
    {                                                                                                                    \
        MACRO ;                                                                                                          \
        status &= METAL_STATUS();                                                                                           \
        i++;                                                                                                             \
    }                                                                                                                    \
                                                                                                                         \
    METAL_RANGE_EXIT(Level, status, "{" #Lhs "[0 : " #LhsSize "], " #Rhs "[0 : " #RhsSize "]}");                            \
}

#define METAL_STATIC_ASSERT(Condition, Message) \
typedef char METAL_CONCAT(__metal_static_assert_, __COUNTER__) [Condition ? 1 : -1];

//Operations
#define METAL_LOG(Message) __metal_unit_impl(__metal_level_expect, __metal_oper_log, 1, 0, Message, 0, 0, __FILE__, __LINE__);
#define METAL_CHECKPOINT() __metal_unit_impl(__metal_level_expect, __metal_oper_checkpoint,        1, 0,       0, 0, 0, __FILE__, __LINE__);
#define METAL_ASSERT_MESSAGE(Condition, Message) __metal_unit_impl(__metal_level_assert, __metal_oper_message, Condition, 0, Message, 0, 0, __FILE__, __LINE__);
#define METAL_EXPECT_MESSAGE(Condition, Message) __metal_unit_impl(__metal_level_expect, __metal_oper_message, Condition, 0, Message, 0, 0, __FILE__, __LINE__);

#define METAL_ASSERT(Condition) __metal_unit_impl(__metal_level_assert, __metal_oper_plain, Condition, 0, #Condition, 0, 0, __FILE__, __LINE__);
#define METAL_EXPECT(Condition) __metal_unit_impl(__metal_level_expect, __metal_oper_plain, Condition, 0, #Condition, 0, 0, __FILE__, __LINE__);

#define METAL_ASSERT_PREDICATE(Function, Args...) __metal_unit_impl(__metal_level_assert, __metal_oper_predicate, Function(Args), 0, #Function, #Args, 0, __FILE__, __LINE__);
#define METAL_EXPECT_PREDICATE(Function, Args...) __metal_unit_impl(__metal_level_expect, __metal_oper_predicate, Function(Args), 0, #Function, #Args, 0, __FILE__, __LINE__);
#define METAL_STATIC_ASSERT_PREDICATE(Function, Args...) METAL_STATIC_ASSERT(Function(Args), #Function "(" #Args ")");

#define METAL_ASSERT_EQUAL(Lhs, Rhs) __metal_unit_impl(__metal_level_assert, __metal_oper_equal, Lhs == Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_EXPECT_EQUAL(Lhs, Rhs) __metal_unit_impl(__metal_level_expect, __metal_oper_equal, Lhs == Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_ASSERT_EQUAL_BITWISE(Lhs, Rhs) METAL_BITWISE(__metal_level_assert, Lhs, Rhs, ==, &&, __metal_oper_equal)
#define METAL_EXPECT_EQUAL_BITWISE(Lhs, Rhs) METAL_BITWISE(__metal_level_expect, Lhs, Rhs, ==, &&, __metal_oper_equal)

#define METAL_STATIC_ASSERT_EQUAL(Lhs, Rhs) METAL_STATIC_ASSERT(Lhs == Rhs, #Lhs " == " #Rhs)
#define METAL_STATIC_ASSERT_EQUAL_BITWISE(Lhs, Rhs) METAL_STATIC_ASSERT(METAL_BITWISE_EXPR(Rhs, Lhs, ==, &&), " [bitwise] " #Rhs " == " #Lhs)

#define METAL_ASSERT_NOT_EQUAL(Lhs, Rhs) __metal_unit_impl(__metal_level_assert, __metal_oper_not_equal, Lhs != Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_EXPECT_NOT_EQUAL(Lhs, Rhs) __metal_unit_impl(__metal_level_expect, __metal_oper_not_equal, Lhs != Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_ASSERT_NOT_EQUAL_BITWISE(Lhs, Rhs) METAL_BITWISE(__metal_level_assert, Lhs, Rhs, != , ||, __metal_oper_not_equal);
#define METAL_EXPECT_NOT_EQUAL_BITWISE(Lhs, Rhs) METAL_BITWISE(__metal_level_expect, Lhs, Rhs, != , ||, __metal_oper_not_equal);

#define METAL_STATIC_ASSERT_NOT_EQUAL(Lhs, Rhs) METAL_STATIC_ASSERT(Rhs != Lhs, #Lhs " != " #Rhs)
#define METAL_STATIC_ASSERT_NOT_EQUAL_BITWISE(Lhs, Rhs) METAL_STATIC_ASSERT(METAL_BITWISE_EXPR(Lhs, Rhs, !=, ||), " [bitwise] " #Lhs " != " #Rhs)

#define METAL_ASSERT_CLOSE(Lhs, Rhs, Tolerance) __metal_unit_impl(__metal_level_assert, __metal_oper_close, (Rhs <= (Lhs + Tolerance)) && (Rhs >= (Lhs - Tolerance)), 0, #Rhs, #Lhs, #Tolerance, __FILE__, __LINE__);
#define METAL_EXPECT_CLOSE(Lhs, Rhs, Tolerance) __metal_unit_impl(__metal_level_expect, __metal_oper_close, (Rhs <= (Lhs + Tolerance)) && (Rhs >= (Lhs - Tolerance)), 0, #Rhs, #Lhs, #Tolerance, __FILE__, __LINE__);
#define METAL_STATIC_ASSERT_CLOSE(Lhs, Rhs, Tolerance) \
    METAL_STATIC_ASSERT((Lhs <= (Rhs + Tolerance)) && (Lhs >= (Rhs - Tolerance)) , #Lhs " == " #Rhs " +/- " #Tolerance)

#define METAL_ASSERT_CLOSE_RELATIVE(Lhs, Rhs, Tolerance) __metal_unit_impl(__metal_level_assert, __metal_oper_close_rel, (Rhs <= (Lhs * (1. + Tolerance))) && (Rhs >= (Lhs * (1. - Tolerance))), 0, #Rhs, #Lhs, #Tolerance, __FILE__, __LINE__);
#define METAL_EXPECT_CLOSE_RELATIVE(Lhs, Rhs, Tolerance) __metal_unit_impl(__metal_level_expect, __metal_oper_close_rel, (Rhs <= (Lhs * (1. + Tolerance))) && (Rhs >= (Lhs * (1. - Tolerance))), 0, #Rhs, #Lhs, #Tolerance, __FILE__, __LINE__);
#define METAL_STATIC_ASSERT_CLOSE_RELATIVE(Lhs, Rhs, Tolerance) \
    METAL_STATIC_ASSERT((Lhs <= (Rhs * (1. + Tolerance))) && (Lhs >= (Rhs * (1. - Tolerance))) , #Lhs " == " #Rhs " +/- " #Tolerance " ~")

#define METAL_ASSERT_CLOSE_PERCENT(Lhs, Rhs, Tolerance) __metal_unit_impl(__metal_level_assert, __metal_oper_close_perc, (Rhs <= (Lhs * (1. + ( Tolerance / 100.)))) && (Rhs >= (Lhs * (1. - (Tolerance / 100.)))), 0, #Rhs, #Lhs, #Tolerance, __FILE__, __LINE__);
#define METAL_EXPECT_CLOSE_PERCENT(Lhs, Rhs, Tolerance) __metal_unit_impl(__metal_level_expect, __metal_oper_close_perc, (Rhs <= (Lhs * (1. + ( Tolerance / 100.)))) && (Rhs >= (Lhs * (1. - (Tolerance / 100.)))), 0, #Rhs, #Lhs, #Tolerance, __FILE__, __LINE__);
#define METAL_STATIC_ASSERT_CLOSE_PERCENT(Lhs, Rhs, Tolerance) \
    METAL_STATIC_ASSERT((Rhs <= (Lhs * (1. + (Tolerance / 100.)))) && (Rhs >= (Lhs * (1. - ( Tolerance / 100.)))) , #Lhs " == " #Rhs " +/- " #Tolerance "%")

#define METAL_ASSERT_GE(Lhs, Rhs) __metal_unit_impl(__metal_level_assert, __metal_oper_ge, Lhs >= Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_EXPECT_GE(Lhs, Rhs) __metal_unit_impl(__metal_level_expect, __metal_oper_ge, Lhs >= Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_ASSERT_GE_BITWISE(Lhs, Rhs) METAL_BITWISE(__metal_level_assert, Lhs, Rhs, >=, &&, __metal_oper_ge)
#define METAL_EXPECT_GE_BITWISE(Lhs, Rhs) METAL_BITWISE(__metal_level_expect, Lhs, Rhs, >=, &&, __metal_oper_ge)
#define METAL_STATIC_ASSERT_GE(Lhs, Rhs) METAL_STATIC_ASSERT(Lhs >= Rhs, #Lhs " >= " #Rhs)
#define METAL_STATIC_ASSERT_GE_BITWISE(Lhs, Rhs) METAL_STATIC_ASSERT(METAL_BITWISE_EXPR(Lhs, Rhs, >=, &&), " [bitwise] " #Lhs " >= " #Rhs)

#define METAL_ASSERT_LE(Lhs, Rhs) __metal_unit_impl(__metal_level_assert, __metal_oper_le, Lhs <= Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_EXPECT_LE(Lhs, Rhs) __metal_unit_impl(__metal_level_expect, __metal_oper_le, Lhs <= Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_ASSERT_LE_BITWISE(Lhs, Rhs) METAL_BITWISE(__metal_level_assert, Lhs, Rhs, <=, &&, __metal_oper_le)
#define METAL_EXPECT_LE_BITWISE(Lhs, Rhs) METAL_BITWISE(__metal_level_expect, Lhs, Rhs, <=, &&, __metal_oper_le)
#define METAL_STATIC_ASSERT_LE(Lhs, Rhs) METAL_STATIC_ASSERT(Lhs <= Rhs, #Lhs " <= " #Rhs)
#define METAL_STATIC_ASSERT_LE_BITWISE(Lhs, Rhs) METAL_STATIC_ASSERT(METAL_BITWISE_EXPR(Lhs, Rhs, <=, &&), " [bitwise] " #Lhs " <= " #Rhs)

#define METAL_ASSERT_GREATER(Lhs, Rhs) __metal_unit_impl(__metal_level_assert, __metal_oper_greater, Lhs > Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_EXPECT_GREATER(Lhs, Rhs) __metal_unit_impl(__metal_level_expect, __metal_oper_greater, Lhs > Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_STATIC_ASSERT_GREATER(Lhs, Rhs) METAL_STATIC_ASSERT(Lhs > Rhs, #Lhs " > " #Rhs)

#define METAL_ASSERT_LESSER(Lhs, Rhs) __metal_unit_impl(__metal_level_assert, __metal_oper_lesser, Lhs < Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_EXPECT_LESSER(Lhs, Rhs) __metal_unit_impl(__metal_level_expect, __metal_oper_lesser, Lhs < Rhs, 0, #Lhs, #Rhs, 0, __FILE__, __LINE__);
#define METAL_STATIC_ASSERT_LESSER(Lhs, Rhs) METAL_STATIC_ASSERT(Lhs < Rhs, #Lhs " < " #Rhs)


#define METAL_ASSERT_EQUAL_RANGED(Lhs, LhsSize, Rhs, RhsSize) METAL_RANGED(__metal_level_assert, Lhs, LhsSize, Rhs, RhsSize, METAL_ASSERT_EQUAL(Lhs[i], Rhs[i]))
#define METAL_EXPECT_EQUAL_RANGED(Lhs, LhsSize, Rhs, RhsSize) METAL_RANGED(__metal_level_expect, Lhs, LhsSize, Rhs, RhsSize, METAL_EXPECT_EQUAL(Lhs[i], Rhs[i]))
#define METAL_ASSERT_EQUAL_BITWISE_RANGED(Lhs, LhsSize, Rhs, RhsSize) METAL_RANGED(__metal_level_assert, Lhs, LhsSize, Rhs, RhsSize, METAL_ASSERT_EQUAL_BITWISE(Lhs[i], Rhs[i]))
#define METAL_EXPECT_EQUAL_BITWISE_RANGED(Lhs, LhsSize, Rhs, RhsSize) METAL_RANGED(__metal_level_expect, Lhs, LhsSize, Rhs, RhsSize, METAL_EXPECT_EQUAL_BITWISE(Lhs[i], Rhs[i]))

#define METAL_ASSERT_NOT_EQUAL_RANGED(Lhs, LhsSize, Rhs, RhsSize) METAL_RANGED(__metal_level_assert, Lhs, LhsSize, Rhs, RhsSize, METAL_ASSERT_NOT_EQUAL(Lhs[i], Rhs[i]))
#define METAL_EXPECT_NOT_EQUAL_RANGED(Lhs, LhsSize, Rhs, RhsSize) METAL_RANGED(__metal_level_expect, Lhs, LhsSize, Rhs, RhsSize, METAL_EXPECT_NOT_EQUAL(Lhs[i], Rhs[i]))
#define METAL_ASSERT_NOT_EQUAL_BITWISE_RANGED(Lhs, LhsSize, Rhs, RhsSize) METAL_RANGED(__metal_level_assert, Lhs, LhsSize, Rhs, RhsSize, METAL_ASSERT_NOT_EQUAL_BITWISE(Lhs[i], Rhs[i]))
#define METAL_EXPECT_NOT_EQUAL_BITWISE_RANGED(Lhs, LhsSize, Rhs, RhsSize) METAL_RANGED(__metal_level_expect, Lhs, LhsSize, Rhs, RhsSize, METAL_EXPECT_NOT_EQUAL_BITWISE(Lhs[i], Rhs[i]))

#define METAL_ASSERT_CLOSE_RANGED(Lhs, LhsSize, Rhs, RhsSize, Tolerance) METAL_RANGED(__metal_level_assert, Lhs, LhsSize, Rhs, RhsSize, METAL_ASSERT_CLOSE(Lhs[i], Rhs[i], Tolerance))
#define METAL_EXPECT_CLOSE_RANGED(Lhs, LhsSize, Rhs, RhsSize, Tolerance) METAL_RANGED(__metal_level_expect, Lhs, LhsSize, Rhs, RhsSize, METAL_EXPECT_CLOSE(Lhs[i], Rhs[i], Tolerance))

#define METAL_ASSERT_CLOSE_RELATIVE_RANGED(Lhs, LhsSize, Rhs, RhsSize, Tolerance) METAL_RANGED(__metal_level_assert, Lhs, LhsSize, Rhs, RhsSize, METAL_ASSERT_CLOSE_RELATIVE(Lhs[i], Rhs[i], Tolerance))
#define METAL_EXPECT_CLOSE_RELATIVE_RANGED(Lhs, LhsSize, Rhs, RhsSize, Tolerance) METAL_RANGED(__metal_level_expect, Lhs, LhsSize, Rhs, RhsSize, METAL_EXPECT_CLOSE_RELATIVE(Lhs[i], Rhs[i], Tolerance))

#define METAL_ASSERT_CLOSE_PERCENT_RANGED(Lhs, LhsSize, Rhs, RhsSize, Tolerance) METAL_RANGED(__metal_level_assert, Lhs, LhsSize, Rhs, RhsSize, METAL_ASSERT_CLOSE_PERCENT(Lhs[i], Rhs[i], Tolerance))
#define METAL_EXPECT_CLOSE_PERCENT_RANGED(Lhs, LhsSize, Rhs, RhsSize, Tolerance) METAL_RANGED(__metal_level_expect, Lhs, LhsSize, Rhs, RhsSize, METAL_EXPECT_CLOSE_PERCENT(Lhs[i], Rhs[i], Tolerance))

#define METAL_ASSERT_GE_RANGED(Lhs, LhsSize, Rhs, RhsSize) METAL_RANGED(__metal_level_assert, Lhs, LhsSize, Rhs, RhsSize, METAL_ASSERT_GE(Lhs[i], Rhs[i]))
#define METAL_EXPECT_GE_RANGED(Lhs, LhsSize, Rhs, RhsSize) METAL_RANGED(__metal_level_expect, Lhs, LhsSize, Rhs, RhsSize, METAL_EXPECT_GE(Lhs[i], Rhs[i]))
#define METAL_ASSERT_GE_BITWISE_RANGED(Lhs, LhsSize, Rhs, RhsSize) METAL_RANGED(__metal_level_assert, Lhs, LhsSize, Rhs, RhsSize, METAL_ASSERT_GE_BITWISE(Lhs[i], Rhs[i]))
#define METAL_EXPECT_GE_BITWISE_RANGED(Lhs, LhsSize, Rhs, RhsSize) METAL_RANGED(__metal_level_expect, Lhs, LhsSize, Rhs, RhsSize, METAL_EXPECT_GE_BITWISE(Lhs[i], Rhs[i]))

#define METAL_ASSERT_LE_RANGED(Lhs, LhsSize, Rhs, RhsSize) METAL_RANGED(__metal_level_assert, Lhs, LhsSize, Rhs, RhsSize, METAL_ASSERT_LE(Lhs[i], Rhs[i]))
#define METAL_EXPECT_LE_RANGED(Lhs, LhsSize, Rhs, RhsSize) METAL_RANGED(__metal_level_expect, Lhs, LhsSize, Rhs, RhsSize, METAL_EXPECT_LE(Lhs[i], Rhs[i]))
#define METAL_ASSERT_LE_BITWISE_RANGED(Lhs, LhsSize, Rhs, RhsSize) METAL_RANGED(__metal_level_assert, Lhs, LhsSize, Rhs, RhsSize, METAL_ASSERT_LE_BITWISE(Lhs[i], Rhs[i]))
#define METAL_EXPECT_LE_BITWISE_RANGED(Lhs, LhsSize, Rhs, RhsSize) METAL_RANGED(__metal_level_expect, Lhs, LhsSize, Rhs, RhsSize, METAL_EXPECT_LE_BITWISE(Lhs[i], Rhs[i]))

#define METAL_ASSERT_GREATER_RANGED(Lhs, LhsSize, Rhs, RhsSize) METAL_RANGED(__metal_level_assert, Lhs, LhsSize, Rhs, RhsSize, METAL_ASSERT_GREATER(Lhs[i], Rhs[i]))
#define METAL_EXPECT_GREATER_RANGED(Lhs, LhsSize, Rhs, RhsSize) METAL_RANGED(__metal_level_expect, Lhs, LhsSize, Rhs, RhsSize, METAL_EXPECT_GREATER(Lhs[i], Rhs[i]))

#define METAL_ASSERT_LESSER_RANGED(Lhs, LhsSize, Rhs, RhsSize) METAL_RANGED(__metal_level_assert, Lhs, LhsSize, Rhs, RhsSize, METAL_ASSERT_LESSER(Lhs[i], Rhs[i]))
#define METAL_EXPECT_LESSER_RANGED(Lhs, LhsSize, Rhs, RhsSize) METAL_RANGED(__metal_level_expect, Lhs, LhsSize, Rhs, RhsSize, METAL_EXPECT_LESSER(Lhs[i], Rhs[i]))

#define METAL_ASSERT_NO_EXECUTE() __metal_unit_impl(__metal_level_assert, __metal_oper_no_exec, 0, 0, "unexpected execution", 0, 0, __FILE__, __LINE__);
#define METAL_EXPECT_NO_EXECUTE() __metal_unit_impl(__metal_level_expect, __metal_oper_no_exec, 0, 0, "unexpected execution", 0, 0, __FILE__, __LINE__);

#define METAL_ASSERT_EXECUTE() __metal_unit_impl(__metal_level_assert, __metal_oper_exec, 1, 0, "expected execution", 0, 0, __FILE__, __LINE__);
#define METAL_EXPECT_EXECUTE() __metal_unit_impl(__metal_level_expect, __metal_oper_exec, 1, 0, "expected execution", 0, 0, __FILE__, __LINE__);


#define METAL_CRITICAL(Check)  __metal_critical ++; Check ; __metal_critical--;
#define METAL_ENTER_CRITICAL() __metal_critical ++;
#define METAL_EXIT_CRITICAL()  __metal_critical --;


#if !defined (METAL_NO_IMPLEMENT_INCLUDE)
#include <metal/unit.ipp>
#endif

#endif /* METAL_TEST_UNIT_H_ */
