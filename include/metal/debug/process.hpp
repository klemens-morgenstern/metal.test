/**
 * @file   metal/gdb/process.hpp
 * @date   13.06.2016
 * @author Klemens D. Morgenstern
 *



 */

#ifndef METAL_DEBUG_PROCESS_H_
#define METAL_DEBUG_PROCESS_H_

#include <boost/asio/io_service.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/deadline_timer.hpp>

#include <metal/debug/break_point.hpp>
#include <metal/debug/interpreter.hpp>

#include <boost/process/child.hpp>
#include <boost/process/async_pipe.hpp>

#include <iterator>

#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <regex>
#include <map>

namespace metal {
namespace debug {

class BOOST_SYMBOL_EXPORT process
{
protected:
    bool _enable_debug = false;
    int _time_out = 10;
    int _exit_code = -1;
    std::ofstream  _log;
    boost::asio::io_service _io_service;
    boost::asio::deadline_timer _timer{_io_service , boost::posix_time::seconds(_time_out)};
    boost::process::async_pipe _out{_io_service};
    boost::process::async_pipe _in {_io_service};
    boost::process::async_pipe _err{_io_service};
    boost::asio::streambuf _err_buf;
    boost::asio::streambuf _out_buf;

    boost::process::child _child;

    std::string _remote;
    std::string _program;

    std::vector<std::string> _init_scripts;

    int _pid = -1;
    std::vector<std::uint64_t> _thread_id;
    std::vector<std::string> _args;
    std::vector<std::unique_ptr<break_point>> _break_points;
    bool _exited = false;
    void _set_timer();
    virtual void _terminate()
    {
        BOOST_THROW_EXCEPTION( std::runtime_error("metal::gdb::process panic!") );
    }
    virtual void _run_impl(boost::asio::yield_context &yield) = 0;

public:

    void set_init_script(      std::vector<std::string> && init_scripts) {_init_scripts = std::move(init_scripts);}
    void set_init_scripts(const std::vector<std::string> &  init_scripts) {_init_scripts = init_scripts;}

    bool running() {return _child.running();}
    void set_exit(int code)
    {
        _exited=true;
        _log << "Exited with " << code << std::endl;
        _exit_code = code;
    }
    void set_program(const std::string & program)
    {
        _program = program;
    }
    void set_args(const std::vector<std::string> & args)
    {
        _args = args;
    }
    void set_remote(const std::string & remote)
    {
        _remote = remote;
    }
    void enable_debug() {_enable_debug = true;}

    std::ostream & log() {return _log;}

    process(const boost::filesystem::path & gdb, const std::string & exe, const std::vector<std::string> & args);
    virtual ~process() = default;
    int exit_code() {return _exit_code;}
    void set_log(const std::string & name)
    {
        if (name == "stderr")
            _log.std::ostream::rdbuf(std::cerr.rdbuf());
        else if (name == "stdout")
            _log.std::ostream::rdbuf(std::cout.rdbuf());
        else
            _log.open(name);
    }
    void set_timeout(int value) {_time_out = value;}
    void add_break_point(std::unique_ptr<break_point> && ptr) { _break_points.push_back(std::move(ptr)); }
    void add_break_points(std::vector<std::unique_ptr<break_point>> && ptrs)
    {
        for (auto & in : ptrs)
            _break_points.emplace_back(in.release());
    }
    virtual void run();
};

} /* namespace gdb_runner */
} /* namespace metal */

#endif /* METAL_GDB_RUNNER_GDB_PROCESS_H_ */
