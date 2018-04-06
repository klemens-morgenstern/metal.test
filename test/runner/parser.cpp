/**
 * @file   /gdb-runner/test/parser.cpp
 * @date   23.06.2016
 * @author Klemens D. Morgenstern
 *



 */

#include <boost/optional/optional_io.hpp>
#include <metal/gdb/mi2/output.hpp>
#include <boost/variant/get.hpp>

#define BOOST_TEST_MODULE parser_test
#define BOOST_TEST_NO_LIB

#include <boost/test/included/unit_test.hpp>
#include <string>

namespace mi2 = metal::gdb::mi2;

BOOST_AUTO_TEST_CASE(stream_output)
{
    auto res = mi2::parse_stream_output("~\"stuff\"");

    BOOST_CHECK(res);
    BOOST_CHECK_EQUAL(res->content, "stuff");
    BOOST_CHECK_EQUAL(res->type, mi2::stream_record::console);

    res = mi2::parse_stream_output("@\"other stuff\"");

    BOOST_CHECK(res);
    BOOST_CHECK_EQUAL(res->content, "other stuff");
    BOOST_CHECK_EQUAL(res->type, mi2::stream_record::target);

    res = mi2::parse_stream_output("&\"thingy\"");


    BOOST_CHECK(res);
    BOOST_CHECK_EQUAL(res->content, "thingy");
    BOOST_CHECK_EQUAL(res->type, mi2::stream_record::log);

    res = mi2::parse_stream_output("&\"thingy");

    BOOST_CHECK(!res);

    res = mi2::parse_stream_output("|\"other stuff\"");

    BOOST_CHECK(!res);
}

BOOST_AUTO_TEST_CASE(async_output)
{


    auto check_res = [&](const mi2::async_output & aso)
              {
                BOOST_CHECK_EQUAL(aso.class_, "stopped");

                auto & results = aso.results;
                BOOST_REQUIRE_EQUAL(results.size(), 2u);
                BOOST_CHECK_EQUAL(results[0].variable, "id");

                BOOST_REQUIRE(results[0].value_.type() == boost::typeindex::type_id<std::string>());
                BOOST_CHECK_EQUAL(boost::get<std::string>(results[0].value_), "id");


                BOOST_CHECK_EQUAL(results[1].variable, "group-id");

                BOOST_REQUIRE(results[1].value_.type() == boost::typeindex::type_id<std::string>());
                BOOST_CHECK_EQUAL(boost::get<std::string>(results[1].value_), "gid");
              };

    auto res = mi2::parse_async_output("=stopped,id=\"id\",group-id=\"gid\"");
    BOOST_REQUIRE(res);
    BOOST_CHECK(!res->first);

    check_res(res->second);

    auto r = mi2::parse_async_output(1234u, "1234=stopped,id=\"id\",group-id=\"gid\"");

    BOOST_REQUIRE(r);
    check_res(*r);

    res = mi2::parse_async_output("4321=stopped,id=\"id\",group-id=\"gid\"");

    BOOST_REQUIRE(res);
    check_res(res->second);
    BOOST_REQUIRE(res->first);
    BOOST_CHECK_EQUAL(*res->first, 4321u);
}

BOOST_AUTO_TEST_CASE(sync_output)
{
    auto res = mi2::parse_record(42,
                          "42^done,"
                               "bkpt={"
                               "number=\"1\","
                               "type=\"breakpoint\","
                               "disp=\"keep\","
                               "enabled=\"y\","
                               "addr=\"0x08048564\","
                               "func=\"main\","
                               "file=\"myprog.c\","
                               "fullname=\"/home/nickrob/myprog.c\","
                               "line=\"68\","
                               "thread-groups=[\"i1\"],"
                               "times=\"0\"}");

    BOOST_REQUIRE(res);
    BOOST_CHECK(res->class_ == mi2::result_class::done);

    auto & r = res->results;

    BOOST_REQUIRE_EQUAL(r.size(), 1u);
    BOOST_CHECK_EQUAL(r[0].variable, "bkpt");
    BOOST_REQUIRE(r[0].value_.type() == boost::typeindex::type_id<mi2::tuple>());

    auto & tup = boost::get<mi2::tuple>(r[0].value_);
    BOOST_REQUIRE_EQUAL(tup.size(), 11u);

    BOOST_CHECK_EQUAL(tup[0] .variable, "number");
    BOOST_CHECK_EQUAL(tup[1] .variable, "type");
    BOOST_CHECK_EQUAL(tup[2] .variable, "disp");
    BOOST_CHECK_EQUAL(tup[3] .variable, "enabled");
    BOOST_CHECK_EQUAL(tup[4] .variable, "addr");
    BOOST_CHECK_EQUAL(tup[5] .variable, "func");
    BOOST_CHECK_EQUAL(tup[6] .variable, "file");
    BOOST_CHECK_EQUAL(tup[7] .variable, "fullname");
    BOOST_CHECK_EQUAL(tup[8] .variable, "line");
    BOOST_CHECK_EQUAL(tup[10].variable, "times");

    auto as_string = [](const mi2::result & val)
        {
            BOOST_REQUIRE(val.value_.type() == boost::typeindex::type_id<std::string>());
            return boost::get<std::string>(val.value_);
        };

    BOOST_CHECK_EQUAL(as_string(tup[0] ), "1");
    BOOST_CHECK_EQUAL(as_string(tup[1] ), "breakpoint");
    BOOST_CHECK_EQUAL(as_string(tup[2] ), "keep");
    BOOST_CHECK_EQUAL(as_string(tup[3] ), "y");
    BOOST_CHECK_EQUAL(as_string(tup[4] ), "0x08048564");
    BOOST_CHECK_EQUAL(as_string(tup[5] ), "main");
    BOOST_CHECK_EQUAL(as_string(tup[6] ), "myprog.c");
    BOOST_CHECK_EQUAL(as_string(tup[7] ), "/home/nickrob/myprog.c");
    BOOST_CHECK_EQUAL(as_string(tup[8] ), "68");
    BOOST_CHECK_EQUAL(as_string(tup[10]), "0");

    BOOST_REQUIRE(tup[9].value_.type() == boost::typeindex::type_id<mi2::list>());
    auto &l = boost::get<mi2::list>(tup[9].value_);

    BOOST_REQUIRE(l.type() == boost::typeindex::type_id<std::vector<mi2::value>>());

    auto v = boost::get<std::vector<mi2::value>>(l);
    BOOST_REQUIRE_EQUAL(v.size(), 1u);

    BOOST_REQUIRE(v[0].type() == boost::typeindex::type_id<std::string>());

    BOOST_CHECK_EQUAL(boost::get<std::string>(v[0]), "i1");

}

BOOST_AUTO_TEST_CASE(sync_output2)
{
    auto str = R"__(1^done,bkpt={number="1",type="breakpoint",disp="keep",enabled="y",addr="0x0000000000401608",func="main()",file="target.cpp",fullname="F:\\mwspace\\test\\gdb-runner\\test\\target.cpp",line="34",thread-groups=["i1"],times="0",original-location="target.cpp:34"})__";
    auto res = mi2::parse_record(1, str);

    BOOST_CHECK(res);
    BOOST_CHECK(!mi2::parse_record(2, str));
}

BOOST_AUTO_TEST_CASE(sync_output3)
{
    auto str = R"__(2^done,bkpt={number="2",type="breakpoint",disp="keep",enabled="y",addr="<MULTIPLE>",times="0",original-location="f"},{number="2.1",enabled="y",addr="0x00000000004015b8",func="f(int&)",file="target.cpp",fullname="F:\\mwspace\\test\\gdb-runner\\test\\target.cpp",line="17",thread-groups=["i1"]},{number="2.2",enabled="y",addr="0x00000000004015c3",func="f(int*)",file="target.cpp",fullname="F:\\mwspace\\test\\gdb-runner\\test\\target.cpp",line="21",thread-groups=["i1"]},{number="2.3",enabled="y",addr="0x00000000004015ca",func="f()",file="target.cpp",fullname="F:\\mwspace\\test\\gdb-runner\\test\\target.cpp",line="24",thread-groups=["i1"]})__";
    auto res = mi2::parse_record(2, str);

    BOOST_CHECK(res);
    BOOST_CHECK(!mi2::parse_record(1, str));

}
