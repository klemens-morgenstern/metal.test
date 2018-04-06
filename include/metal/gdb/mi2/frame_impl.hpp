/**
 * @file  metal/gdb/detail/frame_impl.hpp
 * @date   01.08.2016
 * @author Klemens D. Morgenstern
 *



 */
#ifndef METAL_GDB_DETAIL_FRAME_IMPL_HPP_
#define METAL_GDB_DETAIL_FRAME_IMPL_HPP_

#include <metal/gdb/process.hpp>

namespace metal { namespace gdb { namespace mi2 {


metal::debug::var parse_var(interpreter &interpreter_, const std::string & id, std::string value);

struct frame_impl : metal::debug::frame
{
    std::unordered_map<std::string, std::uint64_t> regs() override;
    void set(const std::string &var, const std::string & val) override;
    void set(const std::string &var, std::size_t idx, const std::string & val) override;
    boost::optional<metal::debug::var> call(const std::string & cl) override;
    metal::debug::var print(const std::string & pt, bool bitwise) override;
    void return_(const std::string & value) override;
    frame_impl(std::string &&id,
               std::vector<metal::debug::arg> && args,
               process & proc,
               mi2::interpreter & interpreter,
               std::ostream & log_)
            : metal::debug::frame(std::move(id), std::move(args)), proc(proc), _interpreter(interpreter), _log(log_)
    {
    }
    void set_exit(int code) override;
    void select(int frame) override;
    virtual std::vector<metal::debug::backtrace_elem> backtrace() override;

    std::size_t get_size(const std::string pt);

    boost::optional<metal::debug::address_info> addr2line(std::uint64_t addr) const override;

    void disable(const metal::debug::break_point & bp) override;
    void enable (const metal::debug::break_point & bp) override;

    std::vector<std::uint8_t> read_memory(std::uint64_t addr, std::size_t size) override;
    void write_memory(std::uint64_t addr, const std::vector<std::uint8_t> &vec) override;


    std::ostream & log() override { return _log; }

    metal::debug::interpreter & interpreter() override {return _interpreter; }

    process & proc;
    metal::gdb::mi2::interpreter & _interpreter;
    std::ostream & _log;
};



}}}



#endif /* METAL_GDB_DETAIL_FRAME_IMPL_HPP_ */
