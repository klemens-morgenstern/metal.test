/**
 * @file   serial/test_functions.cpp
 * @date   03.05.2018
 * @author Klemens D. Morgenstern
 *
 */

#include "test_functions.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <stack>
#include <boost/optional.hpp>
#include "sink.hpp"

static std::string sink_file;
static std::string format;
static boost::optional<std::ofstream> fstr;
static bool no_exit_code{false};
static std::ostream * sink_str = &std::cout;
static bool state{true};
static data_sink_t *data_sink = nullptr;

struct statistic
{
    int executed = 0;
    int errors   = 0;
    int warnings = 0;
};

static statistic free_tests;
static statistic all_tests;
static std::stack<std::pair<std::string, statistic>> current_test_cases;

static void add_assertion(bool condition)
{
    auto & stat = current_test_cases.empty() ? free_tests : current_test_cases.top().second;
    stat.executed++;
    all_tests.executed++;
    if (!condition)
    {
        all_tests.errors++;
        stat.errors++;
    }
}

static void add_expectation(bool condition)
{
    auto & stat = current_test_cases.empty() ? free_tests : current_test_cases.top().second;
    stat.executed++;
    all_tests.executed++;
    if (!condition)
    {
        all_tests.warnings++;
        stat.warnings++;
    }
}


void metal_serial_assert(metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    auto cond = session.get_bool();
    add_assertion(cond);
    data_sink->plain(file, line, cond, level_t::assertion, args.at(0));
}
void metal_serial_expect           (metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    auto cond = session.get_bool();
    add_expectation(cond);
    data_sink->plain(file, line, cond, level_t::expect, args.at(0));
}
void metal_serial_assert_message   (metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    data_sink->message(file, line, session.get_bool(), level_t::assertion, args.at(1));
}
void metal_serial_expect_message   (metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    data_sink->message(file, line, session.get_bool(), level_t::expect, args.at(1));
}
void metal_serial_call             (metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    bool enter = session.get_bool(); //true == entering, false == leaving
    auto &name = args.back();

    if (enter)
    {
        current_test_cases.emplace(name, statistic{});
        data_sink->enter_case(file, line, name);
    }
    else
    {
        auto & current = current_test_cases.top();
        auto & stat = current.second;
        data_sink->exit_case(file, line, current.first, stat.executed, stat.warnings, stat.errors);
        current_test_cases.pop();
    }
}

void metal_serial_checkpoint       (metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line) { data_sink->checkpoint(file, line); }
void metal_serial_assert_no_execute(metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    add_assertion(false);
    data_sink->no_execute(file, line, level_t::assertion);
}
void metal_serial_expect_no_execute(metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    add_expectation(false);
    data_sink->no_execute(file, line, level_t::expect);
}
void metal_serial_assert_equal     (metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    auto cond = session.get_bool();
    auto lhs = session.get_int();
    auto rhs = session.get_int();
    add_assertion(cond);
    data_sink->equal(file, line, cond, level_t::assertion, std::to_string(lhs), std::to_string(rhs),  args.at(0), args.at(1));
}

void metal_serial_expect_equal     (metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    auto cond = session.get_bool();
    auto lhs = session.get_int();
    auto rhs = session.get_int();
    add_expectation(cond);
    data_sink->equal(file, line, cond, level_t::expect, std::to_string(lhs), std::to_string(rhs),  args.at(0), args.at(1));
}

void metal_serial_assert_not_equal (metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    auto cond = session.get_bool();
    auto lhs = session.get_int();
    auto rhs = session.get_int();
    add_assertion(cond);
    data_sink->not_equal(file, line, cond, level_t::assertion, std::to_string(lhs), std::to_string(rhs),  args.at(0), args.at(1));
}

void metal_serial_expect_not_equal (metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    auto cond = session.get_bool();
    auto lhs = session.get_int();
    auto rhs = session.get_int();
    add_expectation(cond);
    data_sink->not_equal(file, line, cond, level_t::expect, std::to_string(lhs), std::to_string(rhs), args.at(0), args.at(1));
}

void metal_serial_assert_ge        (metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    auto cond = session.get_bool();
    auto lhs = session.get_int();
    auto rhs = session.get_int();
    add_assertion(cond);
    data_sink->ge(file, line, cond, level_t::assertion, std::to_string(lhs), std::to_string(rhs), args.at(0), args.at(1));
}

void metal_serial_expect_ge        (metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    auto cond = session.get_bool();
    auto lhs = session.get_int();
    auto rhs = session.get_int();
    add_expectation(cond);
    data_sink->ge(file, line, cond, level_t::expect, std::to_string(lhs), std::to_string(rhs), args.at(0), args.at(1));
}

void metal_serial_assert_le        (metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    auto cond = session.get_bool();
    auto lhs = session.get_int();
    auto rhs = session.get_int();
    add_assertion(cond);
    data_sink->le(file, line, cond, level_t::assertion, std::to_string(lhs), std::to_string(rhs), args.at(0), args.at(1));
}

void metal_serial_expect_le        (metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    auto cond = session.get_bool();
    auto lhs = session.get_int();
    auto rhs = session.get_int();
    add_expectation(cond);
    data_sink->le(file, line, cond, level_t::expect, std::to_string(lhs), std::to_string(rhs), args.at(0), args.at(1));
}

void metal_serial_assert_greater   (metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    auto cond = session.get_bool();
    auto lhs = session.get_int();
    auto rhs = session.get_int();
    add_assertion(cond);
    data_sink->greater(file, line, cond, level_t::assertion, std::to_string(lhs), std::to_string(rhs), args.at(0), args.at(1));
}

void metal_serial_expect_greater   (metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    auto cond = session.get_bool();
    auto lhs = session.get_int();
    auto rhs = session.get_int();
    add_expectation(cond);
    data_sink->greater(file, line, cond, level_t::expect, std::to_string(lhs), std::to_string(rhs), args.at(0), args.at(1));
}


void metal_serial_assert_lesser    (metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    auto cond = session.get_bool();
    auto lhs = session.get_int();
    auto rhs = session.get_int();
    add_assertion(cond);
    data_sink->lesser(file, line, cond, level_t::assertion, std::to_string(lhs), std::to_string(rhs), args.at(0), args.at(1));
}

void metal_serial_expect_lesser    (metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    auto cond = session.get_bool();
    auto lhs = session.get_int();
    auto rhs = session.get_int();
    add_expectation(cond);
    data_sink->lesser(file, line, cond, level_t::expect, std::to_string(lhs), std::to_string(rhs), args.at(0), args.at(1));
}

void metal_serial_test_exit        (metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    data_sink->report(free_tests.executed, free_tests.errors, free_tests.warnings, all_tests.executed, all_tests.errors, all_tests.warnings);
    session.set_exit(all_tests.errors != 0);
}




void metal_serial_test_setup_entries(std::unordered_map<std::string,metal::serial::plugin_function_t> & map)
{
    if (!sink_file.empty())
    {
        fstr.emplace(sink_file);
        sink_str = &*fstr;
    }
    //ok, we setup the logger
    if (format.empty() || (format == "hrf"))
        data_sink = get_hrf_sink(*sink_str);
    else if (format == "json")
        data_sink = get_json_sink(*sink_str);
    else
        std::cerr << "Unknown format \"" << format << "\"" << std::endl;

    map.emplace("METAL_SERIAL_ASSERT", &metal_serial_assert);
    map.emplace("METAL_SERIAL_EXPECT", &metal_serial_expect);
    map.emplace("METAL_SERIAL_ASSERT_MESSAGE", &metal_serial_assert_message);
    map.emplace("METAL_SERIAL_EXPECT_MESSAGE", &metal_serial_expect_message);
    map.emplace("METAL_SERIAL_CALL", &metal_serial_call);
    map.emplace("METAL_SERIAL_CHECKPOINT", &metal_serial_checkpoint);
    map.emplace("METAL_SERIAL_ASSERT_NO_EXECUTE", &metal_serial_assert_no_execute);
    map.emplace("METAL_SERIAL_EXPECT_NO_EXECUTE", &metal_serial_expect_no_execute);
    map.emplace("METAL_SERIAL_ASSERT_EQUAL", &metal_serial_assert_equal);
    map.emplace("METAL_SERIAL_EXPECT_EQUAL", &metal_serial_expect_equal);
    map.emplace("METAL_SERIAL_ASSERT_NOT_EQUAL", &metal_serial_assert_not_equal);
    map.emplace("METAL_SERIAL_EXPECT_NOT_EQUAL", &metal_serial_expect_not_equal);
    map.emplace("METAL_SERIAL_ASSERT_GE", &metal_serial_assert_ge);
    map.emplace("METAL_SERIAL_EXPECT_GE", &metal_serial_expect_ge);
    map.emplace("METAL_SERIAL_ASSERT_LE", &metal_serial_assert_le);
    map.emplace("METAL_SERIAL_EXPECT_LE", &metal_serial_expect_le);
    map.emplace("METAL_SERIAL_ASSERT_GREATER", &metal_serial_assert_greater);
    map.emplace("METAL_SERIAL_EXPECT_GREATER", &metal_serial_expect_greater);
    map.emplace("METAL_SERIAL_ASSERT_LESSER", &metal_serial_assert_lesser);
    map.emplace("METAL_SERIAL_EXPECT_LESSER", &metal_serial_expect_lesser);
    map.emplace("METAL_SERIAL_TEST_EXIT", &metal_serial_test_exit);



}

void metal_serial_test_setup_options(boost::program_options::options_description & op)
{
    namespace po = boost::program_options;
    op.add_options()
            ("metal-test-no-exit-code", po::bool_switch(&no_exit_code),      "disable exit-code")
            ("metal-test-sink",         po::value<std::string>(&sink_file),  "test data sink")
            ("metal-test-format",       po::value<std::string>(&format),     "format [hrf, json]")
            ;
}
