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
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>
#include <boost/process/pipe.hpp>
#include <boost/process/io.hpp>
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
//#include <metal/serial/binary_parser.hpp>


int main(int argc, char **argv)
{
    using namespace std;

    namespace po = boost::program_options;
    namespace fs = boost::filesystem;
    namespace bp = boost::process;
    namespace x3 = boost::spirit::x3;

    fs::path addr2line;
    fs::path input;

    std::string comp;
    std::string binary;

    bool indirect_call = false;
    auto at_option_parser = [](string const &s) -> pair<string, string> {
        if ('@' == s[0])
            return std::make_pair(string("response-file"), s.substr(1));
        else
            return pair<string, string>();
    };

    po::options_description desc("Possible options");
    desc.add_options()
            ("help,H", "produce help message")
            ("binary,B", po::value<string>(&binary), "binary")
            ("compiler,C", po::value<string>(&comp), "compiler [gcc, clang]")
            ("response-file", po::value<string>(), "can be specified with '@name', too")
            ("config-file,E", po::value<string>(), "config file")
            ("addr2line,A", po::value<fs::path>(&addr2line)->default_value("addr2line"), "Custom addr2line command")
            ("input file,I", po::value<fs::path>(&input), "input file");

    po::positional_options_description pos;
    pos.add("binary", -1);

    po::variables_map vm;

    try {
        po::store(po::command_line_parser(argc, argv).
                options(desc).positional(pos).
                extra_parser(at_option_parser).run(), vm);

        po::notify(vm);

        if (vm.count("response-file")) {
            // Load the file and tokenize it
            ifstream ifs(vm["response-file"].as<string>().c_str());
            if (!ifs) {
                cout << "Could not open the response file\n";
                exit(1);
            }
            // Read the whole file into a string
            stringstream ss;
            ss << ifs.rdbuf();
            // Split the file content
            boost::char_separator<char> sep(" \n\r");
            std::string ResponsefileContents(ss.str());
            boost::tokenizer<boost::char_separator<char> > tok(ResponsefileContents, sep);
            vector<string> args(tok.begin(), tok.end());
            // Parse the file and store the options
            po::store(po::command_line_parser(args).options(desc).run(), vm);
        }
        if (vm.count("config-file")) {
            ifstream ifs(vm["config-file"].as<string>());
            po::store(po::parse_config_file(ifs, desc), vm);
        }

    }
    catch (boost::program_options::error &err) {
        if (vm.count("help")) {
            std::cout << desc << endl;
            return 0;
        }
        cerr << err.what() << endl;
        cerr << "\n-----------------------------------\n" << endl;
        cerr << desc << endl;
        return 1;
    }


    if (vm.count("help") || vm.empty()) {
        std::cout << desc << endl;
        return 0;
    }

    boost::optional<fs::ifstream> fstream;
    if (vm.count("input"))
        fstream.emplace(input, std::fstream::binary);

    using iterator_t = boost::spirit::multi_pass<std::istreambuf_iterator<char>>;
    auto itr = fstream ? iterator_t(*fstream) : iterator_t(std::cin);
    const iterator_t end;

    char nullchar{0};
    std::size_t intLength;
    std::size_t ptrLength;

    std::uint64_t initLoc = 0ull;

    enum {bigEndian, littleEndian} endianess;

    {
        int idx = 0;
        std::vector<char> intToken;
        std::vector<char> ptrToken;
        //parse the header
        auto setNullchar  = [&](auto &ctx){nullchar  = x3::_attr(ctx);};
        auto setIntLength = [&](auto &ctx){intLength = x3::_attr(ctx); intToken.reserve(intLength);};
        auto addToTokenI  = [&](auto &ctx)
                            {
                                if (intLength != idx)
                                {
                                    intToken.push_back(x3::_attr(ctx));
                                    idx++;
                                }
                                else
                                    x3::_pass(ctx) = false;
                            };
        auto checkTokenI = [&](auto &ctx){x3::_pass(ctx) = (intLength == idx);};

        auto setPtrLength = [&](auto &ctx){ptrLength = x3::_attr(ctx); ptrToken.reserve(ptrLength);};
        auto addToTokenP  = [&](auto &ctx)
        {
            if (ptrLength != idx)
            {
                ptrToken.push_back(x3::_attr(ctx));
                idx++;
            }
            else
                x3::_pass(ctx) = false;
        };
        auto checkTokenP = [&](auto &ctx){x3::_pass(ctx) = (ptrLength == idx);};

        auto resetIndex = [&](auto &) {idx = 0;};
        auto res = x3::parse(itr, end, *(x3::char_ - (x3::eoi | x3::string("metal rulz")))
                >> "metal rulz" >> x3::byte_[setNullchar]
                >> x3::byte_[setIntLength]
                >> +x3::byte_[addToTokenI]
                >> x3::eps[checkTokenI]
                >> x3::char_('\0')[resetIndex]
                >> x3::byte_[setPtrLength]
                >> +x3::byte_[addToTokenP]
                >> x3::eps[checkTokenP]
                >> x3::char_('\0')
        );

        if (!res)
        {
            std::cerr << "Header parsing failed" << std::endl;
            return 1;
        }
        //0b11001100
        //  11000011
        switch (intLength)
        {
            case 1:
                if (intToken[0] == 0b01000011)
                    endianess = bigEndian;
                else if (intToken[0] == 0b01101100)
                    endianess = littleEndian;
                break;
            case 2:
                if ((intToken[0] == 0b01000011) && (intToken[1] == 0b01101100))
                    endianess = bigEndian;
                else if ((intToken[0] == 0b01101100) && (intToken[1] == 0b01000011))
                    endianess = littleEndian;
                else
                {
                    std::cerr << "Error determining endian type" << std::endl;
                    return 1;
                }
                break;
            case 4:
                if ((intToken[0] == 0b01000011) && (intToken[1] == 0b01101100) && (intToken[2] == 0) && intToken[3] == 0)
                    endianess = bigEndian;
                else if ((intToken[0] == 0) && (intToken[1] == 0) && (intToken[2] == 0b01101100) && intToken[3] == 0b01001100)
                    endianess = littleEndian;
                else
                {
                    std::cerr << "Error determining endian type from " << std::endl;
                    return 1;
                }
                break;
        }

        if (endianess == bigEndian)
            for (auto i = 0u; i<ptrLength; i++)
                reinterpret_cast<char*>(&initLoc)[i] = ptrToken[i];
        else
            for (auto i = 0u; i<ptrLength; i++)
                reinterpret_cast<char*>(&initLoc)[(ptrLength - i) - 1] = ptrToken[i];

        for (auto i = 0u; i<ptrLength; i++)
            std::cerr << "foo: " << (int)ptrToken[i] << std::endl;
    }


    bp::ipstream pin;
    bp::opstream pout;
    bp::child ch(bp::search_path(addr2line), bp::std_in < pout, bp::std_out > pin);

    std::uint64_t startPtr;


    pout << std::hex;
    pout << initLoc << std::endl;
    std:cerr << std::hex << "Foobar" << initLoc << std::endl;
    std::string line;

    while(std::getline(pin, line))
        std::cerr << "Line: " << line << std::endl;





    pin.pipe().close();
    ch.wait();

    return 0;
}

