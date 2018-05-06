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

#include <boost/dll.hpp>
#include <boost/tokenizer.hpp>
#include <memory>
#include <iterator>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/optional.hpp>

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/binary/binary.hpp>

#include <metal/serial/session.hpp>

#include <boost/algorithm/string/trim_all.hpp>

#include "serial/implementation.hpp"
#include "serial/core_functions.hpp"
#include "serial/test_functions.hpp"


namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace x3 = boost::spirit::x3;
using namespace metal::serial;
using namespace std;


struct options_t;



struct options_t
{
    fs::path my_binary;
    fs::path my_path;


    bool help{false};
    fs::path addr2line;
    fs::path source_dir;

    std::string comp;
    std::string binary;

    fs::path log_file;

    bool ignore_exit_code = false;

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
                cerr << "Could not open the response file\n";
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

        metal_serial_test_setup_options(desc);

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
                ("source-dir,S", po::value<fs::path>(&source_dir), "root of the source directory")
                ("log,L",   po::value<fs::path>(&log_file), "log file (instead of stderrr)")
                ("ignore-exit-code", po::bool_switch(&ignore_exit_code), "ignore the exit code");


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

        boost::optional<fs::ofstream> logstream;
        if (!opt.log_file.empty())
        {
            logstream.emplace(opt.log_file);
            std::cerr.rdbuf(logstream->rdbuf());
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
        metal_serial_test_setup_entries(macros);
        for (auto & lib : opt.plugins)
        {
            auto f = boost::dll::import<void(std::unordered_map<std::string, metal::serial::plugin_function_t> &)>(lib, "metal_serial_setup_entries");
            f(macros);
        }
        macros.emplace("METAL_SERIAL_PRINTF", printf_impl);
        macros.emplace("METAL_SERIAL_EXIT",   exit_impl);
        return run_serial(opt.binary, opt.source_dir, opt.addr2line, itr, end, nullchar, intLength,
                          ptrLength, macros, init_loc, endianess, opt.ignore_exit_code);
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
