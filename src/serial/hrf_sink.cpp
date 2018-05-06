/**
 * @file   sink.cpp
 * @date   06.05.2018
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

static std::string descr( const std::string & str)
{
    return " [" + str + "]: ";
}


struct hrf_sink_t : data_sink_t
{
    std::ostream * os = &std::cout;

    std::string group{"__metal_call"};

    std::ostream & loc(const std::string & file, int line)
    {
        return *os << boost::replace_all_copy(file, "\\\\", "\\") << '(' << line << ')';
    }

    std::ostream & check(const std::string & file, int line, bool condition, level_t lvl)
    {
        return loc(file, line) << descr(lvl) << msg_word(condition);
    }

    virtual ~hrf_sink_t() = default;

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

    void message(const std::string & file, int line, bool condition, level_t lvl, const std::string & message) override
    {
        check(file, line, condition, lvl) << " [message]: " << message << endl;
    }
    void plain  (const std::string & file, int line, bool condition, level_t lvl, const std::string & message) override
    {
        check(file, line, condition, lvl) << " [expression]: " << message << endl;
    }

    void equal     (const std::string & file, int line, bool condition, level_t lvl,
                    const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) override
    {

        check(file, line, condition, lvl) << descr("equality") << "; [" << lhs_val << " == " << rhs_val << "]" << endl;
    }

    void not_equal (const std::string & file, int line, bool condition, level_t lvl,
                    const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) override
    {
        check(file, line, condition, lvl) << descr("equality") << lhs + " != " + rhs
                                          << "; [" << lhs_val << " != " << rhs_val << "]" << endl;
    }

    void ge        (const std::string & file, int line, bool condition, level_t lvl,
                    const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) override
    {
        check(file, line, condition, lvl) << descr("comparison") << lhs + " >= " + rhs
                                          << "; [" << lhs_val << " >= " << rhs_val << "]" << endl;
    }

    void greater   (const std::string & file, int line, bool condition, level_t lvl,
                    const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) override
    {
        check(file, line, condition, lvl) << " [comparison]: " << (lhs + " > "  + rhs) << "; [" << lhs_val << " > " << rhs_val << "]" << endl;
    }

    void le        (const std::string & file, int line, bool condition, level_t lvl,
                    const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) override
    {
        check(file, line, condition, lvl) << descr("comparison") << (lhs + " <= " + rhs)
                                          << "; [" << lhs_val << " <= " << rhs_val << "]" << endl;
    }

    void lesser    (const std::string & file, int line, bool condition, level_t lvl,
                    const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) override
    {
        check(file, line, condition, lvl) << " [comparison]: " << (lhs + " < "  + rhs) << "; [" << lhs_val << " < " << rhs_val << "]" << endl;
    }

    void no_execute   (const std::string & file, int line, level_t lvl) override
    {
        check(file, line, false, lvl) << " do not execute." << endl;
    }
};

boost::optional<hrf_sink_t> hrf_sink;

data_sink_t * get_hrf_sink(std::ostream & os)
{
    hrf_sink.emplace();
    hrf_sink->os = &os;
    return &*hrf_sink;
}