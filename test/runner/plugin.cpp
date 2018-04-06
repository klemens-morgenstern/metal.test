#include <boost/dll/alias.hpp>
#include <metal/debug/break_point.hpp>
#include <metal/debug/frame.hpp>
#include <metal/debug/plugin.hpp>
#include <vector>
#include <memory>
#include <iostream>

using namespace metal::debug;


struct f_ptr : break_point
{
    f_ptr() : break_point("f(int*)")
    {

    }

    void invoke(frame & fr, const std::string & file, int line) override
    {
        std::cerr << file << "(" << line << "): " << "f(int*)" << std::endl;
        fr.set("p", 0, "1");
        fr.set("p", 1, "2");
        fr.set("p", 2, "3");
    }
};

struct f_ref : break_point
{
    f_ref() : break_point("f(int&)")
    {
    }

    void invoke(frame & fr, const std::string & file, int line) override
    {
        std::cerr << file << "(" << line << "): " << "f(int&)" << std::endl;
        fr.set("ref", "42");
    }
};



struct f_ret : break_point
{
    f_ret() : break_point("f()")
    {

    }
    void invoke(frame & fr, const std::string & file, int line) override
    {
        std::cerr << file << "(" << line << "): " << "f()" << std::endl;
        fr.return_("42");
    }
};

void metal_dbg_setup_bps(std::vector<std::unique_ptr<metal::debug::break_point>> & bps)
{
    bps.push_back(std::make_unique<f_ptr>());
    bps.push_back(std::make_unique<f_ref>());
    bps.push_back(std::make_unique<f_ret>());
};


