/**
 * @file   test/test.cpp
 * @date   30.07.2016
 * @author Klemens D. Morgenstern
 *



 */

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>

#define METAL_NO_IMPLEMENT_INCLUDE
#include <metal/unit.hpp>

#include <vector>

int __metal_status   = 1;
int __metal_critical = 0;
int __metal_errored = 0;

__metal_level lvl;
__metal_oper oper;
int condition;
int bitwise;
const char* str1;
const char* str2;
const char* str3;
const char* file;
int line;


void __metal_impl(__metal_level lvl,
               __metal_oper oper,
               int condition,
               int bitwise,
               const char* str1,
               const char* str2,
               const char* str3,
               const char* file,
               int line)
{
    ::lvl       = lvl      ;
    ::oper      = oper     ;
    ::condition = condition;
    ::bitwise   = bitwise  ;
    ::str1      = str1     ;
    ::str2      = str2     ;
    ::str3      = str3     ;
    ::file      = file     ;
    ::line      = line     ;

    __metal_status = condition;
}

BOOST_AUTO_TEST_CASE(util)
{
    METAL_LOG("My Message"); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK_EQUAL(::str1, "My Message");
    BOOST_CHECK_EQUAL(lvl, __metal_level_expect);

    METAL_CHECKPOINT(); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK_EQUAL(lvl, __metal_level_expect);

    METAL_ASSERT_MESSAGE(true, "Message"); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK_EQUAL(::str1, "Message");
    BOOST_CHECK_EQUAL(lvl, __metal_level_assert);

    METAL_EXPECT_MESSAGE(true, "Message 2"); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK_EQUAL(::str1, "Message 2");
    BOOST_CHECK_EQUAL(lvl, __metal_level_expect);

}

BOOST_AUTO_TEST_CASE(plain)
{
    METAL_ASSERT(true); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK_EQUAL(::str1, "true");
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(lvl, __metal_level_assert);

    METAL_EXPECT(false); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK_EQUAL(::str1, "false");
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(lvl, __metal_level_expect);
}

BOOST_AUTO_TEST_CASE(equal)
{
    METAL_ASSERT_EQUAL(12,12); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK_EQUAL(::str1, "12");
    BOOST_CHECK_EQUAL(::str2, "12");
    BOOST_CHECK(!::bitwise);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(lvl, __metal_level_assert);

    METAL_EXPECT_EQUAL(1,2); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK_EQUAL(::str1, "1");
    BOOST_CHECK_EQUAL(::str2, "2");
    BOOST_CHECK(!::bitwise);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(lvl, __metal_level_expect);

    METAL_ASSERT_NOT_EQUAL(3,3); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK_EQUAL(::str1, "3");
    BOOST_CHECK_EQUAL(::str2, "3");
    BOOST_CHECK(!::bitwise);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(lvl, __metal_level_assert);

    METAL_EXPECT_NOT_EQUAL(5,6); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK_EQUAL(::str1, "5");
    BOOST_CHECK_EQUAL(::str2, "6");
    BOOST_CHECK(!::bitwise);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(lvl, __metal_level_expect);
}

BOOST_AUTO_TEST_CASE(bitwise_equal)
{
    METAL_ASSERT_EQUAL_BITWISE(12,12); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK_EQUAL(::str1, "12");
    BOOST_CHECK_EQUAL(::str2, "12");
    BOOST_CHECK(::bitwise);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(lvl, __metal_level_assert);

    METAL_EXPECT_EQUAL_BITWISE(1,2); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK_EQUAL(::str1, "1");
    BOOST_CHECK_EQUAL(::str2, "2");
    BOOST_CHECK(::bitwise);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(lvl, __metal_level_expect);

    METAL_ASSERT_NOT_EQUAL_BITWISE(3,3); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK_EQUAL(::str1, "3");
    BOOST_CHECK_EQUAL(::str2, "3");
    BOOST_CHECK(::bitwise);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(lvl, __metal_level_assert);

    METAL_EXPECT_NOT_EQUAL_BITWISE(5,6); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK_EQUAL(::str1, "5");
    BOOST_CHECK_EQUAL(::str2, "6");
    BOOST_CHECK(::bitwise);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(lvl, __metal_level_expect);
}

//ranged

BOOST_AUTO_TEST_CASE(equal_ranged)
{
    std::vector<int> v = { 1,2,3,4,5};
    auto v2 = v;
    METAL_ASSERT_EQUAL_RANGED(v.begin(), v.end(), v2.begin(), v2.end()); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(oper, __metal_oper_exit_ranged);

    METAL_ASSERT_NOT_EQUAL_RANGED(v.begin(), v.end(), v2.begin(), v2.end()); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::bitwise);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(oper, __metal_oper_exit_ranged);

    v2.resize(4);

    METAL_EXPECT_EQUAL_BITWISE_RANGED(v.begin(), v.end(), v2.begin(), v2.end()); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(oper, __metal_oper_exit_ranged);

    METAL_EXPECT_NOT_EQUAL_BITWISE_RANGED(v.begin(), v.end(), v2.begin(), v2.end()); BOOST_CHECK_EQUAL(::line, __LINE__);

    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(oper, __metal_oper_exit_ranged);

}

BOOST_AUTO_TEST_CASE(bitwise_equal_ranged)
{
    METAL_ASSERT_EQUAL_BITWISE(12,12); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK_EQUAL(::str1, "12");
    BOOST_CHECK_EQUAL(::str2, "12");
    BOOST_CHECK(::bitwise);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(lvl, __metal_level_assert);

    METAL_EXPECT_EQUAL_BITWISE(1,2); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK_EQUAL(::str1, "1");
    BOOST_CHECK_EQUAL(::str2, "2");
    BOOST_CHECK(::bitwise);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(lvl, __metal_level_expect);

    METAL_ASSERT_NOT_EQUAL_BITWISE(3,3); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK_EQUAL(::str1, "3");
    BOOST_CHECK_EQUAL(::str2, "3");
    BOOST_CHECK(::bitwise);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(lvl, __metal_level_assert);

    METAL_EXPECT_NOT_EQUAL_BITWISE(0xDEADBEEF, ~0xDEADBEEF); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK_EQUAL(::str1,  "0xDEADBEEF");
    BOOST_CHECK_EQUAL(::str2, "~0xDEADBEEF");
    BOOST_CHECK(::bitwise);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(lvl, __metal_level_expect);
}


BOOST_AUTO_TEST_CASE(close_absolute)
{
    METAL_ASSERT_CLOSE(0.2, 0.5, 0.1); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_close);

    METAL_EXPECT_CLOSE(0.2, 0.1, 0.1); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_close);

    auto v1 = {1.0, 3.0};
    auto v2 = {1.5, 2.5};

    METAL_ASSERT_CLOSE_RANGED(v1.begin(), v1.end(), v2.begin(), v2.end(), 0.5); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exit_ranged);

    METAL_EXPECT_CLOSE_RANGED(v1.begin(), v1.end(), v2.begin(), v2.end(), 0.25); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exit_ranged);
}

BOOST_AUTO_TEST_CASE(close_relative)
{
    METAL_ASSERT_CLOSE_RELATIVE(0.2, 0.5, 0.1); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_close_rel);

    METAL_EXPECT_CLOSE_RELATIVE(0.2, 0.1, 1); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_close_rel);

    auto v1 = {1.0, 3.0};
    auto v2 = {1.5, 1.5};

    METAL_ASSERT_CLOSE_RELATIVE_RANGED(v1.begin(), v1.end(), v2.begin(), v2.end(), 0.5); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exit_ranged);

    METAL_EXPECT_CLOSE_RELATIVE_RANGED(v1.begin(), v1.end(), v2.begin(), v2.end(), 0.05); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exit_ranged);
}

BOOST_AUTO_TEST_CASE(close_percent)
{
    METAL_ASSERT_CLOSE_PERCENT(0.2, 0.5, 0.1); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_close_perc);

    METAL_EXPECT_CLOSE_PERCENT(0.2, 0.1, 10); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_close_perc);

    auto v1 = {1.0, 3.0};
    auto v2 = {1.5, 1.5};

    METAL_ASSERT_CLOSE_PERCENT_RANGED(v1.begin(), v1.end(), v2.begin(), v2.end(), 50); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exit_ranged);

    METAL_EXPECT_CLOSE_PERCENT_RANGED(v1.begin(), v1.end(), v2.begin(), v2.end(), 5); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exit_ranged);
}

BOOST_AUTO_TEST_CASE(ge)
{
    METAL_ASSERT_GE(0.2, 0.5); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_ge);

    METAL_EXPECT_GE(0.2, 0.1); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_ge);

    std::vector<double> v1 = {1.5, 3.0};
    std::vector<double> v2 = {1.5, 2.5};

    METAL_ASSERT_GE_RANGED(v1.begin(), v1.end(), v2.begin(), v2.end()); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exit_ranged);

    v1[0] = 0.1;

    METAL_EXPECT_GE_RANGED(v1.begin(), v1.end(), v2.begin(), v2.end()); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exit_ranged);
}

BOOST_AUTO_TEST_CASE(ge_bitwise)
{
    METAL_ASSERT_GE_BITWISE(3, 4); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_ge);

    METAL_EXPECT_GE_BITWISE(-1, 0); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_ge);

    std::vector<int> v1 = {5, 3};
    std::vector<int> v2 = {5, 1};

    METAL_ASSERT_GE_BITWISE_RANGED(v1.begin(), v1.end(), v2.begin(), v2.end()); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exit_ranged);

    v2[1] = 4;

    METAL_EXPECT_GE_BITWISE_RANGED(v1.begin(), v1.end(), v2.begin(), v2.end()); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exit_ranged);
}

BOOST_AUTO_TEST_CASE(le)
{
    METAL_ASSERT_LE(0.6, 0.5); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_le);

    METAL_EXPECT_LE(0.1, 0.1); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_le);

    std::vector<double> v1 = {1.5, 3.5};
    std::vector<double> v2 = {1.5, 3.0};

    METAL_ASSERT_LE_RANGED(v1.begin(), v1.end(), v2.begin(), v2.end()); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exit_ranged);

    v2[1] = 10.;

    METAL_EXPECT_LE_RANGED(v1.begin(), v1.end(), v2.begin(), v2.end()); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exit_ranged);
}

BOOST_AUTO_TEST_CASE(le_bitwise)
{
    METAL_ASSERT_LE_BITWISE(3, 4); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_le);

    METAL_EXPECT_LE_BITWISE(0, -10); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_le);

    std::vector<int> v1 = {5, 1};
    std::vector<int> v2 = {5, 3};

    METAL_ASSERT_LE_BITWISE_RANGED(v1.begin(), v1.end(), v2.begin(), v2.end()); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exit_ranged);

    v2[1] = 4;

    METAL_EXPECT_LE_BITWISE_RANGED(v1.begin(), v1.end(), v2.begin(), v2.end()); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exit_ranged);
}


BOOST_AUTO_TEST_CASE(greater)
{
    METAL_ASSERT_GREATER(0.5, 0.5); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_greater);

    METAL_EXPECT_GREATER(0.2, 0.1); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_greater);

    std::vector<double> v1 = {1.6, 3.5};
    std::vector<double> v2 = {1.5, 3.0};

    METAL_ASSERT_GREATER_RANGED(v1.begin(), v1.end(), v2.begin(), v2.end()); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exit_ranged);

    v2[1] = 10.;

    METAL_EXPECT_GREATER_RANGED(v1.begin(), v1.end(), v2.begin(), v2.end()); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exit_ranged);
}

BOOST_AUTO_TEST_CASE(lesser)
{
    METAL_ASSERT_LESSER(0.6, 0.5); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_lesser);

    METAL_EXPECT_LESSER(0.1, 0.2); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_lesser);

    std::vector<double> v1 = {1.4, 3.5};
    std::vector<double> v2 = {1.5, 3.0};

    METAL_ASSERT_LESSER_RANGED(v1.begin(), v1.end(), v2.begin(), v2.end()); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exit_ranged);

    v2[1] = 10.;

    METAL_EXPECT_LESSER_RANGED(v1.begin(), v1.end(), v2.begin(), v2.end()); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exit_ranged);
}


BOOST_AUTO_TEST_CASE(exception)
{
    METAL_ASSERT_NO_THROW(    ); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_no_exception);

    METAL_EXPECT_NO_THROW(throw 42); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_no_exception);

    METAL_ASSERT_ANY_THROW(    ); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_any_exception);

    METAL_EXPECT_ANY_THROW(throw 42); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_any_exception);

    METAL_ASSERT_THROW(throw 42, double); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exception);

    METAL_EXPECT_THROW(throw 42, int, double); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exception);

}

BOOST_AUTO_TEST_CASE(execute)
{
    METAL_ASSERT_EXECUTE(); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exec);
    METAL_EXPECT_EXECUTE(); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_exec);

    METAL_ASSERT_NO_EXECUTE(); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_assert);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_no_exec);
    METAL_EXPECT_NO_EXECUTE(); BOOST_CHECK_EQUAL(::line, __LINE__);
    BOOST_CHECK(!::condition);
    BOOST_CHECK_EQUAL(::lvl, __metal_level_expect);
    BOOST_CHECK_EQUAL(::oper, __metal_oper_no_exec);
}

BOOST_AUTO_TEST_CASE(critical)
{
    BOOST_CHECK(!METAL_IS_CRITICAL());

    METAL_ENTER_CRITICAL();
    BOOST_CHECK(METAL_IS_CRITICAL());
    METAL_EXIT_CRITICAL();

    BOOST_CHECK(!METAL_IS_CRITICAL());

    METAL_CRITICAL(BOOST_CHECK(METAL_IS_CRITICAL()));

}

