/**
 * @file   metal/debug/interpreter_impl.hpp
 * @date   01.02.2017
 * @author Klemens D. Morgenstern
 *



 */
#ifndef METAL_DEBUG_INTERPRETER_IMPL_HPP_
#define METAL_DEBUG_INTERPRETER_IMPL_HPP_

#include <metal/debug/interpreter.hpp>
#include <boost/asio/streambuf.hpp>
#define BOOST_COROUTINE_NO_DEPRECATION_WARNING
#include <boost/asio/spawn.hpp>
#undef BOOST_COROUTINE_NO_DEPRECATION_WARNING
#include <boost/process/async_pipe.hpp>
#include <stdexcept>
#include <iostream>

#include <boost/regex.hpp>

namespace metal
{

namespace debug
{

class interpreter_impl : public interpreter
{
protected:
    boost::process::async_pipe & _out;
    boost::process::async_pipe & _in;

    boost::asio::streambuf _out_buf;
    std::string _in_buf;

    boost::asio::yield_context & _yield;

    std::ostream &_fwd;
    bool _debug = false;
public:
    void enable_debug() {_debug = true;}

    interpreter_impl(boost::process::async_pipe & out,
                boost::process::async_pipe & in,
                boost::asio::yield_context & yield_,
                std::ostream & fwd = std::cout) : _out(out), _in(in), _yield(yield_), _fwd(fwd) {}

    std::vector<std::string> communicate(const std::string & in, char delim);
    std::vector<std::string> communicate(const std::string & in, const std::string & delim);
    std::vector<std::string> communicate(const std::string & in, const boost::regex & expr);


    virtual ~interpreter_impl() = default;
};

}

}



#endif /* METAL_DEBUG_INTERPRETER_HPP_ */
