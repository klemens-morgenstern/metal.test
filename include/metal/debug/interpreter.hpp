/**
 * @file   metal/debug/interpreter.hpp
 * @date   01.02.2017
 * @author Klemens D. Morgenstern
 *



 */
#ifndef METAL_DEBUG_INTERPRETER_HPP_
#define METAL_DEBUG_INTERPRETER_HPP_

#include <stdexcept>

namespace metal
{

namespace debug
{

struct interpreter_error : std::runtime_error
{
    using std::runtime_error::runtime_error;
    using std::runtime_error::operator=;
};

class interpreter
{
public:
    interpreter() = default;

    virtual ~interpreter() = default;
};

}

}



#endif /* METAL_DEBUG_INTERPRETER_HPP_ */
