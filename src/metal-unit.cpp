/**
 * @file   /gdb-runner/src/mw-test-backend.cpp
 * @date   01.08.2016
 * @author Klemens D. Morgenstern
 *

 */

#include <boost/dll/alias.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <boost/system/api_config.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/trim_all.hpp>
#include <boost/program_options.hpp>
#include <metal/debug/break_point.hpp>
#include <metal/debug/frame.hpp>
#include <metal/debug/plugin.hpp>

#include <iostream>
#include <fstream>

#include "unit/sink.hpp"

using namespace metal::debug;
using namespace std;

data_sink_t *data_sink = nullptr;

template<typename Lambda>
class l_vis : public boost::static_visitor<void>
{
    Lambda _l;
public:
    l_vis(l_vis &&) = default;
    l_vis(Lambda && l) : _l(std::move(l)) {}

    template<typename ...Args>
    void operator()(Args &&...args) const { _l(std::forward<Args>(args)...);}
};

template<typename Lambda>
l_vis<Lambda> make_vis(Lambda && l) {return l_vis<Lambda>(std::forward<Lambda>(l)); }


struct test_descr
{
    std::string value;
    test_descr(const std::string & val) : value(val) {}
    test_descr(std::string && val) : value(std::move(val)) {}
};

inline ostream& operator<<(ostream & os, const test_descr & p) { return os << " [" << p.value << "] "; }

var print_from_frame(frame & fr, std::size_t frm, const std::string & id)
{
    fr.select(frm);
    var v;
    try { v = fr.print(id); } catch (metal::debug::interpreter_error&) {v.value = "**cannot obtain value**";}
    fr.select(0);
    return v;
}

var print_from_frame(frame & fr, bool bit_wise, std::size_t frm, std::string && id)
{
    fr.select(frm);
    var v;
    try { v = fr.print(id, bit_wise); } catch (metal::debug::interpreter_error&) {v.value = "**cannot obtain value**";}
    fr.select(0);
    return v;
}

template<typename ...Args>
array<var, sizeof...(Args)> print_from_frame(frame & fr, bool bit_wise, std::size_t frm, Args && ... args)
{
    fr.select(frm);
    array<var, sizeof...(Args)> v;

    try
    {
        v = {fr.print(args, bit_wise)...};
    }
    catch (metal::debug::interpreter_error &)
    {
        for (auto & vv : v)
            vv.value = "**cannot obtain value**";
    }

    fr.select(0);
    return v;
}

string str(frame & fr, size_t idx)
{
    return fr.get_cstring(idx + 4);
}

template<typename ...Args>
array<var, sizeof...(Args)> print_from_passed_id(frame & fr, Args&& ... args)
{
    return print_from_frame(fr, 1, str(fr, args)...);
}


var print_from_passed_id(frame & fr, std::size_t idx)
{
    return print_from_frame(fr, 1, str(fr, idx));
}

string  operation  (frame & fr) { return  fr.arg_list(1).value; }
level_t level      (frame & fr) { return (fr.arg_list(0).value == "__metal_level_assert") ? level_t::assertion : level_t::expect;}
bool    condition  (frame & fr) { return stoi(fr.arg_list(2).value) != 0; }
bool    bitwise    (frame & fr) { return stoi(fr.arg_list(3).value) != 0; }
string  file       (frame & fr) { return fr.get_cstring(7); }
int     line       (frame & fr) { return stoi(fr.arg_list(8).value); }
bool    is_critical(frame & fr) 
{ 
    static bool has_no_critical = false;
    if (has_no_critical)
        return false;
    try {
        return stoi(fr.print("__metal_critical").value) != 0; 
    }
    catch (metal::debug::interpreter_error &)
    {
        has_no_critical = true;
        return false;
    }
}
void    set_error  (frame & fr) { fr.set("__metal_errored", "1"); }


string value(const std::string & value)
{
    auto trimmed = boost::algorithm::trim_all_copy(value);

    auto itr = find_if(trimmed.begin(), trimmed.end(), [](char c)
            {
                return isspace(c);
            });

    if (itr == trimmed.end()) //no whitespace/linebrake, so I need no markers.
        return trimmed;
    else
    {
        boost::replace_all(trimmed, "\n\r", " ");
        return '`' + trimmed + '`';
    }
}


struct statistic
{
    int executed = 0;
    int errors   = 0;
    int warnings = 0;
};

inline statistic & operator+=(statistic & lhs, const statistic & rhs)
{
    lhs.executed += rhs.executed;
    lhs.errors   += rhs.errors;
    lhs.warnings += rhs.warnings;
    return lhs;
}

struct error_handler : statistic
{
    virtual ~error_handler() = default;
    virtual void cancel(frame & fr);

    template<typename Func, typename ...Args>
    void check(frame & fr, Func f, Args &&...args);

    template<typename ...Args>
    void log(frame & fr, Args &&...args)
    {
        print_impl(cerr, file(fr), '(', line(fr), ") log: ", std::forward<Args>(args)...);
    }

    template<typename ...Args>
    void note(frame & fr, Args &&...args)
    {
        print_impl(cerr, file(fr), '(', line(fr), ") note: ", std::forward<Args>(args)...);
    }
};

void error_handler::cancel(frame & fr)
{

    fr.set("__metal_critical", "0");
    auto bt = fr.backtrace();
    auto itr = find_if(bt.cbegin(), bt.cend(), [](const backtrace_elem & elem)
            {
                return boost::algorithm::trim_copy(elem.func) == "__metal_call";
            });

    if (itr != bt.cend()) //ok, I am in a group
    {
        data_sink->cancel_func(file(fr), line(fr), "__metal_call", executed, warnings, errors);

        fr.select(itr->cnt);
        fr.return_();
        return;
    }
    //ok, then go to main.
    itr = find_if(bt.cbegin(), bt.cend(), [](const backtrace_elem & elem)
            {
                return boost::algorithm::trim_copy(elem.func) == "main";
            });

    if (itr != bt.cend()) //ok, I am in main, maybe.
    {
        if (itr->cnt == 1) //directly called from main, cancel everything
        {
            data_sink->cancel_main(file(fr), line(fr), executed, warnings, errors);

            fr.select(itr->cnt);
            fr.return_("0");
        }
        else
        {
            data_sink->continue_main(file(fr), line(fr), executed, warnings, errors);

            fr.select(itr->cnt - 1);
            fr.return_();
        }
        return;
    }
}

template<typename Func, typename ...Args>
void error_handler::check(frame& fr, Func f, Args && ... args)
{
    executed++;
    auto lvl  = level(fr);
    auto cond = condition(fr);
    auto crit = is_critical(fr);

    (data_sink->*f)(file(fr), line(fr), cond, lvl, crit, -1, std::forward<Args>(args)...);

    if (!cond)
    {
        if (lvl == level_t::assertion)
        {
            errors++;
            if (crit)
            {
                set_error(fr);
                cancel(fr);
            }
        }
        else
        {
            warnings++;
            if (crit)
                cancel(fr);
        }
    }
}

struct free_t  : error_handler
{

};

struct session_t;

struct case_t  : error_handler
{
    case_t(const case_t & ) = default;
    case_t(case_t && ) = default;

    case_t& operator=(const case_t & ) = default;
    case_t& operator=(case_t && ) = default;

    case_t(session_t & sess, const std::string & id) : sess(&sess), id(id) {}

    session_t * sess;
    std::string id;
    void cancel(frame & fr) override;
};

struct range_t : error_handler
{
    boost::variant<free_t*, case_t*> father;

    range_t(free_t & f) : father(&f) {}
    range_t(case_t & f) : father(&f) {}

    range_t(const range_t&) = default;
    range_t(range_t&&) = default;

    range_t& operator=(const range_t&) = default;
    range_t& operator=(range_t&&) = default;

    void cancel(frame & fr) override { critical_fail = true; }

    void enter(frame & fr)
     {
         if (condition(fr))
             data_sink->enter_range(file(fr), line(fr), str(fr, 0));
         else
         {
             auto lhs_id = str(fr, 1);
             auto rhs_id = str(fr, 2);
             fr.select(1); //get the value of distances
             auto lhs = fr.print(lhs_id).value;
             auto rhs = fr.print(rhs_id).value;
             fr.select(0);

             data_sink->enter_range_mismatch(file(fr), line(fr), str(fr,0), lhs, rhs);
         }
     }
     void exit(frame& fr)
     {
        data_sink->exit_range(file(fr), line(fr), executed, warnings, errors);

        auto vis = make_vis([this](statistic * st)
                {
                    st->executed++;
                    if (this->errors)
                        st->errors++;
                    if (this->warnings)
                        st->warnings++;
                });

        father.apply_visitor(vis);


        if (critical_fail)
        {
            auto vis = make_vis([&](auto & v){v->cancel(fr);});
            father.apply_visitor(vis);
        }

     }

    ~range_t()
    {
    }

    template<typename Func, typename ...Args>
    void check(frame& fr, Func f, Args && ... args);

    bool critical_fail = false;
    int index = 0;
};


template<typename Func, typename ...Args>
void range_t::check(frame& fr, Func f, Args && ... args)
{
    executed++;
    auto lvl  = level(fr);
    auto cond = condition(fr);
    auto crit = is_critical(fr);

    (data_sink->*f)(file(fr), line(fr), cond, lvl, crit, index, std::forward<Args>(args)...);

    if (!cond)
    {
        if (lvl == level_t::assertion)
        {
            errors++;

            if (is_critical(fr))
            {
                set_error(fr);
                cancel(fr);
            }

       }
        else
        {
            warnings++;

            if (is_critical(fr))
                cancel(fr);
        }
    }
    index++;
}

struct summary_t : statistic
{
    summary_t()
    {
        data_sink->start();
    }

    ~summary_t()
    {

    }
};


struct result_sink :  boost::variant<free_t*, range_t*, case_t*>
{
    using base_type = boost::variant<free_t*, range_t*, case_t*>;

    result_sink(free_t  & f) : base_type(&f) {}
    result_sink(range_t & f) : base_type(&f) {}
    result_sink(case_t  & f) : base_type(&f) {}


    result_sink& operator=(free_t  & f) { base_type::operator=(&f); return *this;}
    result_sink& operator=(range_t & f) { base_type::operator=(&f); return *this;}
    result_sink& operator=(case_t  & f) { base_type::operator=(&f); return *this;}


    void add_errors(int cnt = 1)
    {
        auto vis = make_vis([&](auto & v){v->errors += cnt;});
        apply_visitor(vis);
    }
    void add_warnings(int cnt = 1)
    {
        auto vis = make_vis([&](auto & v){v->warnings += cnt;});
        apply_visitor(vis);
    }
    void add_executed(int cnt = 1)
    {
        auto vis = make_vis([&](auto & v){v->executed += cnt;});
        apply_visitor(vis);
    }

    template<typename ...Args>
    void log(frame & fr, Args&&...args)
    {
        auto vis = make_vis([&](auto & v){v->log(fr, args...);});
        apply_visitor(vis);
    }

    template<typename ...Args>
    void check(frame & fr, Args&&...args)
    {
        auto vis = make_vis([&](auto & v){v->check(fr, args...);});
        apply_visitor(vis);
    }

    template<typename ...Args>
    void note(frame & fr, Args && ... args)
    {
        auto vis = make_vis([&](auto & v){v->note(fr, args...);});
        apply_visitor(vis);
    }

    void cancel(frame & fr)
    {
        auto vis = make_vis([&](auto & v){v->cancel(fr);});
        apply_visitor(vis);
    }
};

bool no_exit_code = true;

struct session_t
{

    free_t free; //free test, if nothing is selected
    boost::optional<case_t> case_; //if I'm in a ranged test.
    boost::optional<range_t> range;

    result_sink sink = free;
    summary_t summary;

    void enter_case   (frame & fr);
    void exit_case    (frame & fr);
    void enter_ranged (frame & fr);
    void exit_ranged  (frame & fr);
    void log          (frame & fr) { data_sink->log(file(fr), line(fr), str(fr, 0)); }
    void checkpoint   (frame & fr) { data_sink->checkpoint(file(fr), line(fr)); }
    void message      (frame & fr) { sink.check(fr, &data_sink_t::message, str(fr, 0)); }
    void plain        (frame & fr) { sink.check(fr, &data_sink_t::plain, str(fr, 0)); }
    void predicate    (frame & fr) { sink.check(fr, &data_sink_t::predicate, str(fr, 0), str(fr, 1)); }
    void equal        (frame & fr)
    {
        auto lhs = str(fr, 0);
        auto rhs = str(fr, 1);
        auto bw  = bitwise(fr);
        auto vals = print_from_frame(fr, bw, 1, lhs, rhs);

        sink.check(fr, &data_sink_t::equal, bw, lhs, rhs, vals[0].value, vals[1].value);
    }
    void not_equal    (frame & fr)
    {
        auto lhs = str(fr, 0);
        auto rhs = str(fr, 1);
        auto bw  = bitwise(fr);
        auto vals = print_from_frame(fr, bw, 1, lhs, rhs);
        sink.check(fr, &data_sink_t::not_equal, bw, lhs, rhs, vals[0].value, vals[1].value);
    }
    void close        (frame & fr)
    {
        auto lhs = str(fr, 0);
        auto rhs = str(fr, 1);
        auto tolerance = str(fr, 2);
        auto vals = print_from_frame(fr, false, 1, lhs, rhs, tolerance);

        sink.check(fr, &data_sink_t::close, lhs, rhs, tolerance, vals[0].value, vals[1].value, vals[2].value);
    }
    void close_rel    (frame & fr)
    {
        auto lhs = str(fr, 0);
        auto rhs = str(fr, 1);
        auto tolerance = str(fr, 2);
        auto vals = print_from_frame(fr, false, 1, lhs, rhs, tolerance);

        sink.check(fr, &data_sink_t::close_rel, lhs, rhs, tolerance, vals[0].value, vals[1].value, vals[2].value);
   }
    void close_perc   (frame & fr)
    {
        auto lhs = str(fr, 0);
        auto rhs = str(fr, 1);
        auto tolerance = str(fr, 2);
        auto vals = print_from_frame(fr, false, 1, lhs, rhs, tolerance);

        sink.check(fr, &data_sink_t::close_per, lhs, rhs, tolerance, vals[0].value, vals[1].value, vals[2].value);
    }
    void ge           (frame & fr)
    {
        auto lhs = str(fr, 0);
        auto rhs = str(fr, 1);
        auto bw  = bitwise(fr);
        auto vals = print_from_frame(fr, bw, 1, lhs, rhs);

        sink.check(fr, &data_sink_t::ge, bw, lhs, rhs, vals[0].value, vals[1].value);
    }
    void greater      (frame & fr)
    {
        auto lhs = str(fr, 0);
        auto rhs = str(fr, 1);

        auto vals = print_from_frame(fr, false, 1, lhs, rhs);

        sink.check(fr, &data_sink_t::greater,  lhs, rhs, vals[0].value, vals[1].value);
    }
    void le           (frame & fr)
    {
        auto lhs = str(fr, 0);
        auto rhs = str(fr, 1);
        auto bw  = bitwise(fr);
        auto vals = print_from_frame(fr, bw, 1, lhs, rhs);

        sink.check(fr, &data_sink_t::le, bw, lhs, rhs, vals[0].value, vals[1].value);
    }
    void lesser       (frame & fr)
    {
        auto lhs = str(fr, 0);
        auto rhs = str(fr, 1);
        auto vals = print_from_frame(fr, false, 1, lhs, rhs);

        sink.check(fr, &data_sink_t::lesser, lhs, rhs, vals[0].value, vals[1].value);
    }
    void exception    (frame & fr)
    {
        sink.check(fr, &data_sink_t::exception, str(fr,0), str(fr,1));
    }
    void any_exception(frame & fr)
    {
        sink.check(fr, &data_sink_t::any_exception);
    }
    void no_exception (frame & fr)
    {
        sink.check(fr, &data_sink_t::no_exception);
    }
    void no_exec      (frame & fr)
    {
        sink.check(fr, &data_sink_t::no_execute);
    }
    void exec         (frame & fr)
    {
        sink.check(fr, &data_sink_t::execute);
    }

    void report       (frame & fr)
    {
        summary += free;

        data_sink->report(
                   free.executed,    free.warnings,    free.errors,
                summary.executed, summary.warnings, summary.errors);

        if (no_exit_code && summary.errors)
        {
            fr.select(1);
            fr.return_("0");
        }
    }
};

void session_t::enter_case   (frame & fr)
{
    auto id = str(fr, 0);
    case_ = case_t{*this, id};

    sink = *case_;

    data_sink->enter_case(file(fr), line(fr), id);
}

void session_t::exit_case    (frame & fr)
{
    auto id = str(fr, 0);

    data_sink->exit_case(file(fr), line(fr), id, case_->executed, case_->warnings, case_->errors);

    summary += *case_;

    sink = free;
    case_ = boost::none;


}

void session_t::enter_ranged (frame & fr)
{
    if (sink.type() == boost::typeindex::type_id<case_t*>())
        range = range_t(*boost::get<case_t*>(sink));
    else if (sink.type() == boost::typeindex::type_id<free_t*>())
        range = range_t(*boost::get<free_t*>(sink));
    else
    {
        std::cerr << file(fr) << '(' << line(fr) << ") critical error: "
                "Twice enter into ranged test, check your test!!" << std::endl;
        return;
    }
    range->enter(fr);
    sink = *range;

}

void session_t::exit_ranged  (frame & fr)
{
    range->exit(fr);

    auto vis = make_vis([&](auto & val){this->sink = *val;});
    range->father.apply_visitor(vis);
    range = boost::none;
}


void case_t::cancel(frame & fr)
{

    fr.set("__metal_critical", "0");
    auto bt = fr.backtrace();
    auto itr = find_if(bt.cbegin(), bt.cend(), [](const backtrace_elem & elem)
            {
                return boost::algorithm::trim_copy(elem.func) == "__metal_call";
            });

    if (itr != bt.cend()) //ok, I am in a group
    {

        fr.select(itr->cnt);
        auto name = fr.print("msg").cstring.value;

        if (name.empty())
            name = "**unnamed**";

        data_sink->cancel_case(name, executed, warnings, errors);

        fr.return_();
    }
    else
        error_handler::cancel(fr);

    sess->summary += *this;
    //reset the pointer.
    sess->sink = sess->free;
    sess->case_ = boost::none;
}

struct metal_test_backend : break_point
{

    session_t session;

    metal_test_backend() : break_point("__metal_impl")
    {
    }

    void invoke(frame & fr, const string & file, int line) override
    {
        auto oper = fr.arg_list(1).value;

        if (oper == "__metal_oper_enter_case"        ) session.enter_case   (fr);
        else if (oper == "__metal_oper_exit_case"    ) session.exit_case    (fr);
        else if (oper == "__metal_oper_enter_ranged" ) session.enter_ranged (fr);
        else if (oper == "__metal_oper_exit_ranged"  ) session.exit_ranged  (fr);
        else if (oper == "__metal_oper_log"          ) session.log          (fr);
        else if (oper == "__metal_oper_checkpoint"   ) session.checkpoint   (fr);
        else if (oper == "__metal_oper_message"      ) session.message      (fr);
        else if (oper == "__metal_oper_plain"        ) session.plain        (fr);
        else if (oper == "__metal_oper_predicate"    ) session.predicate    (fr);
        else if (oper == "__metal_oper_equal"        ) session.equal        (fr);
        else if (oper == "__metal_oper_not_equal"    ) session.not_equal    (fr);
        else if (oper == "__metal_oper_close"        ) session.close        (fr);
        else if (oper == "__metal_oper_close_rel"    ) session.close_rel    (fr);
        else if (oper == "__metal_oper_close_perc"   ) session.close_perc   (fr);
        else if (oper == "__metal_oper_ge"           ) session.ge           (fr);
        else if (oper == "__metal_oper_greater"      ) session.greater      (fr);
        else if (oper == "__metal_oper_le"           ) session.le           (fr);
        else if (oper == "__metal_oper_lesser"       ) session.lesser       (fr);
        else if (oper == "__metal_oper_exception"    ) session.exception    (fr);
        else if (oper == "__metal_oper_any_exception") session.any_exception(fr);
        else if (oper == "__metal_oper_no_exception" ) session.no_exception (fr);
        else if (oper == "__metal_oper_no_exec"      ) session.no_exec      (fr);
        else if (oper == "__metal_oper_exec"         ) session.exec         (fr);
        else if (oper == "__metal_oper_report"       ) session.report       (fr);

    }
};

std::string sink_file;  
std::string format;
boost::optional<std::ofstream> fstr;

std::ostream * sink_str = & std::cout;


void metal_dbg_setup_bps(vector<unique_ptr<metal::debug::break_point>> & bps)
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

    bps.push_back(make_unique<metal_test_backend>());
}


void metal_dbg_setup_options(boost::program_options::options_description & op)
{
    namespace po = boost::program_options;
    op.add_options()
                   ("metal-test-no-exit-code", po::bool_switch(&no_exit_code), "disable exit-code")
                   ("metal-test-sink",         po::value<string>(&sink_file),  "test data sink")
                   ("metal-test-format",       po::value<string>(&format),     "format [hrf, json]")
                   ;
}
