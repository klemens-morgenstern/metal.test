/**
 * @file   /metal/gdb/mi2/stuff.hpp
 * @date   20.12.2016
 * @author Klemens D. Morgenstern
 *



 */
#ifndef METAL_GDB_MI2_TYPES_HPP_
#define METAL_GDB_MI2_TYPES_HPP_

#include <tuple>
#include <utility>
#include <stdexcept>
#include <boost/blank.hpp>
#include <string>
#include <vector>
#include <boost/optional.hpp>
#include <boost/variant/variant.hpp>
#include <metal/gdb/mi2/interpreter_error.hpp>
#include <metal/gdb/mi2/output.hpp>

namespace metal {
namespace gdb {
namespace mi2 {

const value& find(const std::vector<result> & input, const char * id);
boost::optional<const value&> find_if(const std::vector<result> & input, const char * id);

struct async_output;
struct result_output;

template<typename T> T parse_result(const std::vector<result>  &);

struct missing_value : interpreter_error
{
    std::string value_name;

    missing_value(const std::string & value_name) : interpreter_error("missing value '" + value_name + "' in result"), value_name(value_name) {}

    using interpreter_error::operator=;
};

struct async_result
{
    std::string reason;
    std::vector<result> content;

    template<typename T>
    T as() const {return parse_result<T>(content);}

    const value & find(const char* key)
    {
        return metal::gdb::mi2::find(content, key);
    }

    boost::optional<const value &> find_if(const char* key)
    {
        return metal::gdb::mi2::find_if(content, key);
    }
};

struct error_
{
    std::string msg;
    boost::optional<std::string> code;
};

struct exception : interpreter_error
{
    error_ err;

    static std::string make_msg(const error_ & err)
    {
        if (err.code)
            return "[" + *err.code + "]: " + err.msg;
        else
            return err.msg;
    }
    exception(const error_ & err) : interpreter_error(make_msg(err)) ,err(err) {}
};

struct running
{
    std::string thread_id;
};

struct stopped
{
    enum reason_t
    {
        breakpoint_hit,            ///<A breakpoint was reached.
        watchpoint_trigger,        ///<A watchpoint was triggered.
        read_watchpoint_trigger,   ///<A read watchpoint was triggered.
        access_watchpoint_trigger, ///<An access watchpoint was triggered.
        function_finished,         ///<An _exec_finish or similar CLI command was accomplished.
        location_reached,          ///<An _exec_until or similar CLI command was accomplished.
        watchpoint_scope,          ///<A watchpoint has gone out of scope.
        end_stepping_range,        ///<An _exec_next, _exec_next_instruction, _exec_step, _exec_step_instruction or similar CLI command was accomplished.
        exited_signalled,          ///<The inferior exited because of a signal.
        exited,                    ///<The inferior exited.
        exited_normally,           ///<The inferior exited normally.
        signal_received,           ///<A signal was received by the inferior.
        solib_event,               ///<The inferior has stopped due to a library being loaded or unloaded. This can happen when stop_on_solib_events (see Files) is set or when a catch load or catch unload catchpoint is in use (see Set Catchpoints).
        fork,                      ///<The inferior has forked. This is reported when catch fork (see Set Catchpoints) has been used.
        vfork,                     ///<The inferior has vforked. This is reported in when catch vfork (see Set Catchpoints) has been used.
        syscall_entry,             ///<The inferior entered a system call. This is reported when catch syscall (see Set Catchpoints) has been used.
        syscall_return,            ///<The inferior returned from a system call. This is reported when catch syscall (see Set Catchpoints) has been used.
        exec                       ///<The inferior called exec. This is reported when catch exec (see Set Catchpoints) has been used.
    } reason;
    std::string stopped_;
    std::string core;
};


struct thread_group_added   { int id; };
struct thread_group_removed { int id; };

struct thread_group_started
{
    int id;
    int pid;
};


struct thread_group_exited
{
    int id;
    boost::optional<int> exited;
};

struct thread_created
{
    int id;
    int gid;
};

struct thread_exited
{
    int id;
    int gid;
};

struct thread_selected
{
    int id;
    int gid;
};

struct library_loaded
{
    int id;
    std::string target_name;
    std::string host_name;
    boost::optional<std::string> symbols_loaded;
};

struct library_unloaded
{
    int id;
    std::string target_name;
    std::string host_name;
};

struct traceframe_changed_t
{
    int num;
    std::string tracepoint;
};

struct traceframe_changed_end
{
    boost::blank end;
};

struct traceframe_changed : boost::variant<traceframe_changed_t, traceframe_changed_end>
{
    using father_type = boost::variant<traceframe_changed_t, traceframe_changed_end>;
    using father_type::variant;
    using father_type::operator=;
};

struct tsv_frame
{
    boost::optional<std::string> name;
    boost::optional<std::string> initial;
};

struct tsv_modified
{
    std::string name;
    std::string initial;
    boost::optional<std::string> current;
};

//https://sourceware.org/gdb/onlinedocs/gdb/GDB_002fMI-Breakpoint-Information.html#GDB_002fMI-Breakpoint-Information
struct breakpoint
{
    int number;
    std::string type;
    boost::optional<std::string> catch_type;
    std::string disp;
    bool enabled;
    std::uint64_t addr;
    boost::optional<std::string> func;
    boost::optional<std::string> filename;
    boost::optional<std::string> fullname;
    boost::optional<int> line;
    boost::optional<std::vector<std::string>> thread_groups;
    boost::optional<std::string> at;
    boost::optional<std::string> pending;
    boost::optional<std::string> evaluated_by;
    boost::optional<std::string> thread;
    boost::optional<std::string> task;
    boost::optional<std::string> cond;
    boost::optional<int> ignore;
    boost::optional<int> enable;
    boost::optional<std::string> traceframe_usage;
    boost::optional<std::string> static_tracepoint_marker_string_id;
    boost::optional<std::string> mask;
    boost::optional<int> pass;
    boost::optional<std::string> original_location;
    int times;
    boost::optional<bool> installed;
    boost::optional<std::string> what;
};

struct breakpoint_created  { breakpoint bkpt; };
struct breakpoint_modified { breakpoint bkpt; };
struct breakpoint_deleted  { int number;};

struct record_started
{
    int thread_group;
    std::string method;
    boost::optional<std::string> format;
};

struct record_stopped { int thread_group;};

struct cmd_param_changed
{
    std::string param;
    std::string value;
};

struct memory_changed
{
    int thread_group;
    std::uint64_t addr;
    int len;
    boost::optional<std::string> code;
};

struct arg
{
    std::string name;
    std::string value;
    boost::optional<std::string> type;
};

struct varobj
{
    std::string name;
    int numchild;
    std::string value;
    std::string type;
    boost::optional<int> thread_id;
    bool has_more;
    bool dynamic;
    boost::optional<std::string> displayhint;
    boost::optional<std::string> exp;
    bool frozen;
};

struct varobj_update
{
    std::string name;
    std::string value;
    boost::optional<bool> in_scope;
    boost::optional<bool> type_changed;
    boost::optional<std::string> new_type;
    boost::optional<int> new_num_children;
    boost::optional<std::string> displayhint;
    bool has_more;
    bool dynamic;
    std::vector<std::string> new_children;
};

struct frame
{
    int level;
    boost::optional<std::string> func;
    boost::optional<std::uint64_t> addr;
    boost::optional<std::string> file;
    boost::optional<int> line;
    boost::optional<std::string> from;
    boost::optional<std::vector<arg>> args;
};

struct thread_info
{
    int id;
    std::string target_id;
    boost::optional<std::string> details;
    enum state_t
    {
        stopped, running
    } state;
    boost::optional<int> core;
    boost::optional<struct frame> frame;
};

struct thread_state
{
    std::vector<thread_info> threads;
    int current_thread_id;
};

struct thread_id_list
{
    std::vector<int> thread_ids;
    int current_thread_id;
    int number_of_threads;
};

struct thread_select
{
    boost::optional<struct frame> frame;
    int new_thread_id;
    std::vector<arg> args;
};

struct thread
{
    int id;
    std::string target_id;
    std::vector<frame> frames;
    std::string state;
};

struct groups
{
    int id;
    std::string type;
    boost::optional<int> pid;
    boost::optional<int> exit_code;
    boost::optional<int> num_children;
    boost::optional<std::vector<thread>> threads;
    boost::optional<std::vector<int>> cores;
    boost::optional<std::string> executable;
};

struct os_info
{
    std::vector<std::string> header;
    std::vector<std::vector<std::string>> values;
};

struct watchpoint
{
    int number;
    std::string exp;
};

struct ada_task_info
{
    boost::optional<std::string> current;
    int id;
    int task_id;
    boost::optional<int> thread_id;
    boost::optional<int> parent_id;
    int priority;
    std::string state;
    std::string name;
};

enum class print_values
{
    no_values = 0,
    all_values = 1,
    simple_values = 2,
};

enum class format_spec
{
    binary,
    decimal,
    hexadecimal,
    octal,
    natural,
    zero_hexadecimal,
    raw
};

enum class output_format
{
    hexadecimal,
    signed_,
    unsigned_,
    octal,
    bianry,
    address,
    character,
    floating,
    string,
    padded_hex,
    raw
};

inline char to_char(output_format of)
{
    switch(of)
    {
        case output_format::hexadecimal: return 'x';
        case output_format::signed_:     return 'd';
        case output_format::unsigned_:   return 'u';
        case output_format::octal:       return 'o';
        case output_format::bianry:      return 't';
        case output_format::address:     return 'a';
        case output_format::character:   return 'c';
        case output_format::floating:    return 'f';
        case output_format::string:      return 's';
        case output_format::padded_hex:  return 'z';
        case output_format::raw:         return 'r';
        default: return '\0';
    }
}

struct linespec_location
{
    boost::optional<std::size_t> linenum;
    boost::optional<int>         offset;
    boost::optional<std::string> filename;
    boost::optional<std::string> function;
    boost::optional<std::string> label;

    linespec_location() = default;
    linespec_location(const linespec_location&) = default;
    linespec_location(linespec_location&&) = default;
    linespec_location(std::size_t linenum) : linenum(linenum) {}
    linespec_location(const std::string & filename, std::size_t linenum)
            : linenum(linenum), filename(filename) {}

    linespec_location(const std::string & function) : function(function) {}
};

struct explicit_location
{
    boost::optional<std::string> source;
    boost::optional<std::string> function;
    boost::optional<std::string> label;
    boost::optional<std::size_t> line;
    boost::optional<int> line_offset;
};

struct address_location
{

    boost::optional<std::string> expression;
    boost::optional<std::uint64_t> funcaddr;
    boost::optional<std::string> filename;

    address_location() = default;
    address_location(const address_location&) = default;
    address_location(address_location&&) = default;
    address_location(const std::string & expression) : expression(expression) {}
    address_location(std::uint64_t funcaddr) : funcaddr(funcaddr) {}
    address_location(const std::string & filename, std::uint64_t funcaddr)
        : funcaddr(funcaddr), filename(filename) {}
};

enum class disassemble_mode
{
    disassembly_only = 0,
    mixed_source_and_disassembly_deprecated = 1,
    disassembly_with_raw_opcodes = 2,
    mixed_source_and_disassembly_with_raw_opcodes_deprecated = 3,
    mixed_source_and_disassembly                  = 4,
    mixed_source_and_disassembly_with_raw_opcodes = 5
};

struct line_asm_insn
{
    std::uint64_t address;
    std::string func_name;
    std::size_t offset;
    std::string inst;
    boost::optional<std::string>  opcodes;
};

struct src_and_asm_line
{
    int line;
    std::string file;
    boost::optional<std::string> fullname;
    boost::optional<std::vector<struct line_asm_insn>> line_asm_insn;
};

struct register_value
{
    std::size_t number;
    std::string value;
};

struct memory_entry
{
    std::uint64_t addr;
    std::vector<std::uint8_t> data;
    boost::optional<std::string> ascii;
};

struct read_memory
{
    std::uint64_t addr;
    std::size_t nr_bytes;
    std::size_t total_bytes;
    std::uint64_t next_row;
    std::uint64_t prev_row;
    std::uint64_t next_page;
    std::uint64_t prev_page;
    std::vector<memory_entry> memory;
};

struct read_memory_bytes
{
    std::uint64_t begin;
    std::uint64_t offset;
    std::uint64_t end;
    std::vector<std::uint8_t> contents;
};

struct found_tracepoint
{
    int traceframe;
    int tracepoint;
    boost::optional<struct frame> frame;
};

struct memory_region
{
    std::uint64_t address;
    std::size_t length;
    boost::optional<std::vector<std::uint8_t>> contents;
};

struct traceframe_collection
{
    std::vector<register_value> explicit_variables;
    std::vector<register_value> computed_expressions;
    std::vector<register_value> registers;
    std::vector<register_value> tvars;
    std::vector<memory_region> memory;
};

struct trace_variable
{
    std::string name;
    std::int64_t initial;
    boost::optional<std::int64_t> current;
};

struct trace_status
{
    bool supported;
    bool running;
    boost::optional<std::string> stop_reason;
    boost::optional<int> stopping_tracepoint;
    boost::optional<std::size_t> frames;
    boost::optional<std::size_t> frames_created;
    boost::optional<std::size_t> buffer_size;
    boost::optional<std::size_t> buffer_free;
    bool circular;
    bool disconnected;
    boost::optional<std::string> trace_file;
};

struct symbol_line
{
    std::uint64_t pc;
    std::string line;
};

struct source_info
{
    boost::optional<int> line;
    std::string file;
    boost::optional<std::string> fullname;
    boost::optional<std::string> macro_info;
};

struct download_info
{
    std::uint64_t address;
    std::size_t load_size;
    std::size_t transfer_rate;
    std::size_t write_rate;
};


struct download_status
{
    std::string section;
    boost::optional<std::size_t> section_sent;
    boost::optional<std::size_t> total_sent;
    std::size_t section_size;
    std::size_t total_size;
};

struct connection_notification
{
    std::string addr;
    std::string func;
    std::vector<std::string> args;
};

struct info_ada_exception
{
    std::string name;
    std::uint64_t address;
};

}
}
}

#endif /* METAL_GDB_MI2_TYPES_HPP_ */
