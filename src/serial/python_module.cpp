//
// Created by kleme on 12.12.2019.
//

#include "python_module.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/eval.h>
#include <pybind11/stl.h>
#include <python.h>
#include <iostream>
#include <filesystem>

namespace py = pybind11;
namespace po = boost::program_options;
namespace fs = std::filesystem;
using namespace py::literals;

namespace metal {namespace serial {

bool python_module::_py_init = false;

python_module::python_module(std::filesystem::path & module, std::filesystem::path & working_dir)
    : _module_path(fs::exists(module) ? module : (working_dir / module))
{
    if (!_py_init)
    {
        Py_Initialize();
        _py_init = true;
    }
}

void python_module::load(boost::program_options::options_description & desc)
{
    auto mod = py::module::import("__main__");
    py::object scope = mod.attr("__dict__");

    struct arg_adaptor{ };

    const auto add_argument = [&](arg_adaptor& , const std::string & key, py::kwargs kwargs)
    {
        const auto target      = kwargs.contains("dest") ? py::cast<std::string>(kwargs["dest"]) : key.substr(2, std::string::npos);
        const auto multitoken  = kwargs.contains("action") && (py::cast<std::string>(kwargs["action"]) == "append");
        const auto description = kwargs.contains("help") ? py::cast<std::string>(kwargs["help"]) : "";

        if (multitoken)
        {
            auto & var = (_extra_args[target] = std::vector<std::string>());
            desc.add_options()(target.c_str(), po::value<std::vector<std::string>>(&std::get<1>(var))->multitoken(), description.c_str());
        }
        else
        {
            auto & var = (_extra_args[target] = std::string());
            desc.add_options()(target.c_str(), po::value<std::string>(&std::get<0>(var)), description.c_str());
        }
    };

    arg_adaptor adapter;

    py::enum_<endianess_t>(mod, "endianess")
            .value("big",    endianess_t::big_endian)
            .value("little", endianess_t::little_endian)
            .export_values();

    py::class_<session>(mod, "session")
            .def("endianess", &session::endianess)
            .def("get_str",   &session::get_str)
            .def("get_raw",   &session::get_raw)
            .def("get_int",   &session::get_int)
            .def("get_uint",  &session::get_uint)
            .def("get_uint8", &session::get_uint8)
            .def("get_int8",  &session::get_int8)
            .def("get_char",  &session::get_char)
            .def("get_uint16",&session::get_uint16)
            .def("get_int16", &session::get_int16)
            .def("get_uint32",&session::get_uint32)
            .def("get_int32", &session::get_int32)
            .def("get_uint64",&session::get_uint64)
            .def("get_int64", &session::get_int64)
            .def("get_bool",  &session::get_bool)
            .def("get_ptr",   &session::get_ptr)
            .def("set_exit",  &session::set_exit);

    py::class_<arg_adaptor>(mod, "argument_parser_t").def("add_argument", add_argument);
    mod.def("metal_serial_plugin", [this](py::object & cl) { _plugin_classes.push_back(cl); });

    py::object parser;
    py::dict local = py::dict("argument_parser"_a = adapter);
    py::eval_file(_module_path.string(), scope, local);

}

std::unordered_map<std::string, plugin_function_t> python_module::load_macros()
{
    std::unordered_map<std::string, plugin_function_t> res;
    //_plugin_objects
    for (auto & cl : _plugin_classes)
    {
        std::vector<std::string> funcs;
        for (auto &elem: cl.attr("__dict__"))
            if (std::isalnum(py::cast<std::string>(elem)[0]))
                funcs.emplace_back(py::cast<std::string>(elem));

        bool has_init = cl.attr("__dict__").contains("__init__");

        const auto inst = has_init ? cl(_extra_args) : cl();

        for (const auto &func : funcs)
            res.emplace(
                    func,
                    [inst, func](session& sess, const std::vector<std::string> & args, const std::string & file, int line) mutable
                    {
                        inst.attr(func.c_str())(sess, args, file, line);
                    });

    }

    return res;
}

}
}
