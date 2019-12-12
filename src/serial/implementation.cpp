/**
 * @file   implementation.cpp
 * @date   03.05.2018
 * @author Klemens D. Morgenstern
 *
 */

#include "implementation.hpp"
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/binary/binary.hpp>
#include <boost/process/pipe.hpp>
#include <boost/process/io.hpp>
#include <boost/process/child.hpp>

#include <boost/process/search_path.hpp>
#include <iostream>
#include <boost/algorithm/string/trim_all.hpp>
#include <boost/filesystem/fstream.hpp>

#include <fstream>
#include <regex>

#define _METAL_SERIAL_VERSION_STRING "__metal_serial_version_0"

namespace bp = boost::process;

namespace fs = std::filesystem;
namespace x3 = boost::spirit::x3;
using namespace metal::serial;
using namespace std;

bool init_session(iterator_t & itr, const iterator_t & end, char & nullchar,
                  int & intLength, int & ptrLength, endianess_t &endianess,  std::uint64_t &init_loc)
{
    int idx = 0;
    std::vector<char> intToken;
    std::vector<char> ptrToken;
    //parse the header
#define check_parser(itr, end, rule, value) if (!x3::parse(itr, end, rule, value)) { std::cerr << "Header parsing failed " << #rule << std::endl; return 1;}

    check_parser(itr, end, x3::lexeme[_METAL_SERIAL_VERSION_STRING] >> x3::byte_, intLength); //not considering the nullchar currentls
    check_parser(itr, end, x3::repeat(intLength)[x3::byte_] >> x3::byte_(nullchar), intToken);
    check_parser(itr, end, x3::byte_, ptrLength);
    check_parser(itr, end, x3::repeat(ptrLength)[x3::byte_] >> x3::byte_(nullchar), ptrToken);
#undef check_parser

    //0b11001100
    //  11000011
    switch (intLength)
    {
        case 1:
            if (intToken[0] == 0b01000011)
                endianess = endianess_t::little_endian;
            else if (intToken[0] == 0b01101100)
                endianess = endianess_t::big_endian;
            break;
        case 2:
            if ((intToken[0] == 0b01000011) && (intToken[1] == 0b01101100))
                endianess = endianess_t::little_endian;
            else if ((intToken[0] == 0b01101100) && (intToken[1] == 0b01000011))
                endianess = endianess_t::big_endian;
            else
            {
                std::cerr << "Error determining endian type" << std::endl;
                return false;
            }
            break;
        case 4:
            if ((intToken[0] == 0b01000011) && (intToken[1] == 0b01101100) && (intToken[2] == 0) && intToken[3] == 0)
                endianess = endianess_t::little_endian;
            else if ((intToken[0] == 0) && (intToken[1] == 0) && (intToken[2] == 0b01101100) && intToken[3] == 0b01001100)
                endianess = endianess_t::big_endian;
            else
            {
                std::cerr << "Error determining endian type from " << std::endl;
                return false;
            }
            break;
    }

    if (endianess == endianess_t::little_endian)
        for (auto i = 0u; i<ptrLength; i++)
            reinterpret_cast<char*>(&init_loc)[i] = ptrToken[i];
    else
        for (auto i = 0u; i<ptrLength; i++)
            reinterpret_cast<char*>(&init_loc)[(ptrLength - i) - 1] = ptrToken[i];

    return true;
}


x3::rule<class parenthesized_code> parenthesized_code;

const auto parenthesized_code_def = '(' >> *((x3::char_ - x3::char_("()")) | parenthesized_code) >> ')';

BOOST_SPIRIT_DEFINE(parenthesized_code);


bool parse_macro(string::const_iterator itr, string::const_iterator end,
                 std::string & macro_name, std::vector<std::string> &args)
{
    args.clear();
    auto skipper = ("//" >> *(!x3::eol >> x3::char_) >> x3::eol) |
                   ("/*" >> (*(!x3::string("*/") >> x3::char_)) >> "*/");

    int parenth_depth = 0;

    auto assign_macro_name =
            [&](auto & ctx)
            {
                macro_name = x3::_attr(ctx);
            };

    auto push_back =
            [&](auto & ctx)
            {
                auto & attr = x3::_attr(ctx);
                std::string res(attr.begin(), attr.end());
                std::replace_if(res.begin(), res.end(),
                                [](char c){return (c == '\r') && (c == '\n');}, ' ');
                boost::trim_all(res);
                if (!res.empty())
                    args.push_back(std::move(res));
            };

    auto arg_rule = x3::raw[*(parenthesized_code | (x3::char_ - x3::char_(",)")))];
    auto rule = *(x3::space | x3::eol) >> (+x3::char_("_a-zA-Z0-9"))[assign_macro_name]
                                       >> *(x3::space | x3::eol)
                                       >> '(' >> -(arg_rule[push_back] % ','  ) >>')';


    auto res = x3::phrase_parse(itr, end, rule, skipper);
    return res;

}

struct loaded_file
{
    string content;
    vector<std::size_t> linestarts;

    auto line(std::size_t line) {return content.cbegin() + linestarts.at(line);}
    auto end() {return content.cend();}

    loaded_file(string && str) : content(std::move(str))
    {
        linestarts.push_back(0);
        linestarts.push_back(0);
        for (std::size_t idx = 0u; idx < content.size(); idx++)
            if (content[idx] == '\n')
                linestarts.push_back(idx+1);
    }
    loaded_file() = default;
    loaded_file(loaded_file&&) = default;
    loaded_file& operator=(loaded_file&&) = default;
};


int run_serial(const std::string binary, const std::filesystem::path &source_dir, const std::filesystem::path &addr2line,
               iterator_t &itr, const iterator_t &end, char nullchar, int intLength, int ptrLength,
               const std::unordered_map<std::string, metal::serial::plugin_function_t> &macros, std::uint64_t init_loc,
               metal::serial::endianess_t endianess, bool ignore_exit_code)
{
    bp::ipstream pin;
    bp::opstream pout;
    bp::child ch(bp::search_path(addr2line.string()), "--exe=" + binary, bp::std_in < pout, bp::std_out > pin,
                 bp::std_err > stderr);

    ch.wait_for(std::chrono::milliseconds(100));

    if (!ch.running()) {
        std::cerr << "addr2line not started (" << bp::search_path(addr2line.string()) << ")" << std::endl;
        return 2;
    }
    else
        ch.detach();
    pout << std::hex;

    unordered_map<fs::path, loaded_file> file_buffer;

    auto load_file = [&](fs::path &pth)
    {
        if (fs::exists(source_dir / pth))
            pth = source_dir / pth;
        if (!fs::exists(pth))
        {
            std::cerr << "Source file " << pth << " not found" << std::endl;
            return false;
        }
        pth = fs::absolute(pth);

        string line;
        vector<string> st; st.push_back("/** INVALID LINE **/"); //to start at one
        std::ifstream istr(pth);
        ostringstream str;
        str << istr.rdbuf();

        file_buffer.emplace(pth, str.str());
        return true;
    };

    auto get_line = [&](fs::path pth, int line) -> boost::iterator_range<typename string::const_iterator>
    {
        if (file_buffer.count(pth) == 0)
            if (!load_file(pth))
                return {};

        auto & fn = file_buffer[pth];

        return boost::make_iterator_range(fn.line(line), fn.end());
    };

    std::string file_name;
    int line_number;
    auto get_loc = [&, addr2lineRegex = std::regex{"^((?:\\w:)?[^:]+):(\\d+)(?:\\s+\\(discriminator \\d+\\))?\\s*"}](std::uint64_t location)
    {
        std::string line;
        std::smatch match;

        pout << "0x" << location << std::endl;
        if (std::getline(pin, line) && std::regex_match(line, match, addr2lineRegex))
        {
            file_name = match[1].str();
            line_number = std::stoi(match[2].str());
                return true;
        }
        else
            std::cerr << "Error reading from addr2line '" << line << "'" << std::endl;
        return false;
    };

    std::string macro_name;
    std::vector<std::string> macro_args;

    auto parse_loc = [&](std::uint64_t location)
    {
        if (get_loc(location))
        {
            auto range = get_line(file_name, line_number);
            return parse_macro(range.begin(), range.end(), macro_name,  macro_args);
        }
        else
            return false;
    };

    //alright, verify the code location
    if (parse_loc(init_loc))
    {
        if ((macro_name != "METAL_SERIAL_INIT") || !macro_args.empty())
        {
            std::cerr << "Could not verify start sequence '" << macro_name << "', " << macro_args.size()  << std::endl;
            return 2;
        }
        std::cerr << "Initializing metal serial from " << file_name << ":" << line_number << std::endl;
    }
    else
        return 2;

    std::unique_ptr<session> session_p = build_session(itr, end, nullchar, intLength, ptrLength, endianess);
    while ((itr != end) && !get_exited(session_p))
    {
        set_session_loc(session_p, "**unknown location**", 00);
        auto loc = session_p->get_ptr();
        if (!parse_loc(loc))
            return 2;
        set_session_loc(session_p, file_name, line_number);
        if (macros.count(macro_name) == 0)
        {
            std::cerr << file_name << "(" << line_number << ") macro not found: '" << macro_name << "'" << std::endl;
            return 2;
        }
        auto & func = macros.at(macro_name);
        func(*session_p, macro_args, file_name, line_number);
    }


    pout.pipe().close();

    if (auto exit_code = get_exited(session_p))
        return ignore_exit_code ? 0 : *exit_code;

    std::cerr << "Ending input " << std::endl;
    return 2;
}

struct session_impl : metal::serial::session
{
    iterator_t & itr;
    const iterator_t & end;
    const char nullChar;
    const size_t ptr_size;

    std::string source_file{"**unknown file**"};
    int line{0};

    boost::optional<int> exited;

    void set_exit(int value) override
    {
        exited = value;
    }

    void check_parser(bool parser, const std::string & msg)
    {
        if (!parser)
            throw parser_exception(msg + " from " + source_file + ":" + to_string(line));
    }

    session_impl(iterator_t & itr, const iterator_t  & end, const char nullChar, size_t ptr_size)
            : itr(itr), end(end), nullChar(nullChar), ptr_size(ptr_size) {}

    std::string get_str() override final
    {
        auto size = *itr;
        std::string res;
        check_parser(x3::parse(itr, end, x3::omit[x3::byte_(size)] >> x3::repeat(size)[x3::char_]
                                                                   >> x3::omit[x3::byte_(nullChar)], res),
                     "Failed to parse string.");
        return res;
    }

    std::vector<char> get_raw() override
    {
        auto size = *itr;
        std::vector<char> res;
        check_parser(x3::parse(itr, end, x3::omit[x3::byte_(size)] >> x3::repeat(size)[x3::byte_]
                                                                   >> x3::omit[x3::byte_(nullChar)], res),
                     "Failed to parse raw data.");
        return res;
    }


    std::uint8_t get_uint8() override final
    {
        std::uint8_t  res{0};
        check_parser(x3::parse(itr, end, x3::omit[x3::byte_(1)] >> x3::byte_ >> x3::omit[x3::byte_(nullChar)], res), "Can't parse uint8_t");
        return res;
    }
    std::int8_t  get_int8() override final
    {
        std::int8_t  res{0};
        check_parser(x3::parse(itr, end, x3::omit[x3::byte_(1)] >> x3::byte_ >> x3::omit[x3::byte_(nullChar)], res), "Can't parse uint8_t");
        return res;
    }

    char get_char() override final
    {
        char res{0};
        check_parser(x3::parse(itr, end, x3::omit[x3::byte_(1)] >> x3::char_ >> x3::omit[x3::byte_(nullChar)], res), "Can't parse uint8_t");
        return res;
    }

    bool get_bool() override final
    {
        bool res{0};
        check_parser(x3::parse(itr, end, x3::omit[x3::byte_(1)] >> x3::byte_ >> x3::omit[x3::byte_(nullChar)], res), "Can't parse bool");
        return res;
    }

    std:: int64_t get_int()  override final
    {
        switch (*itr)
        {
            case 1: return get_int8();
            case 2: return get_int16();
            case 4: return get_int32();
            case 8: return get_int64();
            default:
                check_parser(false ,"Unable to read int of size " + std::to_string(*itr));
                return 0ll;
        }
    }

    std::uint64_t get_uint() override final
    {
        switch (*itr)
        {
            case 1: return get_uint8();
            case 2: return get_uint16();
            case 4: return get_uint32();
            case 8: return get_uint64();
            default:
                check_parser(false ,"Unable to read unsigned int of size " + std::to_string(*itr));
                return 0ull;
        }
    }

    std::uint64_t get_ptr() override final
    {
        switch (ptr_size)
        {
            case 1: return get_uint8();
            case 2: return get_uint16();
            case 4: return get_uint32();
            case 8: return get_uint64();
            default:
                check_parser(false ,"Unable to read pointer of size " + std::to_string(*itr));
                return 0ull;
        }
    }
};


boost::optional<int> get_exited(std::unique_ptr<metal::serial::session> & session)
{
    return static_cast<session_impl*>(session.get())->exited;
}


template<endianess_t endianess_>
struct session_impl_endian final : session_impl
{
    using session_impl::session_impl;
    endianess_t endianess() {return endianess_;}

    std::uint16_t get_uint16() override;
    std::int16_t  get_int16()  override;
    std::uint32_t get_uint32() override;
    std::int32_t  get_int32()  override;
    std::uint64_t get_uint64() override;
    std::int64_t  get_int64()  override;
};

template<> std::uint16_t session_impl_endian<endianess_t::big_endian>::get_uint16()
{
    std::uint16_t res{0};
    check_parser(x3::parse(itr, end, x3::omit[x3::byte_(2)] >> x3::big_word >> x3::omit[x3::byte_(nullChar)], res), "Can't parse uint16_t");

    return res;
}

template<> std::int16_t  session_impl_endian<endianess_t::big_endian>::get_int16()
{
    std::int16_t  res{0};
    check_parser(x3::parse(itr, end, x3::omit[x3::byte_(2)] >> x3::big_word >> x3::omit[x3::byte_(nullChar)], res), "Can't parse int16_t");
    return res;
}

template<> std::uint32_t session_impl_endian<endianess_t::big_endian>::get_uint32()
{
    std::uint32_t res{0};
    check_parser(x3::parse(itr, end, x3::omit[x3::byte_(4)] >> x3::big_dword >> x3::omit[x3::byte_(nullChar)], res), "Can't parse uint32_t");
    return res;
}

template<> std::int32_t  session_impl_endian<endianess_t::big_endian>::get_int32()
{
    std::int32_t  res{0};
    check_parser(x3::parse(itr, end, x3::omit[x3::byte_(4)] >> x3::big_dword >> x3::omit[x3::byte_(nullChar)], res), "Can't parse int32_t");

    return res;
}

template<> std::uint64_t session_impl_endian<endianess_t::big_endian>::get_uint64()
{
    std::uint64_t res{0};
    check_parser(x3::parse(itr, end,  x3::omit[x3::byte_(8)] >> x3::big_qword >>  x3::omit[x3::byte_(nullChar)], res), "Can't parse uint64_t");
    return res;
}

template<> std::int64_t  session_impl_endian<endianess_t::big_endian>::get_int64()
{
    std::int64_t  res{0};
    check_parser(x3::parse(itr, end, x3::omit[x3::byte_(8)] >> x3::big_qword >> x3::omit[x3::byte_(nullChar)], res), "Can't parse int64_t");
    return res;
}


template<> std::uint16_t session_impl_endian<endianess_t::little_endian>::get_uint16()
{
    std::uint16_t res{0};
    check_parser(x3::parse(itr, end, x3::omit[x3::byte_(2)] >> x3::little_word >> x3::omit[x3::byte_(nullChar)], res), "Can't parse uint16_t");

    return res;
}

template<> std::int16_t  session_impl_endian<endianess_t::little_endian>::get_int16()
{
    std::int16_t  res{0};
    check_parser(x3::parse(itr, end, x3::omit[x3::byte_(2)] >> x3::little_word >> x3::omit[x3::byte_(nullChar)], res), "Can't parse int16_t");
    return res;
}

template<> std::uint32_t session_impl_endian<endianess_t::little_endian>::get_uint32()
{
    std::uint32_t res{0};
    check_parser(x3::parse(itr, end, x3::omit[x3::byte_(4)] >> x3::little_dword >> x3::omit[x3::byte_(nullChar)], res), "Can't parse uint32_t");
    return res;
}

template<> std::int32_t  session_impl_endian<endianess_t::little_endian>::get_int32()
{
    std::int32_t  res{0};
    check_parser(x3::parse(itr, end, x3::omit[x3::byte_(4)] >> x3::little_dword >> x3::omit[x3::byte_(nullChar)], res), "Can't parse int32_t");

    return res;
}

template<> std::uint64_t session_impl_endian<endianess_t::little_endian>::get_uint64()
{
    std::uint64_t res{0};
    check_parser(x3::parse(itr, end, x3::omit[x3::byte_(8)] >> x3::little_qword >> x3::omit[x3::byte_(nullChar)], res), "Can't parse uint64_t");
    return res;
}

template<> std::int64_t  session_impl_endian<endianess_t::little_endian>::get_int64()
{
    std::int64_t  res{0};
    check_parser(x3::parse(itr, end, x3::omit[x3::byte_(8)] >> x3::little_qword >> x3::omit[x3::byte_(nullChar)], res), "Can't parse int64_t");
    return res;
}

void set_session_loc(std::unique_ptr<session> & session, const std::string& file_name, int line_number)
{
    auto pi = static_cast<session_impl*>(session.get());
    pi->source_file = file_name;
    pi->line = line_number;
}


std::unique_ptr<session> build_session(iterator_t & itr, const iterator_t & end, char nullchar, int int_length,
                                       int ptr_length, endianess_t endianess)
{
    if (endianess == endianess_t::little_endian)
        return make_unique<session_impl_endian<endianess_t::little_endian>>(itr, end, nullchar, ptr_length);
    else
        return make_unique<session_impl_endian<endianess_t::big_endian>>(itr, end, nullchar, ptr_length);

}