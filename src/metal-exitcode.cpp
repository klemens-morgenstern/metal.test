/**
 * @file   mw-exitcode.cpp
 * @date   12.10.2016
 * @author Klemens D. Morgenstern
 *



 */

#include <metal/debug/break_point.hpp>
#include <metal/debug/frame.hpp>
#include <metal/debug/plugin.hpp>

#include <vector>
#include <memory>


//[exit_stub_decl
struct exit_stub : metal::debug::break_point
{
    exit_stub() : metal::debug::break_point("_exit")
    {
    }

    void invoke(metal::debug::frame & fr, const std::string & file, int line) override;
};
//]

//[exit_stub_invoke
void exit_stub::invoke(metal::debug::frame & fr, const std::string & file, int line)
{
    fr.log() << "***metal-newlib*** Log: Invoking _exit" << std::endl;
    fr.set_exit(std::stoi(fr.arg_list().at(0).value));
}
//]
//[exit_stub_export
void metal_dbg_setup_bps(std::vector<std::unique_ptr<metal::debug::break_point>> & bps)
{
    bps.push_back(std::make_unique<exit_stub>());
};
//]
