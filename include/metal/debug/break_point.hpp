/**
 * @file   metal/debug/break_point.hpp
 * @date   13.06.2016
 * @author Klemens D. Morgenstern
 *



 */

#ifndef METAL_GDB_RUNNER_BREAK_POINT_HPP_
#define METAL_GDB_RUNNER_BREAK_POINT_HPP_

#include <metal/debug/frame.hpp>
#include <string>
#include <unordered_map>
#include <iostream>

namespace metal {
namespace debug {

/** This class is used to implement a break_point, i.e. is inherited by an implementation.
 *
 */
class break_point
{
    std::string _identifier;
    boost::optional<std::string> _condition;
public:
    ///Returns the identifier string, either the functions name or the location
    const std::string& identifier() const {return _identifier;}

    ///Return the condition of the breakpoint
    const boost::optional<std::string> &condition() {return _condition;}

    /**Construct the breakpoint from a function identifier.
     * The function signature can either be the pure name of the function,
     * which will yield several breakpoints or you can be an explicit overload.
     *
     * @note If you want to define the overload, the full type names must be used, not the aliases.
     * E.g. for function `void foo(std::string &);` func_name must be
     * `foo(std::basic_string<char, std::char_traits<char>, std::allocator<char>>&)`.
     *
     * @param func_name The Identifier.
     */
    break_point(const std::string & func_name) : _identifier(func_name) {}
    /**Construct a breakpoint from a location in source-code.
     *
     * @param file_name The name of the source file.
     * @param line The line of the breakpoint in the source file
     */
    break_point(const std::string & file_name, std::size_t line)
            : _identifier(file_name + ":" + std::to_string(line)) {}
    /**This function will be called when the breakpoint is reached.
     *
     * @param fr The data frame of the breakpoint.
     * @param file The file the breakpoint is in.
     * @param line The line in the file.
     */
    virtual void invoke(frame & fr, const std::string & file, int line) = 0;
    /** This function is called when the breakpoint is set once.
     * It is meant to be overridden, this functions is empty.
     *
     * @param addr The address of the location in the binary
     * @param file The file the breakpoint is in
     * @param line The line of the breakpoint in the file.
     */
    virtual void set_at(std::uint64_t addr, const std::string & file, int line) {}
    /** This function is called when the breakpoint is set multiple times.
     * It is meant to be overridden, this functions is empty.
     *
     * @param addr The address of the location in the binary.
     * @param name The name of the breakpoint.
     * @param cnt Number of breakpoints set.
     */
    virtual void set_multiple(std::uint64_t addr, std::string & name, int cnt) {}
    /** This function is called when the breakpoint target could not be found.
     * It is meant to be overridden, this functions is empty.
     */
    virtual void set_not_found() {};

    /** Add a condition for this breakpoint.
     *
     * @param condition A code snippet for the breakpoint condition.
     */
    void set_condition(const std::string & condition) {_condition = condition;}

    ///Destructor
    virtual ~break_point() = default;

};




} /* namespace gdb_runner */
} /* namespace metal */

#endif /* METAL_GDB_RUNNER_BREAK_POINT_HPP_ */
