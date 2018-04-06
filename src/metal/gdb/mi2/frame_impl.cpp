/**
 * @file   metal/gdb/mi2/frame_impl.cpp
 * @date   01.08.2016
 * @author Klemens D. Morgenstern
 *



 */

#include <metal/gdb/mi2/frame_impl.hpp>
#include <metal/gdb/mi2/interpreter.hpp>
#include <algorithm>
#include <iostream>
#include <boost/algorithm/string.hpp>

#define __assume(val)
#include <tao/pegtl.hpp>

#include <iostream>
#include <typeinfo>

using namespace tao;
using namespace std;

namespace metal { namespace gdb { namespace mi2 {

namespace parser
{

template< typename Rule >
struct action
   : pegtl::nothing< Rule > {};


struct ellipsis  : pegtl::string<'.', '.', '.'> {};

template<>
struct action<ellipsis> : pegtl::normal<ellipsis>
{
    template<typename Input>
    static void apply(const Input & input, metal::debug::var & v)
    {
        v.cstring.ellipsis = true;
    }
};


struct cstring_body : pegtl::star<
                pegtl::sor<
                    pegtl::string<'\\', '"'>,
                    pegtl::minus<pegtl::any, pegtl::one<'"'>>
                    >>
{

};

template<>
struct action<cstring_body> : pegtl::normal<cstring_body>
{
    template<typename Input>
    static void apply(const Input & input, metal::debug::var & v)
    {
        v.cstring.value = input.string();
    }
};

struct cstring_t :
        pegtl::seq<
            pegtl::one<'"'>,
            cstring_body,
            pegtl::one<'"'>,
            pegtl::opt<ellipsis>
            >
{};


struct char_num : pegtl::seq<pegtl::opt<pegtl::one<'-'>>, pegtl::plus<pegtl::digit>>
{
};

template<>
struct action<char_num> : pegtl::normal<char_num>
{
    template<typename Input>
    static void apply(const Input & input, std::string & v)
    {
        v = input.string();
    }
};

struct char_t :
        pegtl::seq<
            char_num,
            pegtl::star<pegtl::one<' '>>,
            pegtl::one<'\''>,
            pegtl::rep_max<5,
                pegtl::sor<
                    pegtl::string<'\\', '\''>,
                    pegtl::minus<pegtl::any, pegtl::one<'\''>>
                    >
                >,
            pegtl::one<'\''>
            >

{

};



struct global_offset :
        pegtl::seq<
            pegtl::one<'<'>,
            pegtl::plus<pegtl::minus<pegtl::any, pegtl::one<'>'>>>,
            pegtl::one<'>'>>
{};

struct hex_num : pegtl::seq<pegtl::string<'0', 'x'>, pegtl::plus<pegtl::xdigit>>
{
};

template<>
struct action<hex_num> : pegtl::normal<hex_num>
{
    template<typename Input>
    static void apply(const Input & input, metal::debug::var & v)
    {
        v.value = input.string();
    }
};


struct value_ref :
        pegtl::seq<
            hex_num,
            pegtl::opt<pegtl::seq<pegtl::one<' '>, global_offset>>,
            pegtl::opt<pegtl::seq<pegtl::one<' '>, cstring_t>>
        >
{

};

struct reference : pegtl::seq<pegtl::string<'@', '0', 'x'>, pegtl::plus<pegtl::xdigit>>
{
};

template<>
struct action<reference> : pegtl::normal<hex_num>
{
    template<typename Input>
    static void apply(const Input & input, std::uint64_t & v)
    {
        auto st = input.string();
        try {
            v = std::stoull(st.c_str() + 1, nullptr, 16);
        }
        catch (std::invalid_argument & ia)
        {
            BOOST_THROW_EXCEPTION(parser_error("stoull[reference] - invalid argument '" + st + "'"));
        }
        catch (std::out_of_range & oor)
        {
            BOOST_THROW_EXCEPTION(parser_error("stoull[reference] - out of range '" + st + "'"));
        }
    }
};

}

std::unordered_map<std::string, std::uint64_t> frame_impl::regs()
{
    std::unordered_map<std::string, std::uint64_t> mp;
    auto reg_names = _interpreter.data_list_register_names();
    auto regs = _interpreter.data_list_register_values(format_spec::hexadecimal);

    mp.reserve(regs.size());

    for (auto & r : regs)
    {
        if (r.number > reg_names.size())
        {
            std::uint64_t val = 0ull;
            try {val = std::stoull(r.value, nullptr, 16);} catch(...){}
            mp.emplace(reg_names[r.number], val);
        }
    }

    return mp;
}

void frame_impl::set(const std::string &var, const std::string & val)
{
    _interpreter.data_evaluate_expression('"' + var + " = " + val + '"');
    proc.reset_timer();
}

void frame_impl::set(const std::string &var, std::size_t idx, const std::string & val)
{
    _interpreter.data_evaluate_expression('"' + var + "[" + std::to_string(idx) + "] = " + val + '"');
    proc.reset_timer();
}

boost::optional<metal::debug::var> frame_impl::call(const std::string & cl)
{
    auto val = _interpreter.data_evaluate_expression(cl);
    if (val == "void")
        return boost::none;

    proc.reset_timer();

    metal::debug::var vr;

    std::uint64_t ref_value;
    pegtl::memory_input<> mi{val, "gdb mi2, value parse"};
    if (pegtl::parse<parser::reference, parser::action>(mi, ref_value))
    {
        vr.ref = ref_value;
        return vr;
    }

    if (pegtl::parse<parser::value_ref, parser::action>(mi, vr))
        return vr;

    vr.value = val;

    return vr;
}

std::size_t frame_impl::get_size(const std::string pt)
{
    //ok, we need to check if it's a variable first
    std::string size_st = _interpreter.data_evaluate_expression("sizeof(" + pt + ")");
    proc.reset_timer();

    try {
        return std::stoull(size_st);

    }
    catch (std::invalid_argument & ia)
    {
        BOOST_THROW_EXCEPTION(parser_error("stoull[sizeof(" + pt + ")] - invalid argument '" + size_st + "'"));
    }
    catch (std::out_of_range & oor)
    {
        BOOST_THROW_EXCEPTION(parser_error("stoull[sizeof(" + pt + ")] - out of range '" + size_st + "'"));
    }

    return 0u;
}


metal::debug::var frame_impl::print(const std::string & pt, bool bitwise)
{
    metal::debug::var ref_val;

    auto is_var =
        [](const std::string & pt)
        {
            regex rx{"[^A-Za-z_]\\w*"};
            return regex_match(pt, rx);
        };

    if (bitwise && !is_var(pt))
    {
        try {
            auto size = get_size(pt);
            auto addr = _interpreter.data_evaluate_expression("&" + pt);
            //addr might be inside ""
            auto idx = addr.find(' ');
            if (idx != std::string::npos)
                addr = addr.substr(0, idx);

            auto vv  = _interpreter.data_read_memory_bytes(addr, size);
            auto & val = vv.at(0);

            ref_val.ref = val.begin + val.offset;
            ref_val.value.reserve(val.contents.size() * 8);

            auto bit_to_str =
                    [](char c, int pos)
                    {
                        return ((c >> pos) & 0b1) ? '1' : '0';
                    };

            for (auto & v : boost::make_iterator_range(val.contents.rbegin(), val.contents.rend()))
            {
                ref_val.value.push_back(bit_to_str(v, 7));
                ref_val.value.push_back(bit_to_str(v, 6));
                ref_val.value.push_back(bit_to_str(v, 5));
                ref_val.value.push_back(bit_to_str(v, 4));
                ref_val.value.push_back(bit_to_str(v, 3));
                ref_val.value.push_back(bit_to_str(v, 2));
                ref_val.value.push_back(bit_to_str(v, 1));
                ref_val.value.push_back(bit_to_str(v, 0));
            }
            proc.reset_timer();

            return ref_val;
        }
        catch (unexpected_result_class & urc) //not possible this way.
        {
        }
    }

    auto val = _interpreter.data_evaluate_expression(pt);

    std::uint64_t ref_value;
    pegtl::memory_input<> mi{val, "gdb mi2, value parse"};

    if (pegtl::parse<parser::reference, parser::action>(mi, ref_value))
    {
        ref_val.ref = ref_value;
        val = _interpreter.data_evaluate_expression("*" + std::string(val.c_str()+1)); //to remove the @

    }

    if (pegtl::parse<parser::value_ref, parser::action>(mi, ref_val))
    {

    }
    else
        ref_val.value = val;

    std::string char_v;
    pegtl::memory_input<> mi_ref{ref_val.value, "gdb mi2, char parse"};

    if (pegtl::parse<parser::char_t, parser::action>(mi_ref,  char_v))
        ref_val.value = char_v; 

    if (bitwise //turn the int into a binary.
        && !((ref_val.value.front() == '<') && (ref_val.value.back() == '>'))) //if it's not meta information e.g. <optimized out>
    {
        auto size = get_size(pt);
        std::size_t val = 0u;
        
        
        try {
            val = std::stoull(ref_val.value);
        }
        catch (std::invalid_argument & ia)
        {
            BOOST_THROW_EXCEPTION(parser_error("stoull[print, bitwise] - invalid argument '" + ref_val.value + "'"));
        }
        catch (std::out_of_range & oor)
        {
            BOOST_THROW_EXCEPTION(parser_error("stoull[print, bitwise] - out of range '" + ref_val.value + "'"));
        }

        std::string res;
        res.reserve(size*8);

        auto bit_to_str =
                [](std::uint64_t c, std::uint64_t pos)
                {
                    return ((c >> pos) & 0b1ull) ? '1' : '0';
                };

        bool first = true;
        auto push_back = [&](char c)
        {
            if ((c == '0') && first)
                return;
            else
            {
                first = false;
                res.push_back(c);
            }
        };

        for (auto i = size; i != 0; )
        {
            i--;
            push_back(bit_to_str(val, 7 + (i * 8)));
            push_back(bit_to_str(val, 6 + (i * 8)));
            push_back(bit_to_str(val, 5 + (i * 8)));
            push_back(bit_to_str(val, 4 + (i * 8)));
            push_back(bit_to_str(val, 3 + (i * 8)));
            push_back(bit_to_str(val, 2 + (i * 8)));
            push_back(bit_to_str(val, 1 + (i * 8)));
            push_back(bit_to_str(val, 0 + (i * 8)));
        }
        if (res.empty())
            res = "0";
        ref_val.value = res;

    }

    return ref_val;
}

metal::debug::var parse_var(interpreter & interpreter_,  const std::string & id, std::string val)
{
    metal::debug::var ref_val;
    std::uint64_t ref_value;
    pegtl::memory_input<> mi{val, "gdb mi2, value parse"};

    if (pegtl::parse<parser::reference, parser::action>(mi, ref_value))
    {
        ref_val.ref = ref_value;
        val = interpreter_.data_evaluate_expression("&*" + id); //to remove the @

    }

    if (pegtl::parse<parser::value_ref, parser::action>(mi, ref_val))
        return ref_val;

    ref_val.value = val;
    return ref_val;
}

void frame_impl::return_(const std::string & value)
{
    _interpreter.exec_return(value);
    proc.reset_timer();
}

void frame_impl::set_exit(int code)
{
    proc.set_exit(code);
}

void frame_impl::select(int frame)
{
    _interpreter.stack_select_frame(frame);
    proc.reset_timer();
}

std::vector<metal::debug::backtrace_elem> frame_impl::backtrace()
{
    auto bt = _interpreter.stack_list_frames();
    proc.reset_timer();

    std::vector<metal::debug::backtrace_elem> ret;
    ret.reserve(bt.size());

    for (auto & b : bt)
    {

        metal::debug::backtrace_elem e;
        e.cnt =  b.level;
        e.addr = b.addr;
        if (b.func)
            e.func = *b.func;
        if (b.file)
            e.loc.file = *b.file;
        if (b.line)
            e.loc.line = *b.line;

        ret.push_back(std::move(e));
    }
    return ret;
}

boost::optional<metal::debug::address_info> frame_impl::addr2line(std::uint64_t addr) const
{
    try
    {
        src_and_asm_line dd;

        try {
            dd = _interpreter.data_disassemble(
                mi2::disassemble_mode::mixed_source_and_disassembly_with_raw_opcodes,
                addr, addr+1);
        }
        catch (mi2::interpreter_error & )
        {
            //might be necessary for some gdb
            dd = _interpreter.data_disassemble(
                mi2::disassemble_mode::mixed_source_and_disassembly_with_raw_opcodes_deprecated,
                addr, addr+1);

        }



        proc.reset_timer();

        if (dd.file.empty() && (!dd.fullname || dd.fullname->empty()) && !dd.line_asm_insn)
            return boost::none;

        metal::debug::address_info ai;

        ai.file = dd.file;
        ai.full_name = dd.fullname;
        ai.line = dd.line;

        if (dd.line_asm_insn)
        {
            auto itr = std::find_if(
                    dd.line_asm_insn->begin(),
                    dd.line_asm_insn->end(),
                    [&](const line_asm_insn & lai)
                    {
                        return lai.address == addr;
                    });
            if (itr != dd.line_asm_insn->end())
            {
                ai.function = itr->func_name;
                ai.offset = itr->offset;
            }
        }
        else
            _log << "no line_asm_isns" << std::endl;
        return ai;
    }
    catch (interpreter_error & ie)
    {
        _log << "Exception [" << typeid(ie).name() << "]: " << ie.what() << std::endl;
        return boost::none;
    }
}

void frame_impl::disable(const metal::debug::break_point & bp)
{
    const auto & bps = proc.break_point_map();

    auto itr = std::find_if(bps.begin(), bps.end(), [&](const std::pair<int, break_point*> & bp_p){return bp_p.second == &bp;});
    if (itr == bps.end())
        return ; //should not happen

    int num = itr->first;
    _interpreter.break_disable(num);
    proc.reset_timer();
}

void frame_impl::enable (const metal::debug::break_point & bp)
{
    const auto & bps = proc.break_point_map();

    auto itr = std::find_if(bps.begin(), bps.end(), [&](const std::pair<int, break_point*> & bp_p){return bp_p.second == &bp;});
    if (itr == bps.end())
        return ; //should not happen

    int num = itr->first;
    _interpreter.break_enable(num);
    proc.reset_timer();
}

std::vector<std::uint8_t> frame_impl::read_memory(std::uint64_t addr, std::size_t size)
{
    auto data = _interpreter.data_read_memory_bytes(std::to_string(addr), size);

    std::vector<std::uint8_t> vec;

    for (auto & d : data)
    {
        auto size = d.offset + d.contents.size();
        vec.resize(size);

        std::copy(d.contents.begin(), d.contents.end(), vec.begin() + d.offset);
    }

    return vec;
}

void frame_impl::write_memory(std::uint64_t addr, const std::vector<std::uint8_t> &vec)
{
    _interpreter.data_write_memory_bytes(std::to_string(addr), vec);
}



}}}


