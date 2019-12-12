//
// Created by kleme on 12.12.2019.
//

#ifndef METAL_TEST_PYTHON_MODULE_HPP
#define METAL_TEST_PYTHON_MODULE_HPP

#include <filesystem>
#include <boost/program_options/options_description.hpp>
#include <pybind11/pybind11.h>
#include <metal/serial/session.hpp>
#include <variant>

namespace metal
{
namespace serial
{

class python_module
{
    static bool _py_init;
    std::filesystem::path _module_path;
    std::unordered_map<std::string, std::variant<std::string, std::vector<std::string>>> _extra_args;
    std::vector<pybind11::object> _plugin_classes;
    std::vector<pybind11::object> _plugin_objects;
public:
    python_module() = delete;

    python_module(std::filesystem::path & module, std::filesystem::path & working_dir);

    python_module(const python_module & ) = delete;
    python_module(python_module && ) = default;

    python_module& operator=(const python_module & ) = delete;
    python_module& operator=(python_module && ) = default;

    void load(boost::program_options::options_description & desc);
    std::unordered_map<std::string, plugin_function_t> load_macros();

};

}
}



#endif //METAL_TEST_PYTHON_MODULE_HPP
