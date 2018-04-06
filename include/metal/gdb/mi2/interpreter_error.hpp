/**
 * @file   /metal/gdb/mi2/interpreter_error.hpp
 * @date   21.12.2016
 * @author Klemens D. Morgenstern
 *



 */
#ifndef METAL_GDB_MI2_INTERPRETER_ERROR_HPP_
#define METAL_GDB_MI2_INTERPRETER_ERROR_HPP_

#include <exception>
#include <metal/debug/interpreter.hpp>

namespace metal
{
namespace gdb
{
namespace mi2
{
///Exception for an error occuring in the gdb interpreter.
struct interpreter_error : metal::debug::interpreter_error
{
    using metal::debug::interpreter_error::interpreter_error;
    using metal::debug::interpreter_error::operator=;
};

struct parser_error : interpreter_error
{
    using interpreter_error::interpreter_error;
    using interpreter_error::operator=;
};


}
}
}



#endif /* METAL_GDB_MI2_INTERPRETER_ERROR_HPP_ */
