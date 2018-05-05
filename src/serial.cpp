/**
 * @file   serial.cpp
 * @date   10.04.2018
 * @author Klemens D. Morgenstern
 *
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>
#include <boost/process/pipe.hpp>
#include <boost/process/io.hpp>
#include <boost/dll.hpp>
#include <boost/process/search_path.hpp>
#include <memory>
#include <iterator>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/optional.hpp>
#include <boost/process/child.hpp>

#include <iterator>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/binary/binary.hpp>
#include <boost/spirit/home/support/multi_pass.hpp>

#include <metal/serial.hpp>
#include <metal/serial/session.hpp>

#include <boost/algorithm/string/trim_all.hpp>
#include <boost/functional/hash.hpp>

namespace std
{
template<> struct hash<boost::filesystem::path>
{
    size_t operator()(const boost::filesystem::path& p) const
    {
        return boost::filesystem::hash_value(p);
    }
};
}

using iterator_t = boost::spirit::multi_pass<std::istreambuf_iterator<char>>;
namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace bp = boost::process;
namespace x3 = boost::spirit::x3;
using namespace metal::serial;
using namespace std;
struct parser_exception : std::runtime_error {using std::runtime_error::runtime_error;};


struct options_t;

bool init_session(iterator_t & itr, const iterator_t & end, char & nullchar,
                  int & intLength, int & ptrLength, endianess_t &endianess, std::uint64_t &init_loc);

int run_serial(const options_t &opt, iterator_t & itr, const iterator_t & end, char nullchar, int intLength, int ptrLength,
               const std::unordered_map<std::string, metal::serial::plugin_function_t> & macros, std::uint64_t init_loc,
               endianess_t endianess);

struct options_t
{
    fs::path my_binary;
    fs::path my_path;


    bool help{false};
    fs::path addr2line;
    fs::path source_dir;

    std::string comp;
    std::string binary;

    vector<fs::path> dlls;

    vector<boost::dll::shared_library> plugins;

    string source_folder;

    po::options_description desc;
    po::variables_map vm;

    po::positional_options_description pos;

    static pair<string, string> at_option_parser(string const&s)
    {
        if ('@' == s[0])
            return std::make_pair(string("response-file"), s.substr(1));
        else
            return pair<string, string>();
    }

    void load_cfg(bool allow_unregistered = false)
    {
        if (vm.count("response-file"))
        {
            // Load the file and tokenize it
            ifstream ifs(vm["response-file"].as<string>().c_str());
            if (!ifs)
            {
                cout << "Could not open the response file\n";
                exit(1);
            }
            // Read the whole file into a string
            stringstream ss;
            ss << ifs.rdbuf();
            // Split the file content
            boost::char_separator<char> sep(" \n\r");
            std::string ResponsefileContents( ss.str() );
            boost::tokenizer<boost::char_separator<char> > tok(ResponsefileContents, sep);
            vector<string> args(tok.begin(), tok.end());
            // Parse the file and store the options
            if (allow_unregistered)
                po::store(po::command_line_parser(args).options(desc).allow_unregistered().run(), vm);
            else
                po::store(po::command_line_parser(args).options(desc).run(), vm);
        }
        if (vm.count("config-file"))
        {
            ifstream ifs(vm["config-file"].as<string>());
            po::store(po::parse_config_file(ifs, desc, allow_unregistered), vm);
        }
    }
    void parse(int argc, char** argv)
    {
        my_path = my_binary.parent_path();
        using namespace boost::program_options;

        desc.add_options()
                ("lib,P",         value<vector<fs::path>>(&dlls)->multitoken(), "break-point libraries")
                ("response-file", value<string>(), "can be specified with '@name', too")
                ("config-file,C", value<string>(), "config file")
                ;

        po::store(po::command_line_parser(argc, argv).
                options(desc).extra_parser(at_option_parser).allow_unregistered().run(), vm);

        load_cfg(true);

        po::notify(vm);
        for (auto & dll : dlls)
        {
            if (fs::exists(dll))
                plugins.emplace_back(dll);
            else if (dll.parent_path().empty())
            {
                //check for the local version needed
#if defined(BOOST_WINDOWS_API)
                fs::path p = my_path / ("lib" + dll.string() + ".dll");
#else
                fs::path p = my_path / ("lib" + dll.string() + ".so");
#endif
                if (fs::exists(p))
                    plugins.emplace_back(p);
                else
                    continue;
            }
            else
                continue;

            auto & p = plugins.back();
            if (p.has("metal_serial_setup_options"))
            {
                auto f = boost::dll::import<void(po::options_description &)>(p, "metal_serial_setup_options");
                po::options_description po;
                f(po);
                desc.add(std::move(po));
            }

        }

        //ok, now load the full thingy
        vm.clear();

        desc.add_options()
                ("help,H", "produce help message")
                ("binary,B", po::value<string>(&binary), "binary")
                ("compiler,C", po::value<string>(&comp), "compiler [gcc, clang]")
                ("response-file", po::value<string>(), "can be specified with '@name', too")
                ("config-file,E", po::value<string>(), "config file")
                ("addr2line,A",  po::value<fs::path>(&addr2line)->default_value("addr2line"), "Custom addr2line command")
                ("source-dir,S", po::value<fs::path>(&source_dir), "root of the source directory");


        pos.add("binary", 1);
        pos.add("source-dir", 2);

        po::store(po::command_line_parser(argc, argv).
                options(desc).positional(pos).extra_parser(at_option_parser).run(), vm);

        load_cfg();

        po::notify(vm);
    }
};

int main(int argc, char **argv)
{
    options_t opt;
    try {
        opt.parse(argc, argv);

        if (opt.help) {
            cout << opt.desc << endl;

            return 0;
        }

        if (opt.binary.empty()) {
            cerr << "No executable defined\n" << endl;
            return 2;
        }

        boost::optional<fs::ifstream> fstream;
        auto itr = fstream ? iterator_t(*fstream) : iterator_t(std::cin);
        const iterator_t end;

        char nullchar{0};
        int intLength;
        int ptrLength;
        endianess_t endianess;

        std::uint64_t init_loc;
        if (!init_session(itr, end, nullchar, intLength, ptrLength, endianess, init_loc))
            return 2;

        std::unordered_map<std::string, metal::serial::plugin_function_t> macros;

        for (auto & lib : opt.plugins)
        {
            auto f = boost::dll::import<void(std::unordered_map<std::string, metal::serial::plugin_function_t> &)>(lib, "metal_serial_setup_entries");
            f(macros);
        }

        return run_serial(opt, itr, end, nullchar, intLength, ptrLength, macros, init_loc, endianess);
    }
    catch (parser_exception & pe)
    {
        std::cerr << "Parser error " << pe.what() << std::endl;
        return 2;
    }
    catch (std::exception & e)
    {
        cerr << "**metal-serial** Exception ["
             << boost::core::demangle(typeid(e).name())
             << "] thrown: '" << e.what() << "'" << endl;
        return 2;
    }
    catch (...)
    {
        cerr << "Unknown error" << endl;
        return 2;
    }

}

bool init_session(iterator_t & itr, const iterator_t & end, char & nullchar,
                  int & intLength, int & ptrLength, endianess_t &endianess,  std::uint64_t &init_loc)
{
    int idx = 0;
    std::vector<char> intToken;
    std::vector<char> ptrToken;
    //parse the header
#define check_parser(itr, end, rule, value) if (!x3::parse(itr, end, rule, value)) { std::cerr << "Header parsing failed" << #rule << std::endl; return 1;}

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

std::unique_ptr<session> build_session(iterator_t & itr, const iterator_t & end, char nullchar, int int_length,
                                       int ptr_length, endianess_t endianess);

void set_session_loc(std::unique_ptr<session> & session, const std::string& file_name, int line_number);
boost::optional<int> get_exited(std::unique_ptr<session> & session);

int run_serial(const options_t &opt, iterator_t & itr, const iterator_t & end, char nullchar, int intLength, int ptrLength,
               const std::unordered_map<std::string, metal::serial::plugin_function_t> & macros, std::uint64_t init_loc,
               endianess_t endianess)
{
    bp::ipstream pin;
    bp::opstream pout;
    bp::child ch(bp::search_path(opt.addr2line), "--exe=" + opt.binary, bp::std_in < pout, bp::std_out > pin,
                 bp::std_err > stderr);

    if (!ch.running()) {
        std::cerr << "addr2line not started (" << bp::search_path(opt.addr2line) << ")" << std::endl;
        return 2;
    }
    else
        ch.detach();
    pout << std::hex;

    unordered_map<fs::path, loaded_file> file_buffer;

    auto load_file = [&](fs::path &pth)
                     {
                         if (fs::exists(opt.source_dir / pth))
                             pth = opt.source_dir / pth;
                         if (!fs::exists(pth))
                         {
                            std::cerr << "Source file " << pth << " not found" << std::endl;
                            return false;
                         }
                         pth = fs::absolute(pth);

                         string line;
                         vector<string> st; st.push_back("/** INVALID LINE **/"); //to start at one
                         fs::ifstream istr(pth);
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
    auto get_loc = [&, addr2lineRegex = std::regex{"^((?:\\w:)?[^:]+):(\\d+)\\s*"}](std::uint64_t location)
                    {
                        std::string line;
                        std::smatch match;

                        pout << "0x" << location << std::endl;
                        if (std::getline(pin, line) && std::regex_match(line, match, addr2lineRegex))
                        {
                            file_name = match[1].str();
                            line_number = std::stoi(match[2].str());
                            std::cerr << "Matched " << file_name << ":" << line_number << std::endl;
                            return true;
                        }
                        else
                            std::cerr << "Error reading from addr2line" << std::endl;
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
        std::cout << "Initializing metal serial from " << file_name << ":" << line_number << std::endl;
    }
    else
        return 2;

    std::unique_ptr<session> session_p = build_session(itr, end, nullchar, intLength, ptrLength, endianess);
    while (itr != end && !get_exited(session_p))
    {
        std::cerr << "Itr: " << (int)*itr << std::endl;
        set_session_loc(session_p, "**unknown location**", 00);
        auto loc = session_p->get_ptr();
        std::cerr << "Loc: 0x" << std::hex << loc << std::dec << std::endl;

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
        return *exit_code;

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
    }

    std::vector<char> get_raw() override
    {
        auto size = *itr;
        std::string res;
        check_parser(x3::parse(itr, end, x3::omit[x3::byte_(size)] >> x3::repeat(size)[x3::byte_]
                                                                   >> x3::omit[x3::byte_(nullChar)], res),
                     "Failed to parse raw data.");

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
    auto it2 = itr;

    std::cerr << std::hex << "Foo :" ;
    for (int i = 0; i<10; i++)
        std::cerr << " 0x" << *(it2++);
    std::cerr << std::hex << std::endl;

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

boost::optional<int> get_exited(std::unique_ptr<session> & session) {return static_cast<session_impl*>(session.get())->exited;}

std::unique_ptr<session> build_session(iterator_t & itr, const iterator_t & end, char nullchar, int int_length,
                                       int ptr_length, endianess_t endianess)
{
    if (endianess == endianess_t::little_endian)
        return make_unique<session_impl_endian<endianess_t::little_endian>>(itr, end, nullchar, ptr_length);
    else
        return make_unique<session_impl_endian<endianess_t::big_endian>>(itr, end, nullchar, ptr_length);

}