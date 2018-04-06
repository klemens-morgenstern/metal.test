/**
 * @file   metal/gdb/mi2/interpreter.cpp
 * @date   20.12.2016
 * @author Klemens D. Morgenstern
 *



 */

#define BOOST_COROUTINE_NO_DEPRECATION_WARNING

#include <metal/gdb/mi2/interpreter.hpp>
#include <metal/gdb/mi2/output.hpp>
#include <metal/gdb/mi2/input.hpp>



#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>


#include <boost/algorithm/string/predicate.hpp>
#include <istream>
#include <iostream>
#include <sstream>

namespace asio = boost::asio;

namespace metal
{
namespace gdb
{
namespace mi2
{

inline static std::string quote_if(const std::string & str)
{
    if (std::find_if(str.begin(), str.end(), [](char c){return std::isspace(c);}) != str.end())
    {
        if ((str.front() != '"') || (str.back() != '"'))
            return '"' + str + '"';
    }
    return str;
}

template<typename ...Args>
constexpr bool needs_record()
{
    return sizeof...(Args) > 0;
}

template<typename ...Args>
void interpreter::_work_impl(Args&&...args)
{
    if (!_in_buf.empty())
    {
        asio::async_write(_in, asio::buffer(_in_buf), _yield);
        if (_debug)
            _fwd << _in_buf;
    }
    try {
        asio::async_read_until(_out, _out_buf, "(gdb)", _yield);
    }
    catch (boost::system::system_error & se)
    {
        //ignore this exception if this was the last valid command
        if (_out_buf.size() == 0)
            throw;
    }

    bool received_record = false;
    constexpr static bool needs_record_ = needs_record<Args...>();

    std::istream out_str(&_out_buf);
    std::string line;
    try {
        while (std::getline(out_str, line) && !boost::starts_with(line, "(gdb)"))
        {
            if (_debug)
                _fwd << line ;
            if (auto data = parse_stream_output(line))
            {
                _handle_stream_output(*data);
                continue;
            }

            if (auto data = parse_async_output(line))
            {
                if (data->first)
                {
                    if (!_handle_async_output(*data->first, data->second))
                        BOOST_THROW_EXCEPTION( unexpected_async_record(*data->first, line) );
                }

                else
                    _handle_async_output(data->second);
                continue;
            }

            if (auto data = parse_record(line))
            {
                _handle_record(line, data->first, data->second, std::forward<Args>(args)...);
                received_record = true;
                continue;
            }
            else if (!_debug)
                _fwd << line << '\n';
        }

        if (needs_record_ && !received_record)
            BOOST_THROW_EXCEPTION( interpreter_error("No record received, even though expected"));
    }
    catch (std::exception & e)
    {
        _fwd << "***** Interpreter exception ***** : " << e.what() << std::endl;
        while (std::getline(out_str, line) && !boost::starts_with(line, "(gdb)"))
            _fwd << line;
        _fwd << "(gdb)" << std::endl;
        throw ;
    }
}


void interpreter::_work() {_work_impl();}
void interpreter::_work(std::uint64_t token, result_class rc) { _work_impl(token, rc); }
//void interpreter::_work(const std::function<void(const result_output&)> & func) {_work_impl(func); }
void interpreter::_work(std::uint64_t token, const std::function<void(const result_output&)> & func) {_work_impl(token, func);}

async_result interpreter::wait_for_stop()
{
    async_result pr;
    _in_buf.clear();

    auto l = [&](const async_output& ao)
             {
                if ((ao.type == async_output::exec) &&
                    (ao.class_ == "stopped"))
                {
                    pr.reason  = mi2::find(ao.results, "reason").as_string();
                    pr.content.resize(ao.results.size() - 1);
                    std::copy_if(ao.results.begin(), ao.results.end(), pr.content.begin(),
                                    [](const result & r){return r.variable != "reason";});
                }
             };

    boost::signals2::scoped_connection conn = _async_sink.connect(l);
    _work();
    return pr;
}

void interpreter::_handle_record(const std::string& line, const boost::optional<std::uint64_t> &token, const result_output & sr)
{
    BOOST_THROW_EXCEPTION( unexpected_record(line) );
}
void interpreter::_handle_record(const std::string& line, const boost::optional<std::uint64_t> &token, const result_output & sr,
                    std::uint64_t expected_token, result_class rc)
{
    if (!token)
        BOOST_THROW_EXCEPTION( mismatched_token(expected_token, 0) );


    if (*token != expected_token)
        BOOST_THROW_EXCEPTION( mismatched_token(expected_token, *token) );

    if (sr.class_ == result_class::error)
    {
        auto err = parse_result<error_>(sr.results);
        BOOST_THROW_EXCEPTION( exception(err) );
    }
    if ((sr.class_ == rc) && sr.results.empty())
        return;
    else
        BOOST_THROW_EXCEPTION( unexpected_record(line) );
}

void interpreter::_handle_record(const std::string& line, const boost::optional<std::uint64_t> &token, const result_output & sr,
                    std::uint64_t expected_token, const std::function<void(const result_output&)> & func)
{

    if (!token)
        BOOST_THROW_EXCEPTION( mismatched_token(expected_token, 0) );

    if (*token != expected_token)
        BOOST_THROW_EXCEPTION( mismatched_token(expected_token, *token) );

    func(sr);
}


void interpreter::_handle_async_output(const async_output & ao)
{
    _async_sink(ao);
}
bool interpreter::_handle_async_output(std::uint64_t token, const async_output & ao)
{
    int cnt = 0;
    for (auto itr = _pending_asyncs.begin(); itr != _pending_asyncs.end(); itr++)
        if ((itr->first == token))
        {
            cnt++;
            if (itr->second(ao))
                itr = _pending_asyncs.erase(itr) - 1;
        }

    return cnt != 0;
}

std::string interpreter::read_header()
{
    _in_buf.clear();

    std::string value;
    boost::signals2::scoped_connection conn = _stream_console.connect([&](const std::string & str){value += str; value += '\n';});
    _work();
    return value;
}

/**
 * The breakpoint number number is not in effect until it has been hit count times.
 * To see how this is reflected in the output of the �-break-list� command, see the
 * description of the �-break-list� command below.
 */

void interpreter::break_after(int number, int count)
{
    _in_buf = std::to_string(_token_gen) + "-break-after " + std::to_string(number) + " " + std::to_string(count) + "\n";
    _work(_token_gen++, result_class::done);
}

/**
 * Specifies the CLI commands that should be executed when breakpoint number is hit.
 * The parameters command1 to commandN are the commands. If no command is specified,
 * any previously-set commands are cleared. See Break Commands. Typical use of this
 * functionality is tracing a program, that is, printing of values of some variables
 * whenever breakpoint is hit and then continuing.
 */

void interpreter::break_commands(int number, const std::vector<std::string> & commands)
{
    _in_buf = std::to_string(_token_gen) + "-break-commands " + std::to_string(number);

    for (const auto & cmd : commands)
    {
        _in_buf += " \"";
        _in_buf += cmd;
        _in_buf += '"';
    }

    _in_buf += '\n';
    _work(_token_gen++, result_class::done);
}

/**
 * Breakpoint number will stop the program only if the condition in expr is true.
 * The condition becomes part of the �-break-list� output (see the description of the
 * �-break-list� command below).
 */

void interpreter::break_condition(int number, const std::string & condition)
{
    _in_buf = std::to_string(_token_gen) + "-break-condition " + std::to_string(number) + ' ' + condition + '\n';
    _work(_token_gen++, result_class::done);
}

/**
 * Delete the breakpoint(s) whose number(s) are specified in the argument list.
 * This is obviously reflected in the breakpoint list.
 */
void interpreter::break_delete(int number)
{
    _in_buf = std::to_string(_token_gen) + "-break-delete " + std::to_string(number) + '\n';
    _work(_token_gen++, result_class::done);
}
///@overload void interpreter::break_delete(int number)
void interpreter::break_delete(const std::vector<int> &numbers)
{
    _in_buf = std::to_string(_token_gen) + "-break-delete ";

    for (auto & x : numbers)
        _in_buf += " " + std::to_string(x);

    _in_buf += '\n';
    _work(_token_gen++, result_class::done);
}


/**
 * Disable the named breakpoint(s). The field �enabled� in the break list is now
 * set to �n� for the named breakpoint(s).
 */
void interpreter::break_disable(int number)
{
    _in_buf = std::to_string(_token_gen) + "-break-disable " + std::to_string(number) + '\n';
    _work(_token_gen++, result_class::done);
}
///@overload void interpreter::break_delete(int number)
void interpreter::break_disable(const std::vector<int> &numbers)
{
    _in_buf = std::to_string(_token_gen) + "-break-disable ";

    for (auto & x : numbers)
        _in_buf += " " + std::to_string(x);

    _in_buf += '\n';
    _work(_token_gen++, result_class::done);
}

/**
 * Enable (previously disabled) breakpoint(s).
 */
void interpreter::break_enable(int number)
{
    _in_buf = std::to_string(_token_gen) + "-break-enable " + std::to_string(number) + '\n';
    _work(_token_gen++, result_class::done);
}
///@overload void interpreter::break_delete(int number)
void interpreter::break_enable(const std::vector<int> &numbers)
{
    _in_buf = std::to_string(_token_gen) + "-break-enable ";

    for (auto & x : numbers)
        _in_buf += " " + std::to_string(x);

    _in_buf += '\n';
    _work(_token_gen++, result_class::done);
}


breakpoint interpreter::break_info(int number)
{
    _in_buf = std::to_string(_token_gen) + "-break-info " + std::to_string(number) + '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);
    try {
        auto tab  = find(rc.results, "BreakpointTable").as_tuple();
        auto body = find(tab, "body").as_list().as_results();

        auto itr = std::find_if(body.begin(), body.end(), [](const result & r){return r.variable == "bkpt";});
        if (itr == body.end())
            BOOST_THROW_EXCEPTION( missing_value("bkpt") );

        return parse_result<breakpoint>(itr->value_.as_tuple());
    }
    catch (std::exception & e)
    {
        std::cerr << "Parser error: '" << e.what() << std::endl;
        throw;
        return breakpoint{};
    }
}

static std::string loc_for_break(const linespec_location & exp)
{
    std::string location;

    if (exp.linenum && !exp.filename)
        location = std::to_string(*exp.linenum);
    else if (exp.offset)
        location = std::to_string(*exp.offset);
    else if (exp.filename && exp.linenum)
        location = *exp.filename + ":" + std::to_string(*exp.linenum);
    else if (exp.function && !exp.label && !exp.filename)
        location = *exp.function;
    else if (exp.function && exp.label)
        location = *exp.function + ":" + *exp.label;
    else if (exp.filename && exp.function)
        location = *exp.filename + ":" + *exp.function;
    else if (exp.label)
        location = *exp.label;

    return location;
}

static std::string loc_for_break(const explicit_location & exp)
{
    std::string location;
    if (exp.source)
        location += "--source "   + *exp.source   + " ";
    if (exp.function)
        location += "--function " + *exp.function + " ";
    if (exp.label)
        location += "--label " + *exp.label + " ";

    if (exp.line)
        location += "--line " + std::to_string(*exp.line);
    else if (exp.line_offset)
    {
        if (*exp.line_offset > 0)
            location += "--line +" + std::to_string(*exp.line_offset);
        else
            location += "--line " + std::to_string(*exp.line_offset);
    }
    return location;
}

static std::string loc_for_break(const address_location & exp)
{
    if (exp.expression)
        return "*" + *exp.expression;

    std::stringstream location;
    location << "*";

    if (!exp.filename && exp.funcaddr)
        location << "0x" << std::hex << *exp.funcaddr;
    else if (exp.filename && exp.funcaddr)
        location << "'" << *exp.filename << "'" << "0x" << std::hex << *exp.funcaddr;
    return location.str();
}

std::vector<breakpoint> interpreter::break_insert(const linespec_location & exp,
        bool temporary, bool hardware, bool pending,
        bool disabled, bool tracepoint,
        const boost::optional<std::string> & condition,
        const boost::optional<int> & ignore_count,
        const boost::optional<int> & thread_id)
{
    return break_insert(loc_for_break(exp), temporary, hardware, pending, disabled, tracepoint, condition, ignore_count, thread_id);
}

std::vector<breakpoint> interpreter::break_insert(const explicit_location & exp,
        bool temporary, bool hardware, bool pending,
        bool disabled, bool tracepoint,
        const boost::optional<std::string> & condition,
        const boost::optional<int> & ignore_count,
        const boost::optional<int> & thread_id)
{
    return break_insert(loc_for_break(exp), temporary, hardware, pending, disabled, tracepoint, condition, ignore_count, thread_id);
}

std::vector<breakpoint> interpreter::break_insert(const address_location & exp,
        bool temporary, bool hardware, bool pending,
        bool disabled, bool tracepoint,
        const boost::optional<std::string> & condition,
        const boost::optional<int> & ignore_count,
        const boost::optional<int> & thread_id)
{
    return break_insert(loc_for_break(exp), temporary, hardware, pending, disabled, tracepoint, condition, ignore_count, thread_id);
}

std::vector<breakpoint> interpreter::break_insert(const std::string & location,
        bool temporary, bool hardware, bool pending,
        bool disabled, bool tracepoint,
        const boost::optional<std::string> & condition,
        const boost::optional<int> & ignore_count,
        const boost::optional<int> & thread_id)
{
    _in_buf = std::to_string(_token_gen) + "-break-insert ";
    if (temporary)
        _in_buf += "-t ";
    if (hardware)
        _in_buf += "-h ";
    if (pending)
        _in_buf += "-f ";
    if (disabled)
        _in_buf += "-d ";
    if (tracepoint)
        _in_buf += "-a ";
    if (condition)
        _in_buf += "-c " + quote_if(*condition) + " ";
    if (ignore_count)
        _in_buf += "-i " + std::to_string(*ignore_count) + " ";
    if (thread_id)
        _in_buf += "-p " + std::to_string(*thread_id) + " ";

    _in_buf += (location + '\n');

    metal::gdb::mi2::result_output rc;

    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });


    if (rc.class_ != result_class::done)
        _throw_unexpected_result(result_class::done, rc);

    std::vector<breakpoint> bps;

    for (auto & res : rc.results)
    {
        if (res.variable == "bkpt")
            bps.push_back(parse_result<breakpoint>(res.value_.as_tuple()));
    }

    return bps;
}


breakpoint interpreter::dprintf_insert(
        const std::string & format, const std::vector<std::string> & argument,
        const linespec_location & location,
        bool temporary, bool pending, bool disabled,
        const boost::optional<std::string> & condition,
        const boost::optional<int> & ignore_count,
        const boost::optional<int> & thread_id)
{
    return  dprintf_insert(format, argument, boost::make_optional(loc_for_break(location)),
            temporary, pending, disabled, condition, ignore_count, thread_id);
}

breakpoint interpreter::dprintf_insert(
        const std::string & format, const std::vector<std::string> & argument,
        const explicit_location& location,
        bool temporary, bool pending, bool disabled,
        const boost::optional<std::string> & condition,
        const boost::optional<int> & ignore_count,
        const boost::optional<int> & thread_id)
{
    return  dprintf_insert(format, argument, boost::make_optional(loc_for_break(location)),
            temporary, pending, disabled, condition, ignore_count, thread_id);
}

breakpoint interpreter::dprintf_insert(
        const std::string & format, const std::vector<std::string> & argument,
        const address_location& location,
        bool temporary, bool pending, bool disabled,
        const boost::optional<std::string> & condition,
        const boost::optional<int> & ignore_count,
        const boost::optional<int> & thread_id)
{
    return  dprintf_insert(format, argument, boost::make_optional(loc_for_break(location)),
            temporary, pending, disabled, condition, ignore_count, thread_id);
}

breakpoint interpreter::dprintf_insert(
        const std::string & format, const std::vector<std::string> & argument = {},
        const boost::optional<std::string> & location,
        bool temporary, bool pending, bool disabled,
        const boost::optional<std::string> & condition,
        const boost::optional<int> & ignore_count,
        const boost::optional<int> & thread_id)
{
    _in_buf = std::to_string(_token_gen) + "-dprintf-insert ";
    if (temporary)
        _in_buf += "-t ";
    if (pending)
        _in_buf += "-f ";
    if (disabled)
        _in_buf += "-d ";
    if (condition)
        _in_buf += "-c " + *condition + " ";
    if (ignore_count)
        _in_buf += "-i " + std::to_string(*ignore_count) + " ";
    if (thread_id)
        _in_buf += "-p " + std::to_string(*thread_id) + " ";

    if (location)
        _in_buf += *location + " ";

    _in_buf += format;

    for (auto & arg : argument)
        _in_buf +=  " " + arg;

    _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<breakpoint>(find(rc.results, "bpkt").as_tuple());
}




std::vector<breakpoint> interpreter::break_list()
{
    metal::gdb::mi2::result_output rc;
    _in_buf = std::to_string(_token_gen) + "-break-list\n";
    _work(_token_gen++,
            [&](const metal::gdb::mi2::result_output & rc_in)
                        {
                            rc = std::move(rc_in);
                        });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    auto body = find(rc.results, "body").as_list().as_results();

    std::vector<breakpoint> vec;
    vec.resize(body.size());

    std::transform(body.begin(), body.end(), vec.begin(),
                    [&](const result & rc)
                    {
                       return parse_result<breakpoint>(find(rc.value_.as_tuple(), "bkpt").as_list().as_results());
                    });
    return vec;
}

void interpreter::break_passcount(std::size_t tracepoint_number, std::size_t passcount)
{
    _in_buf = std::to_string(_token_gen) + "-break-passcount " + std::to_string(tracepoint_number) + ' ' + std::to_string(passcount) + '\n';
    _work(_token_gen++, result_class::done);
}

watchpoint interpreter::break_watch(const std::string & expr, bool access, bool read)
{
    _in_buf = std::to_string(_token_gen) + "-break-watch ";

    if (access)
        _in_buf += "-a ";
    if (read)
        _in_buf += "-r ";

    _in_buf += expr;
    _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<watchpoint>(find(rc.results, "wpt").as_tuple());
}

breakpoint interpreter::catch_load(const std::string regexp,
                       bool temporary, bool disabled)
{
    _in_buf = std::to_string(_token_gen) + "-catch-load ";
    if (temporary)
        _in_buf += "-t ";
    if (disabled)
        _in_buf += "-d ";

    _in_buf += regexp;
    _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<breakpoint>(find(rc.results, "bkpt").as_tuple());
}

breakpoint interpreter::catch_unload(const std::string regexp,
                                     bool temporary, bool disabled)

{
    _in_buf = std::to_string(_token_gen) + "-catch-unload ";
    if (temporary)
        _in_buf += "-t ";
    if (disabled)
        _in_buf += "-d ";

    _in_buf += regexp;
    _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<breakpoint>(find(rc.results, "bkpt").as_tuple());
}

breakpoint interpreter::catch_assert(const boost::optional<std::string> & condition, bool temporary, bool disabled)
{
    _in_buf = std::to_string(_token_gen) + "-catch-assert";

    if (condition)
        _in_buf += " -c " + *condition;
    if (temporary)
        _in_buf += " -t";
    if (disabled)
        _in_buf += " -d";

     _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<breakpoint>(find(rc.results, "bkpt").as_tuple());
}

breakpoint interpreter::catch_exception(const boost::optional<std::string> & condition, bool temporary, bool disabled)
{
    _in_buf = std::to_string(_token_gen) + "-catch-exception";

    if (condition)
        _in_buf += " -c " + *condition;
    if (temporary)
        _in_buf += " -t";
    if (disabled)
        _in_buf += " -d";

     _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<breakpoint>(find(rc.results, "bkpt").as_tuple());
}

void interpreter::exec_arguments(const std::vector<std::string> & args)
{
    _in_buf = std::to_string(_token_gen) + "-exec-arguments";

    for (const auto & arg : args)
    {
        _in_buf += ' ';
        _in_buf += arg;
    }

    _in_buf += '\n';
    _work(_token_gen++, result_class::done);
}

void interpreter::environment_cd(const std::string path)
{
    _in_buf = std::to_string(_token_gen) + "-environment-cd " + path + '\n' ;
    _work(_token_gen++, result_class::done);
}

std::string interpreter::environment_directory(const std::vector<std::string> & path, bool reset)
{
    std::string result;

    _in_buf = std::to_string(_token_gen) + "-environment-directory";

    if (reset)
        _in_buf += " -r";


    for (auto & p : path)
        _in_buf += (" " + p);

     _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return find(rc.results, "source-path").as_string();
}

std::string interpreter::environment_path(const std::vector<std::string> & path, bool reset)
{
    std::string result;

    _in_buf = std::to_string(_token_gen) + "-environment-path";

    if (reset)
        _in_buf += " -r";


    for (auto & p : path)
        _in_buf += (" " + p);

     _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return find(rc.results, "path").as_string();
}

std::string interpreter::environment_pwd()
{
    std::string result;

    _in_buf = std::to_string(_token_gen) + "-environment-pwd\n";
    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return find(rc.results, "cwd").as_string();
}


thread_state interpreter::thread_info_(const boost::optional<int> & id)
{
    _in_buf = std::to_string(_token_gen) + "-thread-info";
    if (id)
        _in_buf += (" " + std::to_string(*id) );
    _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
   _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

   if (rc.class_ != result_class::done)
       BOOST_THROW_EXCEPTION( unexpected_result_class(result_class::done, rc.class_) );

   return parse_result<thread_state>(rc.results);
}

thread_id_list interpreter::thread_list_ids()
{
    _in_buf = std::to_string(_token_gen) + "-thread-list-ids\n";


    metal::gdb::mi2::result_output rc;
   _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

   if (rc.class_ != result_class::done)
       BOOST_THROW_EXCEPTION( unexpected_result_class(result_class::done, rc.class_));

   return parse_result<thread_id_list>(rc.results);
}

thread_select interpreter::thread_select(int id)
{
    _in_buf = std::to_string(_token_gen) + "-thread-select " + std::to_string(id) + '\n';

    metal::gdb::mi2::result_output rc;
   _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

   if (rc.class_ != result_class::done)
       BOOST_THROW_EXCEPTION( unexpected_result_class(result_class::done, rc.class_));

   return parse_result<struct thread_select>(rc.results);
}

std::vector<ada_task_info> interpreter::ada_task_info(const boost::optional<int> & task_id)
{
    _in_buf = std::to_string(_token_gen) + "-ada-task-info";
    if (task_id)
        _in_buf += " " +  std::to_string(*task_id);
    _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
   _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

   if (rc.class_ != result_class::done)
       BOOST_THROW_EXCEPTION( unexpected_result_class(result_class::done, rc.class_) );

   std::vector<struct ada_task_info> infos;

   for (const auto & v : find(rc.results, "body").as_list().as_values())
       infos.push_back(parse_result<struct ada_task_info>(v.as_tuple()));


   return infos;
}

void interpreter::exec_continue(bool reverse, bool all)
{
    _in_buf = std::to_string(_token_gen) + "-exec-continue";

    if (reverse)
        _in_buf += " --reverse";
    if (all)
        _in_buf += " --all";
    _in_buf += "\n";

    _work(_token_gen++, result_class::running);
}

void interpreter::exec_continue(bool reverse, int thread_group)
{
    _in_buf = std::to_string(_token_gen) + "-exec-continue";

    if (reverse)
        _in_buf += " --reverse";
    if (thread_group)
        _in_buf += " --thread-group " + std::to_string(thread_group);
    _in_buf += "\n";

    _work(_token_gen++, result_class::running);
}

void interpreter::exec_finish(bool reverse)
{
    _in_buf = std::to_string(_token_gen) + "-exec-finish";

    if (reverse)
        _in_buf += " --reverse";
    _in_buf += "\n";

    _work(_token_gen++, result_class::running);
}

void interpreter::exec_interrupt(bool all)
{
    _in_buf = std::to_string(_token_gen) + "-exec-interrupt";

    if (all)
        _in_buf += " --all";
    _in_buf += "\n";

    _work(_token_gen++, result_class::running);
}

void interpreter::exec_interrupt(int thread_group)
{
    _in_buf = std::to_string(_token_gen) + "-exec-interrupt";

    if (thread_group)
        _in_buf += " --thread-group " + std::to_string(thread_group);
    _in_buf += "\n";

    _work(_token_gen++, result_class::running);
}


static std::string loc_str(const linespec_location & exp)
{
    std::string location;

    if (exp.linenum && !exp.filename)
        location = std::to_string(*exp.linenum);
    else if (exp.offset)
        location = std::to_string(*exp.offset);
    else if (exp.filename && exp.linenum)
        location = *exp.filename + ":" + std::to_string(*exp.linenum);
    else if (exp.function && !exp.label && !exp.filename)
        location = *exp.function;
    else if (exp.function && exp.label)
        location = *exp.function + ":" + *exp.label;
    else if (exp.filename && exp.function)
        location = *exp.filename + ":" + *exp.function;
    else if (exp.label)
        location = *exp.label;

    return location;
}

static std::string loc_str(const explicit_location & exp)
{
    std::string location;
    if (exp.source)
        location += "-source "   + *exp.source   + " ";
    if (exp.function)
        location += "-function " + *exp.function + " ";
    if (exp.label)
        location += "-label " + *exp.label + " ";

    if (exp.line)
        location += "-line " + std::to_string(*exp.line);
    else if (exp.line_offset)
    {
        if (*exp.line_offset > 0)
            location += "-line +" + std::to_string(*exp.line_offset);
        else
            location += "-line " + std::to_string(*exp.line_offset);
    }
    return location;
}

static std::string loc_str(const address_location & exp)
{
    if (exp.expression)
        return "*" + *exp.expression;

    std::stringstream location;
    location << "*";

    if (!exp.filename && exp.funcaddr)
        location << "0x" << std::hex << *exp.funcaddr;
    else if (exp.filename && exp.funcaddr)
        location << "'" << *exp.filename << "'" << "0x" << std::hex << *exp.funcaddr;
    return location.str();
}


void interpreter::exec_jump(const linespec_location & ls) {return exec_jump(loc_str(ls));}
void interpreter::exec_jump(const explicit_location & el) {return exec_jump(loc_str(el));}
void interpreter::exec_jump(const address_location  & al) {return exec_jump(loc_str(al));}

void interpreter::exec_jump(const std::string & location)
{
    _in_buf = std::to_string(_token_gen) + "-exec-jump " + location + '\n';
    _work(_token_gen++, result_class::running);

}

void interpreter::exec_next(bool reverse)
{
    _in_buf = std::to_string(_token_gen) + "-exec-next";

    if (reverse)
        _in_buf += " --reverse";
    _in_buf += "\n";

    _work(_token_gen++, result_class::running);
}

void interpreter::exec_next_instruction(bool reverse)
{
    _in_buf = std::to_string(_token_gen) + "-exec-next-instruction";

    if (reverse)
        _in_buf += " --reverse";
    _in_buf += "\n";

    _work(_token_gen++, result_class::running);
}

frame interpreter::exec_return(const boost::optional<std::string> &val)
{
    _in_buf = std::to_string(_token_gen) + "-exec-return";

    if (val)
        _in_buf += " " + *val;

    _in_buf += "\n";

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<frame>(find(rc.results, "frame").as_tuple());
}

void interpreter::exec_run(bool start, bool all)
{
    _in_buf = std::to_string(_token_gen) + "-exec-run";
    if (start)
        _in_buf += " --start";
    if (all)
        _in_buf += " --all";

    _in_buf += '\n';
    _work(_token_gen++, result_class::running);
}

void interpreter::exec_run(bool start, int thread_group)
{
    _in_buf = std::to_string(_token_gen) + "-exec-run";
    if (start)
        _in_buf += " --start";

    _in_buf += (" thread-group " + std::to_string(thread_group) + '\n');
    _work(_token_gen++, result_class::running);
}

void interpreter::exec_step(bool reverse)
{
    _in_buf = std::to_string(_token_gen) + "-exec-step";

    if (reverse)
        _in_buf += " --reverse";
    _in_buf += "\n";

    _work(_token_gen++, result_class::running);
}
void interpreter::exec_step_instruction(bool reverse)
{
    _in_buf = std::to_string(_token_gen) + "-exec-step-instruction";

    if (reverse)
        _in_buf += " --reverse";
    _in_buf += "\n";

    _work(_token_gen++, result_class::running);
}


void interpreter::exec_until(const linespec_location & ls) {return exec_until(loc_str(ls));}
void interpreter::exec_until(const explicit_location & el) {return exec_until(loc_str(el));}
void interpreter::exec_until(const address_location  & al) {return exec_until(loc_str(al));}

void interpreter::exec_until(const std::string & location)
{
    _in_buf = std::to_string(_token_gen) + "-exec-until " + location + '\n';
    _work(_token_gen++, result_class::running);
}

void interpreter::enable_frame_filters()
{
    _in_buf = std::to_string(_token_gen) + "-enable-frame-filters\n";
    _work(_token_gen++, result_class::running);
}

frame interpreter::stacke_info_frame()
{
    _in_buf = std::to_string(_token_gen) + "-stack-info-frame\n";

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<frame>(find(rc.results, "frame").as_tuple());
}

std::size_t interpreter::stack_info_depth()
{
    _in_buf = std::to_string(_token_gen) + "-stack-info-depth\n";

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return std::stoull(find(rc.results, "depth").as_string());
}

std::vector<frame> interpreter::stack_list_arguments(
        enum print_values print_values,
        const boost::optional<std::pair<std::size_t, std::size_t>> & frame_range,
        bool no_frame_filters,
        bool skip_unavailable)
{
    _in_buf = std::to_string(_token_gen) + "-stack-list-arguments ";
    if (no_frame_filters)
        _in_buf += "--no-frame-filters ";

    if (skip_unavailable)
        _in_buf += "--skip-unavailable ";


    _in_buf += std::to_string(static_cast<int>(print_values));

    if (frame_range)
        _in_buf += " " + std::to_string(frame_range->first) + " " + std::to_string(frame_range->second);

    _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    std::vector<frame> fr;
    auto val = find(rc.results, "stack-args").as_list().as_results();
    fr.reserve(val.size());

    for (const auto & v : val)
        fr.push_back(parse_result<frame>(v.value_.as_tuple()));

    return fr;
}

std::vector<frame> interpreter::stack_list_frames(
        const boost::optional<std::pair<std::size_t, std::size_t>> & frame_range,
        bool no_frame_filters)
{
    _in_buf = std::to_string(_token_gen) + "-stack-list-frames ";
    if (no_frame_filters)
        _in_buf += " --no-frame-filters";

    if (frame_range)
        _in_buf += " " + std::to_string(frame_range->first) + " " + std::to_string(frame_range->second);

    _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    std::vector<frame> fr;
    auto val = find(rc.results, "stack").as_list().as_results();
    fr.reserve(val.size());

    for (const auto & v : val)
        fr.push_back(parse_result<frame>(v.value_.as_tuple()));

    return fr;
}

std::vector<arg> interpreter::stack_list_locals(
        enum print_values print_values,
        bool no_frame_filters,
        bool skip_unavailable)
{
    _in_buf = std::to_string(_token_gen) + "-stack-list-locals ";
    if (no_frame_filters)
        _in_buf += "--no-frame-filters ";

    if (skip_unavailable)
        _in_buf += "--skip-unavailable ";


    _in_buf += std::to_string(static_cast<int>(print_values));

    _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);


    auto lst = find(rc.results, "locals").as_list();

    struct visitor_t : boost::static_visitor<>
    {
        std::vector<arg> fr;

        void operator()(const std::vector<result> & res)
        {
            fr.reserve(res.size());
            for (const auto & r : res)
            {
                arg a;
                a.name = r.value_.as_string();
                fr.push_back(std::move(a));
            }
        }

        void operator()(const std::vector<value> & val)
        {
            fr.reserve(val.size());
            for (const auto & v : val)
                fr.push_back(parse_result<arg>(v.as_tuple()));
        }
    } visitor;

    lst.apply_visitor(visitor);

    return visitor.fr;
}

std::vector<arg> interpreter::stack_list_variables(
        enum print_values print_values,
        bool no_frame_filters,
        bool skip_unavailable)
{
    _in_buf = std::to_string(_token_gen) + "-stack-list-variables ";
    if (no_frame_filters)
        _in_buf += "--no-frame-filters ";

    if (skip_unavailable)
        _in_buf += "--skip-unavailable ";


    _in_buf += std::to_string(static_cast<int>(print_values));

    _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    auto lst = find(rc.results, "variables").as_list();

    struct visitor_t : boost::static_visitor<>
    {
        std::vector<arg> fr;

        void operator()(const std::vector<result> & res)
        {
            fr.reserve(res.size());
            for (const auto & r : res)
            {
                arg a;
                a.name = r.value_.as_string();
                fr.push_back(std::move(a));
            }
        }

        void operator()(const std::vector<value> & val)
        {
            fr.reserve(val.size());
            for (const auto & v : val)
                fr.push_back(parse_result<arg>(v.as_tuple()));
        }
    } visitor;

    lst.apply_visitor(visitor);
    return visitor.fr;
}


void interpreter::stack_select_frame(std::size_t framenum)
{
    _in_buf = std::to_string(_token_gen) + "-stack-select-frame " + std::to_string(framenum) + '\n';
    _work(_token_gen++, result_class::done);
}

void interpreter::enable_pretty_printing()
{
    _in_buf = std::to_string(_token_gen) + "-enable-pretty-printing\n";
    _work(_token_gen++, result_class::done);
}


varobj interpreter::var_create(const std::string& expression,
                            const boost::optional<std::string> & name,
                            const boost::optional<std::uint64_t> & addr)
{
    _in_buf = std::to_string(_token_gen) + "-var-create ";
    if (name)
        _in_buf += *name + " ";
    else
        _in_buf += "- ";

    if (addr)
        _in_buf += std::to_string(*addr) + " ";
    else
        _in_buf += "* ";


    _in_buf += expression;
    _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<varobj>(rc.results);
}

varobj interpreter::var_create_floating(const std::string & expression, const boost::optional<std::string> & name)
{
    _in_buf = std::to_string(_token_gen) + "-var-create ";
    if (name)
        _in_buf += *name + " ";
    else
        _in_buf += "- ";

    _in_buf += "@ ";
    _in_buf += expression;
    _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<varobj>(rc.results);
}

void interpreter::var_delete(const std::string & name, bool leave_children)
{
    _in_buf = std::to_string(_token_gen) + "-var-delete ";
    if (leave_children)
        _in_buf += "-c ";
    _in_buf += name;
    _in_buf += '\n';

    _work(_token_gen++, result_class::done);
}

void interpreter::var_set_format(const std::string & name, format_spec fs)
{
    _in_buf = std::to_string(_token_gen) + "-var-set-format ";
    _in_buf += name;

    switch (fs)
    {
    case format_spec::binary          : _in_buf += " binary\n";           break;
    case format_spec::decimal         : _in_buf += " decimal\n";          break;
    case format_spec::hexadecimal     : _in_buf += " hexadecimal\n";      break;
    case format_spec::octal           : _in_buf += " octal\n";            break;
    case format_spec::natural         : _in_buf += " natural\n";          break;
    case format_spec::zero_hexadecimal: _in_buf += " zero-hexadecimal\n"; break;
    default: break;
    }
    _work(_token_gen++, result_class::done);
}

format_spec interpreter::var_show_format(const std::string & name)
{
    _in_buf = std::to_string(_token_gen) + "-var-show-format " + name + '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);
    auto var = find(rc.results, "format").as_string();

    if (var == "binary")           return format_spec::binary;
    if (var == "decimal")          return format_spec::decimal;
    if (var == "hexadecimal")      return format_spec::hexadecimal;
    if (var == "octal")            return format_spec::octal;
    if (var == "natural")          return format_spec::natural;
    if (var == "zero-hexadecimal") return format_spec::zero_hexadecimal;

    return static_cast<format_spec>(-1);
}


std::size_t interpreter::var_info_num_children(const std::string & name)
{
    _in_buf = std::to_string(_token_gen) + "-var-info-num-children " + name + '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);
    return std::stoull( find(rc.results, "numchild").as_string() );
}

std::vector<varobj> interpreter::var_list_children(const std::string & name,
                                                   const boost::optional<enum print_values> & print_values,
                                                   const boost::optional<std::pair<int, int>> & range)
{
    _in_buf = std::to_string(_token_gen) + "-var-list-children ";

    if (print_values)
        _in_buf += std::to_string(static_cast<int>(*print_values)) + " ";

    _in_buf += name;

    if (range)
        _in_buf += " " + std::to_string(range->first) + " " + std::to_string(range->second);

    _in_buf += '\n';


    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    std::vector<varobj> vec;
    vec.reserve(std::stoi(find(rc.results, "numchild").as_string()));

    for (auto & elem : find(rc.results, "chlidren").as_list().as_values())
        vec.push_back(parse_result<varobj>(find(elem.as_tuple(), "child").as_tuple()));

    return vec;
}

std::string interpreter::var_info_type(const std::string & name)
{
    _in_buf = std::to_string(_token_gen) + "-var-info-type " + name + '\n';
    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return find(rc.results, "type").as_string();
}

std::pair<std::string, std::string> interpreter::var_info_expression(const std::string & exp)
{
    _in_buf = std::to_string(_token_gen) + "-var-info-expression " + exp + '\n';
    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return std::make_pair(find(rc.results, "lang").as_string(), find(rc.results, "exp").as_string());
}

std::string interpreter::var_info_path_expression(const std::string & name)
{
    _in_buf = std::to_string(_token_gen) + "-var-info-path-expression " + name + '\n';
    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return find(rc.results, "path_expr").as_string();
}

std::vector<std::string> interpreter::var_show_attributes(const std::string & name)
{
    _in_buf = std::to_string(_token_gen) + "-var-show-attributes " + name + '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    std::vector<std::string> vec;
    auto in = find(rc.results, "status").as_list().as_values();
    vec.resize(in.size());

    for (const auto & i : in)
        vec.push_back(i.as_string());

    return vec;
}

std::string interpreter::var_evaluate_expression(
        const std::string & name,
        const boost::optional<std::string> & format)
{
    _in_buf = std::to_string(_token_gen) + "-var-evaluate-expression ";

    if (format)
        _in_buf += "-f " + *format + " ";

    _in_buf += name + '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return find(rc.results, "value").as_string();
}

std::string interpreter::var_assign(const std::string & name, const std::string& expr)
{
    _in_buf = std::to_string(_token_gen) + "-var-assign " + name + " " + expr + '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return find(rc.results, "value").as_string();
}


std::vector<varobj_update> interpreter::var_update(
                const boost::optional<std::string> & name,
                const boost::optional<enum print_values> & print_values)
{
    _in_buf = std::to_string(_token_gen) + "-var-update ";

    if (print_values)
        _in_buf += std::to_string(static_cast<int>(*print_values)) + " ";

    if (name)
        _in_buf += *name;
    else
        _in_buf += "*";

    _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    std::vector<varobj_update> vec;
    auto in = find(rc.results, "status").as_list().as_values();
    vec.resize(in.size());

    for (const auto & i : in)
        vec.push_back(parse_result<varobj_update>(i.as_tuple()));

    return vec;
}

void interpreter::var_set_frozen(const std::string & name, bool freeze)
{
    _in_buf = std::to_string(_token_gen) + "-var-set-frozen " + (name + (freeze ? '1' : '0')) +  '\n';
    _work(_token_gen++, result_class::done);
}

void interpreter::var_set_update_range(const std::string & name, int from, int to)
{
    _in_buf = std::to_string(_token_gen) + "-var-set-update-range " + name + " " + std::to_string(from) + " " + std::to_string(to) + '\n';
    _work(_token_gen++, result_class::done);
}

void interpreter::var_set_visualizer(const std::string & name, const boost::optional<std::string> &visualizer)
{
    _in_buf = std::to_string(_token_gen) + "-var-set-visualizer " + name;
    if (visualizer)
        _in_buf += " \"" + *visualizer + "\"\n";
    else
        _in_buf += " None \n";

    _work(_token_gen++, result_class::done);
}

void interpreter::var_set_default_visualizer(const std::string & name)
{
    _in_buf = std::to_string(_token_gen) + "-var-set-visualizer " + name + " gdb.default_visualizer\n";
    _work(_token_gen++, result_class::done);
}


src_and_asm_line interpreter::data_disassemble (disassemble_mode de)
{
    _in_buf = std::to_string(_token_gen) + "-data-disassemble -- " + std::to_string(static_cast<int>(de)) + '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    auto &asm_insns = find(rc.results, "asm_insns");

    if (asm_insns.type() == boost::typeindex::type_id<list>())
           return parse_result<src_and_asm_line>(find(asm_insns.as_list().as_results(), "src_and_asm_line").as_tuple());
    else
    {
        auto in = asm_insns.as_list().as_values();

        std::vector<line_asm_insn> vec;
        vec.reserve(in.size());

        for (auto & i : in)
            vec.push_back(parse_result<line_asm_insn>(i.as_tuple()));

        src_and_asm_line dd;
        dd.line_asm_insn = std::move(vec);

        return dd;
    }
}

src_and_asm_line interpreter::data_disassemble (disassemble_mode de, std::size_t start_addr, std::size_t end_addr)
{
    _in_buf = std::to_string(_token_gen) + "-data-disassemble";
    _in_buf += " -s " + std::to_string(start_addr);
    _in_buf += " -e " + std::to_string(end_addr);
    _in_buf += " -- " + std::to_string(static_cast<int>(de)) + '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    auto &asm_insns = find(rc.results, "asm_insns");

    if (asm_insns.type() == boost::typeindex::type_id<list>())
    {
        auto & list = asm_insns.as_list();
        if (list.type() == boost::typeindex::type_id<std::vector<result>>())
            return parse_result<src_and_asm_line>(find(asm_insns.as_list().as_results(), "src_and_asm_line").as_tuple());
        else
            return src_and_asm_line();
    }

    else
    {
        auto in = asm_insns.as_list().as_values();

        std::vector<line_asm_insn> vec;
        vec.reserve(in.size());

        for (auto & i : in)
            vec.push_back(parse_result<line_asm_insn>(i.as_tuple()));

        src_and_asm_line dd;
        dd.line_asm_insn = std::move(vec);

        return dd;
    }
}

src_and_asm_line interpreter::data_disassemble (disassemble_mode de, const std::string & filename, std::size_t linenum, const boost::optional<int> &lines)
{
    _in_buf = std::to_string(_token_gen) + "-data-disassemble ";
    _in_buf += " -f " + filename;
    _in_buf += " -l " + std::to_string(linenum);

    if (lines)
        _in_buf += " -n " + std::to_string(*lines);

    _in_buf += " -- " + std::to_string(static_cast<int>(de)) + '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    auto &asm_insns = find(rc.results, "asm_insns");

    if (asm_insns.type() == boost::typeindex::type_id<list>())
           return parse_result<src_and_asm_line>(find(asm_insns.as_list().as_results(), "src_and_asm_line").as_tuple());
    else
    {
        auto in = asm_insns.as_list().as_values();

        std::vector<line_asm_insn> vec;
        vec.reserve(in.size());

        for (auto & i : in)
            vec.push_back(parse_result<line_asm_insn>(i.as_tuple()));

        src_and_asm_line dd;
        dd.line_asm_insn = std::move(vec);

        return dd;
    }
}

std::string interpreter::data_evaluate_expression(const std::string & expr)
{
    _in_buf = std::to_string(_token_gen) + "-data-evaluate-expression " + quote_if(expr) + "\n";

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return find(rc.results, "value").as_string();
}


std::vector<std::string> interpreter::data_list_changed_registers()
{
    _in_buf = std::to_string(_token_gen) + "-data-list-changed-registers\n";

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    auto l = find(rc.results, "changed-registers").as_list().as_values();

    std::vector<std::string> ret;
    ret.reserve(l.size());
    for (auto & v : l)
        ret.push_back(v.as_string());

    return ret;
}

std::vector<std::string> interpreter::data_list_register_names()
{
    _in_buf = std::to_string(_token_gen) + "-data-list-register-names\n";

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    auto l = find(rc.results, "register-names").as_list().as_values();

    std::vector<std::string> ret;
    ret.reserve(l.size());
    for (auto & v : l)
        ret.push_back(v.as_string());

    return ret;
}

std::vector<register_value> interpreter::data_list_register_values(
                     format_spec fmt, const boost::optional<std::vector<int>> & regno, bool skip_unavaible)
{
    _in_buf = std::to_string(_token_gen) + "-data-list-register-values ";

    if (skip_unavaible)
        _in_buf += "--skip-unavailable ";

    switch (fmt)
    {
    case format_spec::hexadecimal: _in_buf += "x"; break;
    case format_spec::octal:       _in_buf += "o"; break;
    case format_spec::binary:      _in_buf += "t"; break;
    case format_spec::decimal:     _in_buf += "d"; break;
    case format_spec::raw:         _in_buf += "r"; break;
    case format_spec::natural:     _in_buf += "N"; break;
    default: break;
    }

    if (regno)
        for (auto & v : *regno)
            _in_buf += " " + std::to_string(v);

    _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    auto l = find(rc.results, "register-values").as_list().as_values();

    std::vector<register_value> ret;
    ret.reserve(l.size());
    for (auto & v : l)
        ret.push_back(parse_result<register_value>(v.as_tuple()));

    return ret;
}

read_memory interpreter::data_read_memory(const std::string & address,
                 std::size_t word_size,
                 std::size_t nr_rows,
                 std::size_t nr_cols,
                 const boost::optional<int> & byte_offset,
                 const boost::optional<char> & aschar)
{
    _in_buf = std::to_string(_token_gen) + "-data-read-memory";

    if (byte_offset)
        _in_buf += " -o " + std::to_string(*byte_offset);

    _in_buf += " " + address;
    _in_buf += " x"; //always hex
    _in_buf += " " + std::to_string(word_size);
    _in_buf += " " + std::to_string(nr_rows);
    _in_buf += " " + std::to_string(nr_cols);
    if (aschar)
        (_in_buf += " ") += *aschar;

    _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<read_memory>(rc.results);
}

std::vector<read_memory_bytes> interpreter::data_read_memory_bytes(const std::string & address, std::size_t count, const boost::optional<int> & offset)
{
    _in_buf = std::to_string(_token_gen) + "-data-read-memory-bytes";

    if (offset)
        _in_buf += " -o " + std::to_string(*offset);

    _in_buf += " " + address;
    _in_buf += " " + std::to_string(count);
    _in_buf += '\n';


    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    std::vector<read_memory_bytes> vec;

    auto memory = find(rc.results, "memory").as_list().as_values();
    vec.reserve(memory.size());

    for (auto & mem : memory)
        vec.push_back(parse_result<read_memory_bytes>(mem.as_tuple()));

    return vec;


}

void interpreter::data_write_memory_bytes(const std::string & address, const std::vector<std::uint8_t> & contents, const boost::optional<std::size_t> & count)
{
    _in_buf = std::to_string(_token_gen) + "-data-write-memory-bytes " + address;

    std::stringstream ss;
    ss << " ";

    constexpr static char arr_conv[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    for (auto & c : contents)
        ss << arr_conv[(c & 0xF0) >> 4] << arr_conv[c & 0x0F];

    if (count)
        ss << " " << std::hex << *count;

    ss << std::endl;
    _in_buf += ss.str();

    _work(_token_gen++, result_class::done);
}

boost::optional<found_tracepoint> interpreter::trace_find(boost::none_t)
{
    _in_buf = std::to_string(_token_gen) + "-trace-find none\n";
    _work(_token_gen++, result_class::done);
    return boost::none;
}

boost::optional<found_tracepoint> interpreter::trace_find(const std::string & str)
{
    _in_buf = std::to_string(_token_gen) + "-trace-find " + str + "\n";
    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<boost::optional<found_tracepoint>>(rc.results);
}


boost::optional<found_tracepoint> interpreter::trace_find_by_frame(int frame)
{
    _in_buf = std::to_string(_token_gen) + "-trace-find frame-number " + std::to_string(frame) + "\n";
    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<boost::optional<found_tracepoint>>(rc.results);
}

boost::optional<found_tracepoint> interpreter::trace_find(int number)
{
    _in_buf = std::to_string(_token_gen) + "-trace-find tracepoint-number " + std::to_string(number) + "\n";
    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<boost::optional<found_tracepoint>>(rc.results);
}

boost::optional<found_tracepoint> interpreter::trace_find_at(std::uint64_t addr)
{
    _in_buf = std::to_string(_token_gen) + "-trace-find pc " + std::to_string(addr) + "\n";
    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<boost::optional<found_tracepoint>>(rc.results);
}

boost::optional<found_tracepoint> interpreter::trace_find_inside(std::uint64_t start, std::uint64_t end)
{
    _in_buf = std::to_string(_token_gen) + "-trace-find pc-inside-range " + std::to_string(start) + " " + std::to_string(end) + "\n";
    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<boost::optional<found_tracepoint>>(rc.results);
}

boost::optional<found_tracepoint> interpreter::trace_find_outside(std::uint64_t start, std::uint64_t end)
{
    _in_buf = std::to_string(_token_gen) + "-trace-find pc-outside-range " + std::to_string(start) + " " + std::to_string(end) + "\n";
    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<boost::optional<found_tracepoint>>(rc.results);
}

boost::optional<found_tracepoint> interpreter::trace_find_line(std::size_t & line, const boost::optional<std::string> & file)
{
    _in_buf = std::to_string(_token_gen) + "-trace-find line ";

    if (file)
        _in_buf += *file + ":";
    _in_buf += std::to_string(line) + '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<boost::optional<found_tracepoint>>(rc.results);
}

void interpreter::trace_define_variable(const std::string & name, const boost::optional<std::string> & value)
{
    _in_buf = std::to_string(_token_gen) + "-trace-define-variable " + name ;;

    if (value)
        _in_buf += " " + *value;
    _in_buf += '\n';

    _work(_token_gen++, result_class::done);
}


traceframe_collection interpreter::trace_frame_collected(
                       const boost::optional<std::string> & var_pval,
                       const boost::optional<std::string> & comp_pval,
                       const boost::optional<std::string> & regformat,
                       bool memory_contents)
{
    _in_buf = std::to_string(_token_gen) + "-trace-find line ";

    if (var_pval)
        _in_buf += "--var-print-values "   + *var_pval;
    if (comp_pval)
        _in_buf += "--comp-print-valuess "   + *comp_pval;
    if (regformat)
        _in_buf += "--registers-format "   + *regformat;
    if (memory_contents)
        _in_buf += "--memory-contents";

    _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<traceframe_collection>(rc.results);
}


std::vector<trace_variable> interpreter::trace_list_variables()
{
    _in_buf = std::to_string(_token_gen) + "-trace-list-variable\n";

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    std::vector<trace_variable> vars;

    auto ls = find(rc.results, "body").as_list().as_results();

    vars.resize(ls.size());

    for (auto & l : ls)
        vars.push_back(parse_result<trace_variable>(l.value_.as_tuple()));

    return vars;
}

void interpreter::trace_save(const std::string & filename, bool remote)
{
    _in_buf = std::to_string(_token_gen) + "-trace-save ";

    if (remote)
        _in_buf += "-r ";

    _in_buf += filename + '\n';

    _work(_token_gen++, result_class::done);
}

void interpreter::trace_start()
{
    _in_buf = std::to_string(_token_gen) + "-trace-start\n";
    _work(_token_gen++, result_class::done);
}

struct trace_status interpreter::trace_status()
{
    _in_buf = std::to_string(_token_gen) + "-trace-status\n";

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<struct trace_status>(rc.results);
}

void interpreter::trace_stop()
{
    _in_buf = std::to_string(_token_gen) + "-trace-stop\n";
    _work(_token_gen++, result_class::done);
}

std::vector<symbol_line> interpreter::symbol_list_lines(const std::string & filename)
{
    _in_buf = std::to_string(_token_gen) + "-trace-status\n";

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    auto var = find(rc.results, "lines").as_list().as_values();

    std::vector<symbol_line> res;
    res.reserve(var.size());

    for (auto & v : var)
        res.push_back(parse_result<symbol_line>(v.as_tuple()));

    return res;
}

void interpreter::file_exec_and_symbols(const std::string & file)
{
    _in_buf = std::to_string(_token_gen) + "-file-exec-and-symbols " + file + '\n';
    _work(_token_gen++, result_class::done);
}

void interpreter::file_exec_file(const std::string & file)
{
    _in_buf = std::to_string(_token_gen) + "-file-exec-file " + file + '\n';
    _work(_token_gen++, result_class::done);
}


source_info interpreter::file_list_exec_source_file()
{
    _in_buf = std::to_string(_token_gen) + "-file-list-exec-source-file\n";

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<source_info>(rc.results);
}

std::vector<source_info> interpreter::file_list_exec_source_files()
{
    _in_buf = std::to_string(_token_gen) + "-file-list-exec-source-files\n";

     metal::gdb::mi2::result_output rc;
     _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
            {
                rc = std::move(rc_in);
            });

     if (rc.class_ != result_class::done)
        _throw_unexpected_result(result_class::done, rc);

     auto var = find(rc.results, "files").as_list().as_values();

     std::vector<source_info> res;
     res.reserve(var.size());

     for (auto & v : var)
         res.push_back(parse_result<source_info>(v.as_tuple()));

     return res;
}

void interpreter::file_symbol_file(const std::string & file)
{
    _in_buf = std::to_string(_token_gen) + "-file-symbol-file " + file + '\n';
    _work(_token_gen++, result_class::done);
}

download_info interpreter::target_download()
{
    _in_buf = std::to_string(_token_gen) + "-target-download\n";

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return parse_result<download_info>(rc.results);
}

connection_notification interpreter::target_select(const std::string & type, const std::vector<std::string> & args)
{
    _in_buf = std::to_string(_token_gen) + "-target-select " + type ;

    for (auto & arg : args)
        _in_buf += " " + arg;

    _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::connected)
       _throw_unexpected_result(result_class::connected, rc);

    return parse_result<connection_notification>(rc.results);
}

void interpreter::target_file_put(const std::string & hostfile, const std::string & targetfile)
{
    _in_buf = std::to_string(_token_gen) + "-target-file-put " + hostfile + ' ' + targetfile + '\n';
    _work(_token_gen++, result_class::done);
}

void interpreter::target_file_get(const std::string & targetfile, const std::string & hostfile)
{
    _in_buf = std::to_string(_token_gen) + "-target-file-get " + targetfile + ' ' + hostfile + '\n';
    _work(_token_gen++, result_class::done);
}

void interpreter::target_file_delete(const std::string & targetfile)
{
    _in_buf = std::to_string(_token_gen) + "-target-file-delete " + targetfile + '\n';
    _work(_token_gen++, result_class::done);
}

std::vector<info_ada_exception> interpreter::info_ada_exceptions(const std::string & regexp)
{
    _in_buf = std::to_string(_token_gen) + "-info-ada-exceptions " + regexp + '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    auto entries = find(rc.results, "body").as_list().as_values();
    std::vector<info_ada_exception> vec;
    vec.reserve(entries.size());

    for (auto & e : entries)
        vec.push_back(parse_result<info_ada_exception>(e.as_tuple()));

    return vec;
}

bool interpreter::info_gdb_mi_command(const std::string & cmd_name)
{
    _in_buf = std::to_string(_token_gen) + "-info-gdb-mi-command " + cmd_name + '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);


    return find(rc.results, "exists").as_string() == "true";
}

std::vector<std::string> interpreter::list_features()
{
    _in_buf = std::to_string(_token_gen) + "-list-features\n";

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    auto entries = find(rc.results, "result").as_list().as_values();
    std::vector<std::string> vec;
    vec.reserve(entries.size());

    for (auto & e : entries)
        vec.push_back(e.as_string());

    return vec;
}

std::vector<std::string> interpreter::list_target_features()
{
    _in_buf = std::to_string(_token_gen) + "-list-target-features\n";

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    auto entries = find(rc.results, "result").as_list().as_values();
    std::vector<std::string> vec;
    vec.reserve(entries.size());

    for (auto & e : entries)
        vec.push_back(e.as_string());

    return vec;
}

void interpreter::gdb_exit()
{
    _in_buf = std::to_string(_token_gen) + "-gdb-exit\n";
    _work(_token_gen++, result_class::exit);
}
void interpreter::gdb_set(const std::string & name, const std::string & value)
{
    _in_buf = std::to_string(_token_gen) + "-gdb-set " + name + " " + value + '\n';
    _work(_token_gen++, result_class::done);
}
std::string interpreter::gdb_show(const std::string & name)
{
    _in_buf = std::to_string(_token_gen) + "-gdb-show " + name + '\n';
    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return find(rc.results, "value").as_string();

}
std::string interpreter::gdb_version()
{
    std::string value;
    boost::signals2::scoped_connection conn = _stream_console.connect([&](const std::string & str){value += str; value += '\n';});
    _in_buf = std::to_string(_token_gen) + "-gdb-version\n";
    _work(_token_gen++, result_class::done);

    return value;
}
/**
 * Lists thread groups (see [Thread groups](https://sourceware.org/gdb/onlinedocs/gdb/Thread-groups.html#Thread-groups). When a single thread group is passed as the argument, lists the children of that group. When several thread group are passed, lists information about those thread groups. Without any parameters, lists information about all top-level thread groups.

Normally, thread groups that are being debugged are reported. With the �--available� option, gdb reports thread groups available on the target.

The output of this command may have either a �threads� result or a �groups� result. The �thread� result has a list of tuples as value, with each tuple describing a thread (see [GDB/MI Thread Information](https://sourceware.org/gdb/onlinedocs/gdb/GDB_002fMI-Thread-Information.html#GDB_002fMI-Thread-Information)). The �groups� result has a list of tuples as value, each tuple describing a thread group. If top-level groups are requested (that is, no parameter is passed), or when several groups are passed, the output always has a �groups� result. The format of the �group� result is described below.

To reduce the number of roundtrips it's possible to list thread groups together with their children, by passing the �--recurse� option and the recursion depth. Presently, only recursion depth of 1 is permitted. If this option is present, then every reported thread group will also include its children, either as �group� or �threads� field.

In general, any combination of option and parameters is permitted, with the following caveats:

 - When a single thread group is passed, the output will typically be the �threads� result. Because threads may not contain anything, the �recurse� option will be ignored.
 - When the �--available� option is passed, limited information may be available. In particular, the list of threads of a process might be inaccessible. Further, specifying specific thread groups might not give any performance advantage over listing all thread groups. The frontend should assume that �-list-thread-groups --available� is always an expensive operation and cache the results.

 */

std::vector<groups> interpreter::list_thread_groups(bool available, boost::optional<int> recurse, std::vector<int> groups)
{
    _in_buf = std::to_string(_token_gen) + "-list-thread-groups";
    if (available)
        _in_buf += " --available";
    if (recurse)
        _in_buf += " --recurse " + std::to_string(*recurse);

    for (auto & g : groups)
        _in_buf += " " + std::to_string(g);

    _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    std::vector<value> res;

    if (auto val = find_if(rc.results, "threads"))
        res = std::move(val->as_list().as_values());
    else if (auto val = find_if(rc.results, "groups"))
        res = std::move(val->as_list().as_values());

    std::vector<struct groups> vec;

    vec.reserve(res.size());

    for (auto & r : res)
        vec.push_back(parse_result<struct groups>(r.as_tuple()));

    return vec;
}

/**
 *  If no argument is supplied, the command returns a table of available operating-system-specific information types. If one of these types is supplied as an argument type, then the command returns a table of data of that type.
    The types of information available depend on the target operating system.
 * @param type The type of information to be obtained.
 * @return A table, in which the first row is the
 */
std::vector<std::vector<std::string>> interpreter::info_os(const boost::optional<std::string> & type)
{
    _in_buf = std::to_string(_token_gen) + "-info-os";
    if (type)
        _in_buf += " " + *type;

    _in_buf += '\n';

    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    std::vector<std::string> titles;
    std::vector<std::string> ids;
    {
        auto in = find(rc.results, "hdr").as_list().as_values();
        for (auto & v : in)
        {
            auto tup = v.as_tuple();
            titles.push_back(find(tup, "col_name").as_string());
            ids.   push_back(find(tup, "colhdr").as_string());
        }
    }

    auto body = find(rc.results, "body").as_list().as_values();

    std::vector<std::vector<std::string>> res;
    res.reserve(body.size() + 1);
    res.push_back(std::move(titles));


    for (auto & b : body)
    {
        std::vector<std::string> line;
        line.reserve(ids.size());
        for (auto & id : ids)
            line.push_back(find(b.as_tuple(), id.c_str()).as_string());

        res.push_back(std::move(line));
    }
    return res;
}

/**
 * Creates a new inferior (see [Inferiors and Programs](https://sourceware.org/gdb/onlinedocs/gdb/Inferiors-and-Programs.html#Inferiors-and-Programs)). The created inferior is not associated with any executable. Such association may be established with the �-file-exec-and-symbols� command (see [GDB/MI File Commands](https://sourceware.org/gdb/onlinedocs/gdb/GDB_002fMI-File-Commands.html#GDB_002fMI-File-Commands)). The command response has a single field, �inferior�, whose value is the identifier of the thread group corresponding to the new inferior.
 * @return
 */
std::string interpreter::add_inferior()
{
    _in_buf = std::to_string(_token_gen) + "-add-inferior\n";
    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return find(rc.results, "inferior").as_string();
}

///Execute the specified command in the given interpreter.
void interpreter::interpreter_exec(const std::string & interpreter, const std::string & command)
{
    _in_buf = std::to_string(_token_gen) + "-interpreter-exec " + interpreter + " " + quote_if(command) + '\n';
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output &){});
}

/// Set terminal for future runs of the program being debugged.
void interpreter::inferior_tty_set(const std::string & terminal)
{
    _in_buf = std::to_string(_token_gen) + "-inferior-tty-set " + terminal + '\n';
    _work(_token_gen++, result_class::done);
}

///Show terminal for future runs of program being debugged.
std::string interpreter::inferior_tty_show()
{
    _in_buf = std::to_string(_token_gen) + "-inferior-tty-show\n";
    metal::gdb::mi2::result_output rc;
    _work(_token_gen++, [&](const metal::gdb::mi2::result_output & rc_in)
           {
               rc = std::move(rc_in);
           });

    if (rc.class_ != result_class::done)
       _throw_unexpected_result(result_class::done, rc);

    return find(rc.results, "inferior_tty_terminal").as_string();
}

///Toggle the printing of the wallclock, user and system times for an MI command as a field in its output. This command is to help frontend developers optimize the performance of their code. The default argument is true.
void interpreter::enable_timings(bool enable)
{
    _in_buf = std::to_string(_token_gen) + "-enable-timings ";
    if (enable)
        _in_buf += "yes";
    else
        _in_buf += "no";
    _in_buf += "\n";
    _work(_token_gen++, result_class::done);
}

}
}
}




