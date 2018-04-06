/**
 * @file   metal/debug/process.cpp/process.cpp
 * @date   14.02.2017
 * @author Klemens D. Morgenstern
 *

 */

#include <metal/gdb/process.hpp>

#include <boost/process/io.hpp>
#include <boost/process/async.hpp>
#include <boost/process/search_path.hpp>

namespace bp = boost::process;

using std::endl;

namespace metal {
namespace debug {

process::process(const boost::filesystem::path & gdb, const std::string & exe, const std::vector<std::string> & args)
        : _child(gdb, exe, args, _io_service, bp::std_in < _in, bp::std_out > _out, bp::std_err > _err,
                bp::on_exit([this](int, const std::error_code&){_timer.cancel();_out.async_close(); _err.async_close();}))
{
}

void process::_set_timer()
{
    if (_time_out > 0)
    {
        _timer.expires_from_now(boost::posix_time::seconds(_time_out));
        _timer.async_wait(
                [this](const boost::system::error_code & ec)
                {
                    if (ec == boost::asio::error::operation_aborted)
                        return;
                    if (_child.running())
                        _child.terminate();
                    _io_service.stop();
                    _log << "...Timeout..." << endl;
                    _exit_code = 1;
                });
    }
}

void process::run()
{
    _set_timer();
    if (!_child.running())
    {
        _log << "debugger not running" << endl;
        _terminate();
    }
    _set_timer();
    _log << "Starting run" << endl << endl;

    boost::asio::spawn(_io_service, [this](boost::asio::yield_context yield){_run_impl(yield);});
    _io_service.run();

}

}
}
