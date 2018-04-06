/**
 * @file   sink.cpp
 * @date   12.09.2016
 * @author Klemens D. Morgenstern
 *


 */
#include "sink.hpp"
#include <iostream>
#include <boost/optional.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace std;

inline static const char* msg_word(bool cond)
{
    return cond ? static_cast<const char*>(" succeeded") : static_cast<const char*>(" failed") ;
}

inline static const char* bw_str(bool bitwise)
{
    if (bitwise)
        return " bitwise";
    else
        return "";
}

inline static const char* crit_word(bool crit)
{
    return crit ? static_cast<const char*>(" critical") : static_cast<const char*>("") ;
}

inline static const char* descr(level_t lvl)
{
    switch (lvl)
    {
    case level_t::assertion:
        return " assertion";
    case level_t::expect:
        return " expectation";
    default:
        return "";
    }
}

static std::string ranged(int idx, std::string && st)
{
    if (idx == -1)
        return std::move(st);

    return "**range**[" + to_string(idx) + "]";
}

static std::string descr(bool bw, const std::string & str)
{
    if (bw)
        return " [bitwise " + str + "]: ";
    else
        return " [" + str + "]: ";
}

static std::string bitpre(bool bw, const std::string & str)
{
    if (bw)
        return "0b" + str;
    else
        return str;
}

struct hrf_sink_t : data_sink_t
{
    std::ostream * os = &std::cout;

    std::string group{"__metal_call"};

    std::ostream & loc(const std::string & file, int line)
    {
        return *os << boost::replace_all_copy(file, "\\\\", "\\") << '(' << line << ')';
    }

    std::ostream & check(const std::string & file, int line, bool condition, level_t lvl, bool critical)
    {
        return loc(file, line) << crit_word(critical) << descr(lvl) << msg_word(condition);
    }

    virtual ~hrf_sink_t() = default;
    void start() override
    {
        *os << "starting test execution" << std::endl;
    }
    void cancel_func(const std::string & file, int line, const std::string & id, int executed, int warnings, int errors)
    {
        loc(file, line) << " report: cancel test case [" << id << "]: "
                     "{ executed : " << executed <<
                     ", warnings : " << warnings <<
                     ", errors : " << errors << "}" << endl;
    }
    void cancel_main  (const std::string & file, int line, int executed, int warnings, int errors) override
    {
        loc(file, line) << " report: canceling main: "
                         "{ executed : " << executed <<
                         ", warnings : " << warnings <<
                         ", errors : " << errors << "}" << endl;
    }
    void continue_main(const std::string & file, int line, int executed, int warnings, int errors) override
    {
        loc(file, line) << " report: continuing in main: "
                         "{ executed : " << executed <<
                         ", warnings : " << warnings <<
                         ", errors : " << errors << "}" << endl;
    }

    void enter_range (const std::string & file, int line, const std::string & descr) override
    {
        loc(file, line) << " report: entering ranged test [" << descr << "]" << endl;
    }
    void enter_range_mismatch (const std::string & file, int line,
                               const std::string & descr, const std::string & lhs, const std::string& rhs) override
    {
        loc(file, line) << " error entering ranged test [" << descr << "] with mismatch: "
                        << lhs << " != " << rhs << endl;
    }

    void exit_range  (const std::string & file, int line, int executed, int warnings, int errors) override
    {
        loc(file, line) <<  " exiting ranged test: "
                            "{ executed : " << executed <<
                            ", warnings : " << warnings <<
                            ", errors : " << errors << "}" << endl;
    }

    void cancel_case (const std::string & id, int executed, int warnings, int errors) override
    {
        *os << "    "
                "canceling test case ["<<  id <<  "]: "
                          "{ executed : " << executed <<
                          ", warnings : " << warnings <<
                          ", errors : " << errors << "}" << endl;
    }

    void enter_case(const std::string & file, int line, const std::string & id) override
    {
        loc(file, line) << " entering test case ["<<  id <<  "]" << endl;
    }
    void exit_case (const std::string & file, int line, const std::string & id, int executed, int warnings, int errors) override
    {
        loc(file, line) << " exiting test case ["<<  id <<  "]: "
                          "{ executed : " << executed <<
                          ", warnings : " << warnings <<
                          ", errors : " << errors << "}" << endl;
    }
    void report (int free_executed, int free_warnings, int free_errors,
                 int executed, int warnings, int errors) override
    {
        if (free_executed)
            *os << "free tests "
                         << ": { executed : " << free_executed
                         << ", warnings : " << free_warnings
                         <<   ", errors : " << free_errors << "}" << endl;

        *os << "full test report"
             << ": { executed : " << executed
             << ", warnings : " << warnings
             <<   ", errors : " << errors << "}" << endl;
    }

    void log        (const std::string & file, int line, const std::string & message) override
    {
        loc(file, line) << " log : " << message << endl;
    }
    void checkpoint (const std::string & file, int line) override
    {
        loc(file, line) << " checkpoint" << endl;
    }

    void message(const std::string & file, int line, bool condition, level_t lvl, bool critical, int, const std::string & message) override
    {
        check(file, line, condition, lvl, critical) << " [message]: " << message << endl;
    }
    void plain  (const std::string & file, int line, bool condition, level_t lvl, bool critical, int, const std::string & message) override
    {
        check(file, line, condition, lvl, critical) << " [expression]: " << message << endl;
    }

    void predicate  (const std::string & file, int line, bool condition, level_t lvl, bool critical, int,
                     const std::string & name, const std::string &args) override
    {
        check(file, line, condition, lvl, critical) << " [predicate]: " << name << '(' << args << ')' << endl;
    }
    void equal     (const std::string & file, int line, bool condition, level_t lvl, bool critical, int index, bool bw,
                    const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) override
    {

        check(file, line, condition, lvl, critical) << descr(bw, "equality") << ranged(index, lhs + " == " + rhs)
                                                    << "; [" << bitpre(bw, lhs_val) << " == " << bitpre(bw, rhs_val) << "]" << endl;
    }

    void not_equal (const std::string & file, int line, bool condition, level_t lvl, bool critical, int index, bool bw,
                    const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) override
    {
        check(file, line, condition, lvl, critical) << descr(bw, "equality") << ranged(index, lhs + " != " + rhs)
                                                    << "; [" << bitpre(bw, lhs_val) << " != " << bitpre(bw, rhs_val) << "]" << endl;
    }

    void close     (const std::string & file, int line, bool condition, level_t lvl, bool critical, int index,
                    const std::string & lhs, const std::string &rhs, const std::string & tolerance,
                    const std::string & lhs_val, const std::string & rhs_val, const std::string& tolerance_val) override
    {
        check(file, line, condition, lvl, critical) << " [equality]: " << ranged(index, lhs + " == " + rhs + " +/- " + tolerance)
                         << "; [" << lhs_val << " == " << rhs_val << " +/- " << tolerance_val << "]" << endl;
    }

    void close_rel (const std::string & file, int line, bool condition, level_t lvl, bool critical, int index,
                    const std::string & lhs, const std::string &rhs, const std::string & tolerance,
                    const std::string & lhs_val, const std::string & rhs_val, const std::string& tolerance_val) override
    {
        check(file, line, condition, lvl, critical) << " [equality]: " << ranged(index, lhs + " == " + rhs + " +/- " + tolerance)
                         << "~; [ " << lhs_val << " == " << rhs_val << " +/- " << tolerance_val << " ~ ]" << endl;
    }

    void close_per (const std::string & file, int line, bool condition, level_t lvl, bool critical, int index,
                    const std::string & lhs, const std::string &rhs, const std::string & tolerance,
                    const std::string & lhs_val, const std::string & rhs_val, const std::string& tolerance_val) override
    {
        check(file, line, condition, lvl, critical) << " [equality]: " << ranged(index, lhs + " == " + rhs + " +/- " + tolerance)
                         << "%; [ " << lhs_val << " == " << rhs_val << " +/- " << tolerance_val << " % ]" << endl;
    }

    void ge        (const std::string & file, int line, bool condition, level_t lvl, bool critical, int index, bool bw,
                    const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) override
    {
        check(file, line, condition, lvl, critical) << descr(bw, "comparison") << ranged(index, lhs + " >= " + rhs)
                                                    << "; [" << bitpre(bw, lhs_val) << " >= " << bitpre(bw, rhs_val) << "]" << endl;
    }

    void greater   (const std::string & file, int line, bool condition, level_t lvl, bool critical, int index,
                    const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) override
    {
        check(file, line, condition, lvl, critical) << " [comparison]: " << ranged(index, lhs + " > "  + rhs) << "; [" << lhs_val << " > " << rhs_val << "]" << endl;
    }

    void le        (const std::string & file, int line, bool condition, level_t lvl, bool critical, int index, bool bw,
                    const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) override
    {
        check(file, line, condition, lvl, critical) << descr(bw, "comparison") << ranged(index, lhs + " <= " + rhs)
                                                    << "; [" << bitpre(bw, lhs_val) << " <= " << bitpre(bw, rhs_val) << "]" << endl;
    }

    void lesser    (const std::string & file, int line, bool condition, level_t lvl, bool critical, int index,
                    const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) override
    {
        check(file, line, condition, lvl, critical) << " [comparison]: " << ranged(index, lhs + " < "  + rhs) << "; [" << lhs_val << " < " << rhs_val << "]" << endl;
    }

    void exception (const std::string & file, int line, bool condition, level_t lvl, bool critical, int,
                    const std::string & got, const std::string & expected) override
    {
        check(file, line, condition, lvl, critical) << " throw exception [" << expected << "] got [" << got << "]" <<  endl;
    }

    void any_exception(const std::string & file, int line, bool condition, level_t lvl, bool critical, int) override
    {
        check(file, line, condition, lvl, critical) << " throw any exception." << endl;
    }
    void no_exception (const std::string & file, int line, bool condition, level_t lvl, bool critical, int) override
    {
        check(file, line, condition, lvl, critical) << " throw no exception." << endl;
    }
    void no_execute   (const std::string & file, int line, bool condition, level_t lvl, bool critical, int) override
    {
        check(file, line, condition, lvl, critical) << " do not execute." << endl;
    }
    void execute      (const std::string & file, int line, bool condition, level_t lvl, bool critical, int) override
    {
        check(file, line, condition, lvl, critical) << " do execute." << endl;
    }
};

boost::optional<hrf_sink_t> hrf_sink;

data_sink_t * get_hrf_sink(std::ostream & os)
{
    hrf_sink.emplace();
    hrf_sink->os = &os;
    return &*hrf_sink;
}