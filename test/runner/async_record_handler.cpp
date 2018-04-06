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

#include <boost/test/unit_test.hpp>
#include <string>

namespace mi2 = metal::gdb::mi2;

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


}
