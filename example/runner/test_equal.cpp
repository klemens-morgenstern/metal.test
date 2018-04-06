
#include <metal/debug/break_point.hpp>
#include <metal/debug/plugin.hpp>
#include <metal/debug/frame.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <memory>

//[test_equal_def
struct test_equal : metal::gdb::break_point
{
    test_equal() : metal::gdb::break_point("test_equal(bool, const char *, const char *, const char*, int)")
    {
    }

    void invoke(metal::gdb::frame & fr, const std::string & file, int line) override;

};
//]

//[test_equal_invoke_1
void test_equal::invoke(metal::gdb::frame & fr, const std::string & file, int line)
{
    bool condition = std::stoi(fr.arg_list(0).value) != 0;
    std::string lhs_name = fr.get_cstring(1);
    std::string rhs_name = fr.get_cstring(2);
    std::string __file__ = fr.get_cstring(3);
    int __line__ = std::stoi(fr.arg_list(4).value);
//]
//[test_equal_invoke_2
    fr.select(1); //select on outer
    std::string lhs_value = fr.print(lhs_name).value;
    std::string rhs_value = fr.print(rhs_name).value;
    fr.select(2);
//]
//[test_equal_invoke_3
    std::cout << __file__ << "(" << __line__ << ")" <<
    std::cout << "equality test [" << lhs_name << " == " << rhs_name << "] ";
    if (condition)
        std::cout << "succeeded: ";
    else
        std::cout << "failed: ";

    std::cout << "[" << lhs_value << " == " << rhs_name << "]" << std::endl;
}
//]

std::vector<std::unique_ptr<metal::gdb::break_point>> metal_gdb_setup_bps()
{
    std::vector<std::unique_ptr<metal::gdb::break_point>> vec;

    vec.push_back(std::make_unique<test_equal>());
    return vec;
};


