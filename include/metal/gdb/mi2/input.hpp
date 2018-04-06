/**
 * @file   /gdb-runner/include/metal/gdb/mi2/input.hpp/input.hpp
 * @date   08.12.2016
 * @author Klemens D. Morgenstern
 *



 */
#ifndef METAL_GDB_MI2_INPUT_HPP_
#define METAL_GDB_MI2_INPUT_HPP_

#include <algorithm>
#include <numeric>

namespace metal
{
namespace gdb
{
namespace mi2
{

/* Input:

27.2.1 gdb/mi Input Syntax

command ==>
    cli-command | mi-command
cli-command ==>
    [ token ] cli-command nl, where cli-command is any existing gdb CLI command.
mi-command ==>
    [ token ] "-" operation ( " " option )* [ " --" ] ( " " parameter )* nl
token ==>
    "any sequence of digits"
option ==>
    "-" parameter [ " " parameter ]
parameter ==>
    non-blank-sequence | c-string
operation ==>
    any of the operations described in this chapter
non-blank-sequence ==>
    anything, provided it doesn't contain special characters such as "-", nl, """ and of course " "
c-string ==>
    """ seven-bit-iso-c-string-content """
nl ==>
    CR | CR-LF

 */

template<std::size_t Size>
constexpr std::size_t str_size(const char(&)[Size])     {return Size;}
inline    std::size_t str_size(const std::string & str) {return str.size();}

constexpr auto nl = "\r";

inline std::string cli_command(const boost::optional<int> &token, const std::string & command)
{
    if (token)
        return std::to_string(*token) + " " + command + nl;
    else
        return command + nl;
}

inline std::string cstring(const std::string & str) {return '"' + str + '"';}



inline std::string mi_command(const boost::optional<int> &token,
                              const std::string& operation,
                              const std::vector<std::string> & options,
                              const std::vector<std::string> & parameters)
{
    auto str_size = [](int sz, const std::string & str){return sz + str.size();};

    std::size_t options_size    = std::accumulate(  options.begin(),  options.end(), options.size() > 0 ? 3 : 0 ,str_size);
    std::size_t parameters_size = std::accumulate(parameters.begin(), parameters.end(), 0 , str_size);

    std::string value;
    //6 is for a possible token.
    value.reserve(6 + operation.size() + options_size + parameters_size );

    if (token)
        value += std::to_string(*token);

    value += "-";
    value += operation;

    for (auto &option : options)
        (value += " ") += option;

    if (parameters.size() > 0)
        value += " --";

    for (auto &parameter : parameters)
        (value += " ") += parameter;

    return value;
}
}
}
}

#endif /* METAL_GDB_MI2_INPUT_HPP_ */
