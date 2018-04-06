/**
 * @file   metal/gdb/frame.hpp
 * @date   13.06.2016
 * @author Klemens D. Morgenstern
 *



 */

#ifndef METAL_GDB_FRAME_HPP_
#define METAL_GDB_FRAME_HPP_

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <ostream>
#include <boost/optional.hpp>
#include <metal/debug/location.hpp>
#include <metal/debug/interpreter.hpp>

namespace metal {
namespace debug {

class break_point;

/**This function represents a null-terminated string in gdb.
 * Gdb can add a display for a c-string if a char* is passed to a function.
 *
 * @note GDB can shorten the string and add an ellipsis at the end.
 *
*/
struct cstring_t
{
    std::string value; ///The cstring value extracted from the gdb output.
    bool ellipsis;     ///Is true, if gdb adds an ellipsis at the end of the shortened string.
};

/** This type represents a variable in gdb, as given by frame::print or frame::call.
 *
 */
struct var
{
    boost::optional<std::uint64_t> ref; ///The address of the target the value is a reference.
    std::string value;   ///<The actually value
    cstring_t   cstring; ///<The value as cstring if available.
};

/** This type represents an argument passed to a function. It inherits var and adds the parameter identifier.
 *
 */
struct arg : var
{
    ///The identifier of the parameter.
    std::string id;
};

/** This class represents an entry in the backtrace.
 *
 */
struct backtrace_elem
{
    int cnt; ///<The position in the backtrace
    boost::optional<std::uint64_t> addr; ///<Gives the position the functions
    std::string func; ///<The name of the functions
    location loc; ///< Location of the function called.
};

/// �nformation about a certain address in the program, i.e in source code.
struct address_info
{
    std::string file; ///<The name of the file the code was generated from
    std::size_t line; ///<The line in the file
    boost::optional<std::string> full_name; ///<The absolute path of the file
    boost::optional<std::string> function;  ///<The function this piece of code is part of
    boost::optional<std::uint64_t> offset; ///<The offset in the containing function, if available.
};

/** This class represents a stackframe.
 * A stackframe let's you examine the stack in gdb. A reference to the frame will be passed to the break-point implementation on invocation.
 *
 * For more details have a look at [the gdb documentation](http://ftp.gnu.org/old-gnu/Manuals/gdb/html_chapter/gdb_7.html).
 *
 */
struct frame
{
    ///Get the id of the current frame
    const std::string & id() const {return _id;}
    ///Get the already read argument list. This means it does not need to be read again.
    const std::vector<arg> &arg_list() const {return _arg_list;}
    /** This function is for convenience and let's the user access elements in the argument list.
     *  @param index The index for the element.
     *  @overload const std::vector<arg> &arg_list() const
     */
    const arg &arg_list(std::size_t index) const
    {
        try {
            return _arg_list.at(index);
        }
        catch(std::out_of_range & o)
        {
            BOOST_THROW_EXCEPTION(std::out_of_range("Out of range in frame::arg_list(idx)[" + id() + "]: " + o.what() )) ;
            //for C++ complacancy
            return _arg_list.front();
        }
    }

    /**Examine the program at the given address.
     *
     * @return Only returns a set value, if the address is in the right space.
     */
    virtual boost::optional<address_info> addr2line(std::uint64_t addr) const = 0;
    /** This function returns the cstring of the argument requested, if it is a null-terminated string.
     * This will take care of the possible ellipsis of passed cstrings.
     *
     * @param index Position of the argument the cstring shall be obtained from.
     */
    inline std::string get_cstring(std::size_t index);
    ///Returns the map of the registers and their values.
    virtual std::unordered_map<std::string, std::uint64_t> regs() = 0;
    ///Set a variable in the current frame
    virtual void set(const std::string &var, const std::string & val)   = 0;
    ///Set a member of an array in the current frame
    virtual void set(const std::string &var, std::size_t idx, const std::string & val) = 0;
    ///Call a function in the current frame. If not void the return value holds an instance of gdb::var.
    virtual boost::optional<var> call(const std::string & cl)       = 0;
    /**Print the value of a symbol in the current frame.
     *
     * @param id The name of the symbol
     * @param bitwise If set true, the value will be printed as an array of binary values.
     * @return The printed value
     */
    virtual var print(const std::string & id, bool bitwise = false) = 0;
    /** Return from the current function.
     *
     * @param value The return value, needs to be passed if the return is not void.
     */
    virtual void return_(const std::string & value = "") = 0;
    /** Sets the program to have exited. This is necessary for embedded environments
     * where there is no operating system.
     *
     * @attention Do only use it in an embedded environment.
     * @param code The exit code.
     */
    virtual void set_exit(int code) = 0;
    /** This function switches the current frame.
     * The frame of the break-point location is zero, while adding one is always the next outer frame i.e. function call.
     *
     * \par Example
     *
     * \code{.cpp}
     * void foo()
     * {
     * }
     *
     * void bar()
     * {
     *    int value = 42;
     *    foo();
     * }
     * \endcode
     *
     * If the breakpoint is in `foo()`, selecting frame 1, will yield the frame of bar. E.g.:
     *
     * \code{.cpp}
     * void invoke(frame &fr)
     * {
     *    fr.select(1); //select bar
     *    auto val = fr.print("value");
     *    assert(val.value == "42");
     * }
     * \endcode
     *
     * @param frame The frame to be selected.
     */
    virtual void select(int frame) = 0;
    /**This function returns the backtrace of the current position.
     *
     * @see The the gdb [documentation](https://sourceware.org/gdb/onlinedocs/gdb/Backtrace.html) for more details.
     *
     * @return A vector of the backtrace elements.
     */
    virtual std::vector<backtrace_elem> backtrace() = 0;
    ///This function allows access to the log sink of the gdb-runner.
    virtual std::ostream & log() = 0;
    ///Gives a reference to the interpreter
    virtual class interpreter & interpreter() = 0;

    ///Disable a breakpoint
    virtual void disable(const break_point & bp) = 0;
    ///Reenable a breakpoint
    virtual void enable(const break_point & bp) = 0;
    ///Read a chunk of memory
    virtual std::vector<std::uint8_t> read_memory(std::uint64_t addr, std::size_t size) = 0;
    ///Write a chunk of memory
    virtual void write_memory(std::uint64_t addr, const std::vector<std::uint8_t> &vec) = 0;
protected:
#if !defined(METAL_GDB_DOXYGEN)
    frame(std::string && id, std::vector<arg> && args)
            : _id(std::move(id)), _arg_list(std::move(args))
    {

    }
    virtual ~frame() = default;
    std::string _id;
    std::vector<arg> _arg_list;
#endif
};

std::string frame::get_cstring(std::size_t index)
{
    auto &entry = arg_list(index);

    if (!entry.cstring.ellipsis)
        return entry.cstring.value;
    //has ellipsis, so I'll need to get the rest manually
    auto val = entry.cstring.value;
    auto idx = val.size();

    while(true)
    {
        auto p = print(entry.id + '[' + std::to_string(idx++) + ']');
        auto i = std::stoi(p.value);
        auto value  = static_cast<char>(i);
        if (value == '\0')
            break;
        else
            val.push_back(value);
    }
    return val;
}

} /* namespace gdb_runner */
} /* namespace metal */

#endif /* METAL_GDB_RUNNER_FRAME_HPP_ */
