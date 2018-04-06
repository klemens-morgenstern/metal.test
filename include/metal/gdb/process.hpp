/**
 * @file   metal/gdb/process.hpp
 * @date   13.06.2016
 * @author Klemens D. Morgenstern
 *



 */

#ifndef METAL_GDB_PROCESS_H_
#define METAL_GDB_PROCESS_H_


#include <boost/asio/io_service.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/deadline_timer.hpp>

#include <metal/debug/process.hpp>
#include <boost/process/child.hpp>
#include <boost/process/async_pipe.hpp>

#include <metal/gdb/mi2/interpreter.hpp>

#include <iterator>

#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <regex>
#include <map>


namespace metal {
namespace gdb {

using metal::debug::break_point;

class BOOST_SYMBOL_EXPORT process : public metal::debug::process
{

    std::map<int, break_point*>               _break_point_map;
    void _run_impl(boost::asio::yield_context &yield) override;

    void _read_header(mi2::interpreter & interpreter);
    void _set_breakpoint(break_point & p, mi2::interpreter & interpreter, const std::regex & rx);
    void _handle_breakpoint(mi2::interpreter & interpreter, const std::vector<std::string> & buf, std::smatch & sm);
    void _err_read_handler(const boost::system::error_code & ec);

    void _set_info(const std::string & version, const std::string & toolset, const std::string & config)
    {
        _log << "GDB Version \"" << version << '"' << std::endl;
        _log << "GDB Toolset \"" << toolset << '"' << std::endl;
        _log << "Config      \"" << config  << '"' << std::endl;
    }

    void _read_info   (mi2::interpreter & interpreter);
    void _init_bps    (mi2::interpreter & interpreter);
    void _start       (mi2::interpreter & interpreter);
    void _start_remote(mi2::interpreter & interpreter);
    void _start_local (mi2::interpreter & interpreter);
    void _handle_bps  (mi2::interpreter & interpreter);

public:
    void reset_timer();

    const std::map<int, break_point*> & break_point_map() const {return _break_point_map;}

    process(const boost::filesystem::path & gdb, const std::string & exe, const std::vector<std::string> & args = {});
    ~process() = default;
    void run() override;
};

} /* namespace gdb_runner */
} /* namespace metal */

#endif /* METAL_GDB_RUNNER_GDB_PROCESS_H_ */
