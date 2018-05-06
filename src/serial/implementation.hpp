/**
 * @file   serial/implementation.hpp
 * @date   03.05.2018
 * @author Klemens D. Morgenstern
 *
 */

#ifndef METAL_SERIAL_IMPLEMENTATION_HPP
#define METAL_SERIAL_IMPLEMENTATION_HPP

#include <metal/serial/session.hpp>
#include <iterator>
#include <boost/filesystem/path.hpp>
#include <boost/spirit/home/support/multi_pass.hpp>
#include <boost/optional.hpp>
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
struct parser_exception : std::runtime_error {using std::runtime_error::runtime_error;};

bool init_session(iterator_t & itr, const iterator_t & end, char & nullchar,
                  int & intLength, int & ptrLength, metal::serial::endianess_t &endianess, std::uint64_t &init_loc);

int run_serial(const std::string binary, const boost::filesystem::path &source_dir, const boost::filesystem::path addr2line,
               iterator_t &itr, const iterator_t &end, char nullchar, int intLength, int ptrLength,
               const std::unordered_map<std::string, metal::serial::plugin_function_t> &macros, std::uint64_t init_loc,
               metal::serial::endianess_t endianess, bool ignore_exit_code);

std::unique_ptr<metal::serial::session> build_session(iterator_t & itr, const iterator_t & end, char nullchar,
                                                      int int_length, int ptr_length, metal::serial::endianess_t endianess);

void set_session_loc(std::unique_ptr<metal::serial::session> & session, const std::string& file_name, int line_number);
boost::optional<int> get_exited(std::unique_ptr<metal::serial::session> & session);

#endif //METAL_SERIAL_IMPLEMENTATION_HPP
