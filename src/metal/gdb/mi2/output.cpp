/**
* @file  metal/gdb/mi2/output.cpp
 * @date   13.12.2016
 * @author Klemens D. Morgenstern
 *



 */

#include <metal/gdb/mi2/output.hpp>
#include <boost/algorithm/string/replace.hpp>
#define __assume(Val)
#include <tao/pegtl.hpp>

using namespace tao;

namespace metal
{
namespace gdb
{
namespace mi2
{



result::result(const result & rhs) : variable(rhs.variable) {value_ = rhs.value_;}

result & result::operator=(const result & rhs)
{
    variable = rhs.variable;
    value_   = rhs.value_;
    return *this;
}


namespace parser
{

template< typename Rule >
struct action
   : pegtl::nothing< Rule > {};


template<typename Rule, typename Ptr, Ptr ptr >
struct member : Rule
{
    constexpr static Ptr member_pointer = ptr;

};

// my pegtl extensions
template< typename Rule > struct control : pegtl::normal< Rule >
{
};

template<typename Rule, typename Ptr, Ptr ptr>
struct control<member<Rule, Ptr, ptr>> : pegtl::normal<Rule>
{

    template<pegtl::apply_mode A, pegtl::rewind_mode M, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0, typename ... States>
    static bool match( Input & in, State0 && st0,  States && ... st )
    {
        return control<Rule>::template match<A, M, Action, Control>(in, st0.*ptr, std::forward<States>(st)...);
    }

    template<pegtl::apply_mode A, pegtl::rewind_mode M, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0>
    static bool match( Input & in, State0 && st0)
    {
        return control<Rule>::template match<A, M, Action, Control>(in, st0.*ptr);
    }
};

template<typename Rule>
struct push_back : Rule {};

template<typename Rule>
struct control<push_back<Rule>> : pegtl::normal<Rule>
{

    template<pegtl::apply_mode A, pegtl::rewind_mode M, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0, typename ... States>
    static bool match( Input & in, std::vector<State0> & st0,  States && ... st )
    {
        State0 val;
        auto res = control<Rule>::template match<A, M, Action, Control>(in, val, std::forward<States>(st)...);
        st0.push_back(std::move(val));
        return res;
    }

    template<pegtl::apply_mode A, pegtl::rewind_mode M, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0>
    static bool match( Input & in, std::vector<State0> & st0)
    {
        State0 val;
        auto res = control<Rule>::template match<A, M, Action, Control>(in, val);
        st0.push_back(std::move(val));
        return res;
    }
};


template<typename Rule, typename Type>
struct variant_elem : Rule {};

template<typename Rule, typename Type>
struct control<variant_elem<Rule, Type>> : pegtl::normal<Rule>
{
    template<pegtl::apply_mode A, pegtl::rewind_mode M, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0, typename ... States>
    static bool match( Input & in, State0 & st0,  States && ... st )
    {
        Type val;
        auto res = control<Rule>::template match<A, M, Action, Control>(in, val, std::forward<States>(st)...);
        st0 = std::move(val);
        return res;
    }

    template<pegtl::apply_mode A, pegtl::rewind_mode M, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0>
    static bool match( Input & in, State0 & st0)
    {
        Type val;
        auto res = control<Rule>::template match<A, M, Action, Control>(in, val);
        st0 = std::move(val);
        return res;
    }
};


struct done      : pegtl::string<'d', 'o', 'n', 'e'> {};
struct running   : pegtl::string<'r', 'u', 'n', 'n', 'i', 'n', 'g'> {};
struct connected : pegtl::string<'c', 'o', 'n', 'n', 'e', 'c', 't', 'e', 'd'> {};
struct error     : pegtl::string<'e', 'r', 'r', 'o', 'r'> {};
struct exit      : pegtl::string<'e', 'x', 'i', 't'> {};

template<> struct action<done>      { template<typename T, typename ...Args> static void apply(const T&, result_class & res, Args&&...) { res = result_class::done;      } };
template<> struct action<running>   { template<typename T, typename ...Args> static void apply(const T&, result_class & res, Args&&...) { res = result_class::running;   } };
template<> struct action<connected> { template<typename T, typename ...Args> static void apply(const T&, result_class & res, Args&&...) { res = result_class::connected; } };
template<> struct action<error>     { template<typename T, typename ...Args> static void apply(const T&, result_class & res, Args&&...) { res = result_class::error;     } };
template<> struct action<exit>      { template<typename T, typename ...Args> static void apply(const T&, result_class & res, Args&&...) { res = result_class::exit;      } };

struct result_class : pegtl::sor<done, running, connected, error, exit>
{

};

//struct async_class_rule : pegtl::sor<stopped> {};

struct async_class_rule : pegtl::plus<pegtl::ranges<'A', 'Z', 'a', 'z', '0', '9', '-'>>
{

};

template<> struct action<async_class_rule>
{
    template<typename T, typename ...Args> static void apply(const T& in, std::string & res, Args&&...)
    {
        res = in.string();
    }
};


struct cstring : pegtl::seq<pegtl::one<'"'>, pegtl::star<pegtl::sor<pegtl::string<'\\', '"'>, pegtl::not_one<'"'>>>, pegtl::one<'"'>> {};


template<>
struct action<cstring>
{
   template<typename T, typename ...Args>
   static void apply( const T & in , std::string & data, Args&&...)
   {
       if (in.size() > 2)
       {
           data.assign(in.begin() +1, in.end() - 1);
           boost::replace_all(data, "\\\"", "\"");
           boost::replace_all(data, "\\\\\"", "\\\"");
       }
       else
           data = "";
   }
};


struct stream_record_type : pegtl::one<'~', '@', '&'> {};

template<>
struct action<stream_record_type>
{
   template<typename T>
   static void apply( const T & in , stream_record::type_t & data)
   {
       switch (*in.begin())
       {
       case '~':
           data = stream_record::console;
       break;
       case '@':
           data = stream_record::target;
       break;
       case '&':
           data = stream_record::log;
       break;
       }
   }
};

struct stream_output : pegtl::seq<
                        member<stream_record_type, decltype(&stream_record::type),    &stream_record::type>,
                        member<cstring,            decltype(&stream_record::content), &stream_record::content>,
                        pegtl::opt<pegtl::one<'\r'>, pegtl::eof>>
{

};


struct async_output_type : pegtl::one<'*', '+', '='> {};

template<>
struct action<async_output_type>
{
   template<typename T, typename ...Args>
   static void apply( const T & in , async_output::type_t & data, Args&&...)
   {
       switch (*in.begin())
       {
       case '*':
           data = async_output::exec;
       break;
       case '+':
           data = async_output::status;
       break;
       case '=':
           data = async_output::status;
       break;
       }
   }
};



struct variable : pegtl::plus<pegtl::not_one<'='>> {};

template<>
struct action<variable>
{
   thread_local static std::string last_var_name; //since a tuple may have unnamed follow-ups.
   template<typename T>
   static void apply( const T & in , std::string & data)
   {
       data = in.string();
       last_var_name = in.string();
   }
   template<typename T, typename ...Args>
   static void apply( const T & in , std::string & data, Args && ...)
   {
       data = in.string();
       last_var_name = in.string();
   }
};

thread_local std::string action<variable>::last_var_name = "";

struct inherited_name : pegtl::success {};

template<>
struct action<inherited_name>
{
    template<typename T>
    static void apply(const T& , std::string & data)
    {
        data = action<variable>::last_var_name;
    }
    template<typename T, typename ...Args>
    static void apply(const T& , std::string & data, Args && ...)
    {
        data = action<variable>::last_var_name;
    }
};

struct value_rule;

template<typename Rule>
struct unique_ptr : Rule {};

template<typename Rule> struct control<unique_ptr<Rule>> : pegtl::normal<Rule>
{
    template<pegtl::apply_mode A, pegtl::rewind_mode M, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0, typename ... States>
    static bool match( Input & in, std::unique_ptr<State0> & st0,  States && ... st )
    {
        if (!st0)
            st0 = std::make_unique<State0>();

        return control<Rule>::template match<A, M, Action, Control>(in, *st0, std::forward<States>(st)...);
    }
    template<pegtl::apply_mode A, pegtl::rewind_mode M, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State>
    static bool match( Input & in, std::unique_ptr<State> & st0)
    {
        if (!st0)
            st0 = std::make_unique<State>();

        return control<Rule>::template match<A, M, Action, Control>(in, *st0);
    }
};


struct result_rule : pegtl::seq<
                      member<variable,      decltype(&result::variable), &result::variable>,
                      pegtl::one<'='>,
                      member<unique_ptr<value_rule>,    decltype(&result::value_p),    &result::value_p>>
{
};

struct anonym_result_rule :
                pegtl::seq<
                    member<inherited_name,          decltype(&result::variable), &result::variable>,
                    member<unique_ptr<value_rule>,  decltype(&result::value_p),&result::value_p>>
{

};

struct tuple_rule : pegtl::seq<pegtl::one<'{'> , pegtl::list<push_back<result_rule>, pegtl::one<','>>, pegtl::one<'}'>> {};


struct list_rule : pegtl::sor<
                    pegtl::string<'[', ']'>,
                    pegtl::seq<pegtl::one<'['>, variant_elem<pegtl::list<push_back<result_rule>, pegtl::one<','>>, std::vector<result>>, pegtl::one<']'>>,
                    pegtl::seq<pegtl::one<'['>, variant_elem<pegtl::list<push_back<value_rule >, pegtl::one<','>>, std::vector<value >>, pegtl::one<']'>>> {};

struct value_rule : pegtl::sor<
                        variant_elem<cstring, std::string>,
                        variant_elem<tuple_rule, tuple>,
                        variant_elem<list_rule,  list>
                    > {};


struct async_output_rule : pegtl::seq<
                            member<async_output_type,  decltype(&async_output::type), &async_output::type>,
                            member<async_class_rule, decltype(&async_output::class_), &async_output::class_>,
                            member<pegtl::star<pegtl::one<','>, push_back<result_rule>>,  decltype(&async_output::results), &async_output::results>,
                            pegtl::opt<pegtl::one<'\r'>>, pegtl::eof> {};


struct result_record_rule : pegtl::seq<
                            pegtl::one<'^'>,
                            member<result_class, decltype(&result_output::class_), &result_output::class_>,
                            member<
                                pegtl::star<
                                    pegtl::one<','>,
                                    pegtl::seq<
                                        push_back<result_rule>,
                                        pegtl::star<
                                            pegtl::one<','>,
                                            push_back<
                                                anonym_result_rule
                                                >
                                        >
                                    >>, decltype(&result_output::results), &result_output::results>,
                            pegtl::opt<pegtl::one<'\r'>>, pegtl::eof> {};


struct token : pegtl::plus<pegtl::digit>
{

};


inline unsigned long long my_stoull(const std::string & st)
{
    try {
        return std::stoull(st);
    }
    catch (std::invalid_argument & ia)
    {
        BOOST_THROW_EXCEPTION(parser_error("stoull - invalid argument '" + st + "'"));
    }
    catch (std::out_of_range & oor)
    {
        BOOST_THROW_EXCEPTION(parser_error("stoull - out of range '" + st + "'"));
    }
    return 0ull;
}


template<>
struct control<token> : pegtl::normal<token>
{
    template<pegtl::apply_mode A, pegtl::rewind_mode M, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0, typename ... States>
    static bool match( Input & in, State0 && st0,  const std::uint64_t &tk, States && ... st )
    {
        auto beg = in.current();
        auto res = pegtl::normal<token>::template match<A, M, Action, Control>(in, st0, std::forward<States>(st)...);
        if (res)
        {
            auto end = in.current();
            auto val = my_stoull(std::string(beg, end));
            return val == tk;
        }
        return false;
    }

    template<pegtl::apply_mode A, pegtl::rewind_mode M, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0>
    static bool match( Input & in, State0 && st0, const std::uint64_t &tk)
    {
        auto beg = in.current();
        auto res = pegtl::normal<token>::template match<A, M, Action, Control>(in, st0);
        if (res)
        {
            auto end = in.current();
            auto val = my_stoull(std::string(beg, end));
            return val == tk;

        }
        return false;
    }

    template<pegtl::apply_mode A, pegtl::rewind_mode M, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0, typename ... States>
    static bool match( Input & in, State0 && st0,  boost::optional<std::uint64_t> &tk, States && ... st )
    {
        auto beg = in.current();
        auto res = pegtl::normal<token>::template match<A, M, Action, Control>(in, st0, std::forward<States>(st)...);
        if (res)
        {
            auto end = in.current();
            tk = my_stoull(std::string(beg, end));
            return true;
        }
        return false;
    }

    template<pegtl::apply_mode A, pegtl::rewind_mode M, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename State0>
    static bool match( Input & in, State0 && st0, boost::optional<std::uint64_t> &tk)
    {
        auto beg = in.current();
        auto res = pegtl::normal<token>::template match<A, M, Action, Control>(in, st0);
        if (res)
        {
            auto end = in.current();
            tk = my_stoull(std::string(beg, end));
            return true;

        }
        return false;
    }
};

}


boost::optional<stream_record> parse_stream_output(const std::string & data)
{
    stream_record sr;
    pegtl::memory_input<> mi{data, "gdb-input-stream"};
    if (pegtl::parse<parser::stream_output, parser::action, parser::control>(mi, sr))
        return sr;
    else
        return boost::none;
}

boost::optional<async_output> parse_async_output(std::uint64_t token, const std::string & data)
{
    using type = pegtl::seq<parser::token, parser::async_output_rule>;
    async_output res;

    pegtl::memory_input<> mi{data, "gdb-input-stream"};
    if (pegtl::parse<type, parser::action, parser::control>(mi, res, token))
        return res;
    else
        return boost::none;
}

boost::optional<std::pair<boost::optional<std::uint64_t>, async_output>> parse_async_output(const std::string & data)
{
    using type = pegtl::seq<
                     pegtl::opt<parser::token>,
                     parser::async_output_rule>;
    boost::optional<std::uint64_t> token;
    async_output res;

    pegtl::memory_input<> mi{data, "gdb-input-stream"};
    if (pegtl::parse<type, parser::action, parser::control>(mi, res, token))
    {
        return std::make_pair(token, res);
    }
    else
        return boost::none;
}

bool is_gdb_end(const std::string & data)
{
    using type = pegtl::seq<pegtl::string<'(', 'g', 'b', 'd', ')'>, pegtl::opt<pegtl::one<'\r'>>>;
    pegtl::memory_input<> mi{data, "gdb-input-stream"};
    return pegtl::parse<type>(mi);
}


boost::optional<result_output> parse_record(std::uint64_t token, const std::string & data)
{
    using type = pegtl::seq<parser::token, parser::result_record_rule>;
    result_output res;

    pegtl::memory_input<> mi{data, "gdb-input-stream"};
    if (pegtl::parse<type, parser::action, parser::control>(mi, res, token))
        return res;
    else
        return boost::none;
}

boost::optional<std::pair<boost::optional<std::uint64_t>, result_output>> parse_record(const std::string & data)
{
    using type = pegtl::seq<
                     pegtl::opt<parser::token>,
                     parser::result_record_rule>;
    boost::optional<std::uint64_t> token;
    result_output res;

    pegtl::memory_input<> mi{data, "gdb-input-stream"};
    if (pegtl::parse<type, parser::action, parser::control>(mi, res, token))
    {
        return std::make_pair(token, res);
    }
    else
        return boost::none;
}

std::string to_string(const stream_record & ar)
{
    std::string ret;
    switch (ar.type)
    {
    case stream_record::console: ret = "~"; break;
    case stream_record::log:     ret = "&"; break;
    case stream_record::target:  ret = "@"; break;
    }

    ret += ar.content;
    return ret;
}
std::string to_string(const result & res)
{
    std::string ret;
    ret = res.variable;
    ret += "=";
    ret += to_string(res.value_);
    return ret;
}

std::string to_string(const std::vector<result> & tup)
{
    std::string ret = "{";

    for (auto & t: tup)
    {
        if (&t != &tup.front())
            ret += ", ";

        ret += to_string(t);
    }

    ret += "}";
    return ret;
}
std::string to_string(const value & val)
{
    struct vis_t : boost::static_visitor<std::string>
    {
        std::string operator()(const std::string & str) const {return str;}
        std::string operator()(const tuple & tup) const {return to_string(tup);}
        std::string operator()(const list & lst) const {return to_string(lst);}
    } vis;
    return boost::apply_visitor(vis, val);
}

struct list_t : boost::static_visitor<std::string>
{
    template<typename T>
    std::string operator()(const T & tup) const
    {
        std::string ret = "[";
        for (auto & t: tup)
        {
            if (&t != &tup.front())
                ret += ", ";
            ret += to_string(t);
        }
        ret += "]";
        return ret;
    }
};

std::string to_string(const list & ls)
{
    list_t list;
    return boost::apply_visitor(list, ls);
}
std::string to_string(const async_output & ao)
{
    std::string ret;
    switch (ao.type)
    {
    case async_output::exec: ret = "*";
    case async_output::notify: ret = "+";
    case async_output::status: ret = "=";
    }


    ret += ao.class_ + ", ";
    ret += to_string(ao.results);

    return ret;
}
std::string to_string(const result_output & ro)
{
    return to_string(ro) + ", " + to_string(ro.results);
}


}
}
}
