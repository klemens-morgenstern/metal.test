/**
 * @file   serial/session.hpp
 * @date   03.05.2018
 * @author Klemens D. Morgenstern
 *
 */

#ifndef METAL_SERIAL_SESSION_HPP
#define METAL_SERIAL_SESSION_HPP

#include <unordered_map>
#include <boost/program_options/options_description.hpp>
#include <string>
#include <vector>
#include <cstdint>

namespace metal
{
namespace serial
{

enum endianess_t {big_endian, little_endian};

class session;

using plugin_function_t = std::function<void(session&, const std::vector<std::string> &,
                                             const std::string & file, int line)>;

struct session
{
    virtual endianess_t endianess() = 0;

    virtual std::string get_str() = 0;
    virtual std::vector<char> get_raw() = 0;

    virtual std:: int64_t get_int()  = 0;
    virtual std::uint64_t get_uint() = 0;

    virtual std::uint8_t get_uint8() = 0;
    virtual std:: int8_t  get_int8() = 0;
    virtual char get_char() = 0;

    virtual std::uint16_t get_uint16() = 0;
    virtual std:: int16_t  get_int16() = 0;

    virtual std::uint32_t get_uint32() = 0;
    virtual std:: int32_t  get_int32() = 0;

    virtual std::uint64_t get_uint64() = 0;
    virtual std:: int64_t  get_int64() = 0;

    virtual bool get_bool() = 0;
    virtual std::uint64_t get_ptr() = 0;

    virtual void set_exit(int code) = 0;

    virtual ~session() = default;
};

}
}

///This function is the central function needed to provide a break-point plugin.
extern "C" BOOST_SYMBOL_EXPORT void metal_serial_setup_entries(std::unordered_map<std::string,
                                                               metal::serial::plugin_function_t> &);
///This function can be used to add program options for the plugin.
extern "C" BOOST_SYMBOL_EXPORT void metal_serial_setup_options(boost::program_options::options_description & po);


#endif //METAL_SERIAL_SESSION_HPP
