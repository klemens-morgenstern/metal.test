/**
 * @file  metal/gdb/mi2/output.hpp
 * @date   13.12.2016
 * @author Klemens D. Morgenstern
 *



 */
#ifndef METAL_GDB_MI2_OUTPUT_HPP_
#define METAL_GDB_MI2_OUTPUT_HPP_

#include <string>
#include <vector>
#include <boost/optional.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/get.hpp>
#include <boost/variant/recursive_wrapper.hpp>
#include <metal/gdb/mi2/interpreter_error.hpp>
#include <cstdint>

namespace metal
{
namespace gdb
{
namespace mi2
{

///Thrown if the interpreter attempts to interpret the value as another type than given.
struct unexpected_type : interpreter_error
{
    using interpreter_error::interpreter_error;
    using interpreter_error::operator=;
};

///Class indicating the result of the gdb operation.
enum class result_class
{
    done,
    running,
    connected,
    error,
    exit
};

///Convert the result_class to the corresponding string for debug purposes.
inline std::string to_string(result_class rc)
{
    switch (rc)
    {
    case result_class::done:      return "done";
    case result_class::running:   return "running";
    case result_class::connected: return "connected";
    case result_class::error:     return "error";
    case result_class::exit:      return "exit";
    default: return "***invalid value***";
    }
}

///A stream record as given used by the mi2 interface.
/** GDB internally maintains a number of output streams: the console, the target, and the log.
 * The output intended for each of these streams is funneled through the GDB/MI interface using stream records.

Each stream record begins with a unique prefix character which identifies its stream (see GDB/MI Output Syntax).
In addition to the prefix, each stream record contains a string-output. This is either raw text (with an implicit new line)
or a quoted C string (which does not contain an implicit newline).

\par console
The console output stream contains text that should be displayed in the CLI console window. It contains the textual responses to CLI commands.

\par target
The target output stream contains any textual output from the running target. This is only present when GDB�s event loop is truly asynchronous, which is currently only the case for remote targets.

\par log
The log stream contains debugging messages being produced by GDB�s internals.
 */
struct stream_record
{
    ///The type of the stream record.
    enum type_t
    {
        console, target, log
    } type;
    std::string content;
};

///Try to parse a string as stream_output.
boost::optional<stream_record> parse_stream_output(const std::string & data);

struct value;

///A value with a key.
struct result
{
    std::string variable;
    std::unique_ptr<value> value_p = std::make_unique<value>(); //shared for spirit.x3
    value & value_ = *value_p;

    result() = default;
    result(const result &);
    result(result &&) = default;

    result & operator=(const result &);
    result & operator=(result &&) = default;
};

///A tuple, i.e. a sequence of results.
struct tuple : std::vector<result>
{
    using father_type = std::vector<result>;
    using father_type::vector;
    using father_type::operator=;
};

struct list;

///A value, i.e. either a string, a tuple or a list.
struct value : boost::variant<std::string, tuple, boost::recursive_wrapper<list>>
{
    using father_type = boost::variant<std::string, tuple, boost::recursive_wrapper<list>>;

    inline value();
    template<typename T>
    inline value(T && str);

    template<typename T>
    value & operator=(T&& rhs);
    ///Try to interpret it as a string. May throw unexpected_type if it is not a string.
    inline const std::string & as_string() const
    {
        if (type() != boost::typeindex::type_id<std::string>())
            BOOST_THROW_EXCEPTION( unexpected_type("unexpected type [" + boost::typeindex::type_index(type()).pretty_name() + " != " + "std::string]") );
        return boost::get<std::string>(*this);
    }
    ///Try to interpret it as a list. May throw unexpected_type if it is not a list.
    inline const list &        as_list()   const;
    ///Try to interpret it as a tuple. May throw unexpected_tuple if it is not a tuple.
    inline const tuple&        as_tuple()  const
    {
        if (type() != boost::typeindex::type_id<tuple>())
            BOOST_THROW_EXCEPTION( unexpected_type("unexpected type [" + boost::typeindex::type_index(type()).pretty_name() + " != " + "metal::gdb::mi2::tuple]") );
        return boost::get<tuple>(*this);
    }
};

///A list, i.e. a sequence of values or results.
struct list : boost::variant<std::vector<value>, std::vector<result>>
{
    using father_type = boost::variant<std::vector<value>, std::vector<result>>;
    using father_type::variant;
    using father_type::operator=;
    ///Try to interpret it as a sequence of values. May throw unexpected_type if it is a sequence of results instead.
    const std::vector<value> & as_values() const
    {
        if (type() != boost::typeindex::type_id<std::vector<value>>())
            BOOST_THROW_EXCEPTION( unexpected_type("unexpected type [" + boost::typeindex::type_index(type()).pretty_name() + " != " + "std::vector<value>]") );
        return boost::get<std::vector<value>>(*this);
    }
    ///Try to interpret it as a sequence of results. May throw unexpected_type if it is a sequence of values instead.
    const std::vector<result> & as_results() const
    {
        if (type() != boost::typeindex::type_id<std::vector<result>>())
            BOOST_THROW_EXCEPTION( unexpected_type("unexpected type [" + boost::typeindex::type_index(type()).pretty_name() + " != " + "std::vector<result>]") );
        return boost::get<std::vector<result>>(*this);
    }
};

const list & value::as_list()   const
{
    if (type() != boost::typeindex::type_id<list>())
        BOOST_THROW_EXCEPTION( unexpected_type("unexpected type [" + boost::typeindex::type_index(type()).pretty_name() + " != " + "metal::gdb::mi2::list]") );

    return boost::get<list>(*this);
}


//declared here because of forward-decl.
value::value() {};

template<typename T>
value::value(T&& value) : father_type(std::forward<T>(value)) {}

template<typename T>
value &value::operator=(T&& rhs) { *static_cast<father_type*>(this) = std::forward<T>(rhs); return *this;}

///Class representing asynchronous output
struct async_output
{
    ///The type of asynchronous output.
    enum type_t
    {
        exec,
        status,
        notify
    } type;
    ///Currently only "stopped", but kept for further extensions.
    std::string class_;
    ///The actual data of the output.
    std::vector<result> results;
};

///The output of a synchronous operation.
struct result_output
{
    ///The class of the output
    result_class class_;
    ///The results of it.
    std::vector<result> results;
};

///Try to parste the string as stream output.
boost::optional<stream_record> parse_stream_output(const std::string & data);

///Try to parse the string as asynchronous output, expecting the passed token.
boost::optional<async_output> parse_async_output(std::uint64_t token, const std::string & value);
///Try to parse the string as asynchronous output, with the token being optional.
boost::optional<std::pair<boost::optional<std::uint64_t>, async_output>> parse_async_output(const std::string & value);
///Parse the end token of an output seqeuence.
bool is_gdb_end(const std::string & data);

///Parse a string as record, expecting the passed token.
boost::optional<result_output> parse_record(std::uint64_t token, const std::string & data);
///Parse a string as record, while the token is optional.
boost::optional<std::pair<boost::optional<std::uint64_t>, result_output>> parse_record(const std::string & data);

std::string to_string(const stream_record & ar);
std::string to_string(const result & res);
std::string to_string(const std::vector<result> & tup);
std::string to_string(const value & val);
std::string to_string(const list & ls);
std::string to_string(const async_output & ao);
std::string to_string(const result_output & ro);


}
}
}

#endif /* METAL_GDB_MI2_OUTPUT_HPP_ */
