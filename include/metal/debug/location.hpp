/**
 * @file   metal/gdb/location.hpp
 * @date   01.08.2016
 * @author Klemens D. Morgenstern
 *



 */
#ifndef METAL_GDB_LOCATION_HPP_
#define METAL_GDB_LOCATION_HPP_

#include <string>

namespace metal
{
namespace debug
{
///Simple structure representing a location in code
struct location
{
    std::string file; ///<The file the location is in.
    int line;         ///<The line in the file.
};

}
}


#endif /* METAL_GDB_LOCATION_HPP_ */
