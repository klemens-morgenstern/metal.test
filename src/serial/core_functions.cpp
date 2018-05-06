/**
 * @file   serial/core_functions.cpp
 * @date   03.05.2018
 * @author Klemens D. Morgenstern
 *
 */

#include <iostream>
#include "core_functions.hpp"
#include <boost/format.hpp>
#include <boost/algorithm/string/predicate.hpp>

void printf_impl(metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    auto format = args.at(0);
    format.pop_back();
    format.erase(format.begin());
    boost::format ft(format);

    for (auto itr = args.begin() + 1; itr< args.end(); itr++)
    {
        auto & arg = *itr;

        if (boost::starts_with(arg, "BYTE"))
            ft = ft % session.get_char();
        else if (boost::starts_with(arg, "INT"))
            ft = ft % session.get_int();
        else if (boost::starts_with(arg, "UINT"))
            ft = ft % session.get_uint();
        else if (boost::starts_with(arg, "STR"))
            ft = ft % session.get_str();
        else if (boost::starts_with(arg, "PTR"))
            ft = ft % session.get_ptr();
        else if (boost::starts_with(arg, "MEMORY"))
        {
            auto raw = session.get_raw();
            ft = ft % std::string(raw.begin(), raw.end());
        }
    }
    std::cout << ft << std::endl;
}

void exit_impl  (metal::serial::session& session, const std::vector<std::string> & args, const std::string & file, int line)
{
    auto code = session.get_int();
    std::cerr << "Exiting serial execution with " << code << std::endl;
    session.set_exit(code);
}
