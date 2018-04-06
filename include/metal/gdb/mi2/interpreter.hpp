/**
 * @file   /metal/gdb/mi2/interpreter.hpp
 * @date   09.12.2016
 * @author Klemens D. Morgenstern
 *



 */
#ifndef METAL_GDB_MI2_INTERPRETER_HPP_
#define METAL_GDB_MI2_INTERPRETER_HPP_

#include <boost/asio/streambuf.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/process/async_pipe.hpp>
#include <boost/signals2/signal.hpp>

#include <metal/gdb/mi2/types.hpp>
#include <metal/gdb/mi2/async_record_handler_t.hpp>
#include <metal/gdb/mi2/output.hpp>
#include <string>
#include <ostream>
#include <functional>
#include <iostream>

#include <metal/debug/interpreter_impl.hpp>

namespace metal
{
namespace gdb
{
namespace mi2
{
///Thrown if the wrong result class is yielded by the operation.
struct unexpected_result_class : interpreter_error
{
    result_class expected;
    result_class got;
    std::string msg;
    unexpected_result_class(result_class ex, result_class got) :
        interpreter_error("unexpected result-class [" + to_string(ex) + " != " + to_string(got) + "]"),
        expected(ex), got(got)
    {

    }

    unexpected_result_class(result_class ex, result_class got, const std::string & msg) :
        interpreter_error("unexpected result-class [" + to_string(ex) + " != " + to_string(got) + "] : \"" + msg + "\""),
            expected(ex), got(got), msg(msg)
    {

    }
};

///Is thrown if the record the interpreter yields is another than expected
struct unexpected_record : interpreter_error
{
    unexpected_record(const std::string & line) : interpreter_error("unexpected record " + line)
    {

    }
};

///Thrown if a async record with token that was not expected is yielded.
struct unexpected_async_record : unexpected_record
{
    unexpected_async_record(std::uint64_t token, const std::string & line) : unexpected_record("unexpected async record [" + std::to_string(token) + "]" + line)
    {

    }
};

///Thrown if a synchronous operation has another token then expected.
struct mismatched_token : unexpected_record
{
    mismatched_token(std::uint64_t expected, std::uint64_t got)
        : unexpected_record("Token mismatch: [" + std::to_string(expected) + " != " + std::to_string(got) + "]")
    {

    }
};


class BOOST_SYMBOL_EXPORT interpreter : public metal::debug::interpreter_impl
{
    boost::signals2::signal<void(const std::string&)> _stream_console;
    boost::signals2::signal<void(const std::string&)> _stream_log;

    boost::signals2::signal<void(const async_output &)> _async_sink;

    std::uint32_t _token_gen = 0;

    static void _throw_unexpected_result(result_class rc, const metal::gdb::mi2::result_output & res);

    void _handle_stream_output(const stream_record & sr);
    void _handle_async_output(const async_output & ao);
    bool _handle_async_output(std::uint64_t token, const async_output & ao);
    template<typename ...Args>
    void _work_impl(Args&&...args);
    void _work();
    void _work(std::uint64_t token, result_class rc);
  //  void _work(const std::function<void(const result_output&)> & func);
    void _work(std::uint64_t, const std::function<void(const result_output&)> & func);

    void _handle_record(const std::string& line, const boost::optional<std::uint64_t> &token, const result_output & sr);
    void _handle_record(const std::string& line, const boost::optional<std::uint64_t> &token, const result_output & sr,
                        std::uint64_t expected_token, result_class rc);
//    void _handle_record(const std::string& line, const boost::optional<std::uint64_t> &token, const stream_record & sr,
//                        const std::function<void(const result_output&)> & func);
    void _handle_record(const std::string& line, const boost::optional<std::uint64_t> &token, const result_output & sr,
                        std::uint64_t expected_token, const std::function<void(const result_output&)> & func);

    std::vector<std::pair<std::uint64_t, std::function<bool(const async_output &)>>> _pending_asyncs;
public:
    interpreter(boost::process::async_pipe & out,
                boost::process::async_pipe & in,
                boost::asio::yield_context & yield_,
                std::ostream & fwd = std::cout);

    boost::signals2::signal<void(const std::string&)> & stream_console_sig() {return _stream_console;}
    boost::signals2::signal<void(const std::string&)> & stream_log_sig() {return _stream_log;}

    async_record_handler_t async_record_handler{_async_sink};

    async_result wait_for_stop();

    //read the opening of the interpreter
    std::string read_header();

    void break_after(int number, int count);
    void break_commands(int number, const std::vector<std::string> & commands);
    void break_condition(int number, const std::string & condition);
    void break_delete(int number);
    void break_delete(const std::vector<int> &numbers);

    void break_disable(int number);
    void break_disable(const std::vector<int> &numbers);

    void break_enable(int number);
    void break_enable(const std::vector<int> &numbers);

    breakpoint break_info(int number);

    std::vector<breakpoint> break_insert(const linespec_location & exp,
            bool temporary = false, bool hardware = false, bool pending = false,
            bool disabled = false, bool tracepoint = false,
            const boost::optional<std::string> & condition = boost::none,
            const boost::optional<int> & ignore_count = boost::none,
            const boost::optional<int> & thread_id = boost::none);

    std::vector<breakpoint> break_insert(const explicit_location & exp,
            bool temporary = false, bool hardware = false, bool pending = false,
            bool disabled = false, bool tracepoint = false,
            const boost::optional<std::string> & condition = boost::none,
            const boost::optional<int> & ignore_count = boost::none,
            const boost::optional<int> & thread_id = boost::none);

    std::vector<breakpoint> break_insert(const address_location & exp,
            bool temporary = false, bool hardware = false, bool pending = false,
            bool disabled = false, bool tracepoint = false,
            const boost::optional<std::string> & condition = boost::none,
            const boost::optional<int> & ignore_count = boost::none,
            const boost::optional<int> & thread_id = boost::none);

    std::vector<breakpoint> break_insert(const std::string & location,
            bool temporary = false, bool hardware = false, bool pending = false,
            bool disabled = false, bool tracepoint = false,
            const boost::optional<std::string> & condition = boost::none,
            const boost::optional<int> & ignore_count = boost::none,
            const boost::optional<int> & thread_id = boost::none);

    breakpoint dprintf_insert(
            const std::string & format, const std::vector<std::string> & argument,
            const linespec_location & location,
            bool temporary = false, bool pending = false, bool disabled = false,
            const boost::optional<std::string> & condition = boost::none,
            const boost::optional<int> & ignore_count      = boost::none,
            const boost::optional<int> & thread_id         = boost::none);

    breakpoint dprintf_insert(
            const std::string & format, const std::vector<std::string> & argument,
            const explicit_location& location,
            bool temporary = false, bool pending = false, bool disabled = false,
            const boost::optional<std::string> & condition = boost::none,
            const boost::optional<int> & ignore_count      = boost::none,
            const boost::optional<int> & thread_id         = boost::none);

    breakpoint dprintf_insert(
            const std::string & format, const std::vector<std::string> & argument,
            const address_location & location,
            bool temporary = false, bool pending = false, bool disabled = false,
            const boost::optional<std::string> & condition = boost::none,
            const boost::optional<int> & ignore_count      = boost::none,
            const boost::optional<int> & thread_id         = boost::none);

    breakpoint dprintf_insert(
            const std::string & format, const std::vector<std::string> & argument,
            const boost::optional<std::string> & location = boost::none,
            bool temporary = false, bool pending = false, bool disabled = false,
            const boost::optional<std::string> & condition = boost::none,
            const boost::optional<int> & ignore_count      = boost::none,
            const boost::optional<int> & thread_id         = boost::none);

    std::vector<breakpoint> break_list();
    void break_passcount(std::size_t tracepoint_number, std::size_t passcount);
    watchpoint break_watch(const std::string & expr, bool access = false, bool read = false);

    breakpoint catch_load(const std::string regexp,
                          bool temporary = false,
                          bool disabled = false);

    breakpoint catch_unload(const std::string regexp,
                            bool temporary = false,
                            bool disabled = false);

    breakpoint catch_assert(const boost::optional<std::string> & condition = boost::none,
                            bool temporary = false, bool disabled = false);
    breakpoint catch_exception(const boost::optional<std::string> & condition = boost::none,
                               bool temporary = false, bool disabled = false);

    void exec_arguments(const std::vector<std::string> & args = {});
    void environment_cd(const std::string path);

    std::string environment_directory(const std::vector<std::string> & path, bool reset = false);
    std::string environment_path     (const std::vector<std::string> & path, bool reset = false);
    std::string environment_pwd();


    thread_state   thread_info_(const boost::optional<int> & id);
    thread_id_list thread_list_ids();
    struct thread_select  thread_select(int id);

    std::vector<struct ada_task_info> ada_task_info(const boost::optional<int> & task_id);

    void exec_continue(bool reverse = false, bool all = false);
    void exec_continue(bool reverse, int thread_group);
    void exec_finish(bool reverse);
    void exec_interrupt(bool all = false);
    void exec_interrupt(int thread_group);

    void exec_jump(const std::string & location);
    void exec_jump(const linespec_location & ls);
    void exec_jump(const explicit_location & el);
    void exec_jump(const address_location  & al);

    void exec_next(bool reverse);
    void exec_next_instruction(bool reverse);
    frame exec_return(const boost::optional<std::string> &val = boost::none);

    void exec_run(bool start = false, bool all = false);
    void exec_run(bool start, int thread_group);

    void exec_step(bool reverse = false);
    void exec_step_instruction(bool reverse = false);

    void exec_until(const std::string & location);
    void exec_until(const linespec_location & ls);
    void exec_until(const explicit_location & el);
    void exec_until(const address_location  & al);

    void enable_frame_filters();
    frame stacke_info_frame();
    std::size_t stack_info_depth();
    std::vector<frame> stack_list_arguments(
            print_values print_values_,
            const boost::optional<std::pair<std::size_t, std::size_t>> & frame_range = boost::none,
            bool no_frame_filters = false,
            bool skip_unavailable = false);

    std::vector<frame> stack_list_frames(
            const boost::optional<std::pair<std::size_t, std::size_t>> & frame_range = boost::none,
            bool no_frame_filters = false);

    std::vector<arg> stack_list_locals(
            print_values print_values_,
            bool no_frame_filters = false,
            bool skip_unavailable = false);

    std::vector<arg> stack_list_variables(
            print_values print_values_,
            bool no_frame_filters = false,
            bool skip_unavailable = false);

    void stack_select_frame(std::size_t framenum);

    void enable_pretty_printing();
    varobj var_create(const std::string& expression,
                   const boost::optional<std::string> & name = boost::none,
                   const boost::optional<std::uint64_t> & addr = boost::none);

    varobj var_create_floating(const std::string & expression, const boost::optional<std::string> & name = boost::none);
    void var_delete(const std::string & name, bool leave_children = false);
    void var_set_format(const std::string & name, format_spec fs);
    format_spec var_show_format(const std::string & name);
    std::size_t var_info_num_children(const std::string & name);
    std::vector<varobj> var_list_children(const std::string & name,
                                          const boost::optional<print_values> & print_values_ = boost::none,
                                          const boost::optional<std::pair<int, int>> & range = boost::none);
    std::string var_info_type(const std::string & name);
    std::pair<std::string, std::string> var_info_expression(const std::string & name);
    std::string var_info_path_expression(const std::string & name);
    std::vector<std::string> var_show_attributes(const std::string & name);
    std::string var_evaluate_expression(const std::string & name, const boost::optional<std::string> & format = boost::none);
    std::string var_assign(const std::string & name, const std::string& expr);
    std::vector<varobj_update> var_update(
                    const boost::optional<std::string> & name = boost::none,
                    const boost::optional<print_values> & print_values_ = boost::none);
    void var_set_frozen(const std::string & name, bool freeze = true);
    void var_set_update_range(const std::string & name, int from, int to);
    void var_set_visualizer(const std::string & name, const boost::optional<std::string> &visualizer = boost::none);
    void var_set_default_visualizer(const std::string & name);

    src_and_asm_line data_disassemble (disassemble_mode de);
    src_and_asm_line data_disassemble (disassemble_mode de, std::size_t start_addr, std::size_t end_addr);
    src_and_asm_line data_disassemble (disassemble_mode de, const std::string & filename, std::size_t linenum, const boost::optional<int> &lines = boost::none);

    std::string data_evaluate_expression(const std::string & expr);
    std::vector<std::string> data_list_changed_registers();
    std::vector<std::string> data_list_register_names();

    std::vector<register_value> data_list_register_values(
                         format_spec fmt,
                         const boost::optional<std::vector<int>> & regno = boost::none,
                         bool skip_unavaible = false);

    //note: removed output format, you'll get that directly as byte anyway
    read_memory data_read_memory(const std::string & address,
                     std::size_t word_size,
                     std::size_t nr_rows,
                     std::size_t nr_cols,
                     const boost::optional<int> & byte_offset = boost::none,
                     const boost::optional<char> & aschar = boost::none
                      );

    std::vector<read_memory_bytes> data_read_memory_bytes(const std::string &address, std::size_t count, const boost::optional<int> & offset = boost::none);

    void data_write_memory_bytes(const std::string & address,
                                 const std::vector<std::uint8_t> & contents,
                                 const boost::optional<std::size_t> & count = boost::none);

    boost::optional<found_tracepoint> trace_find(boost::none_t = boost::none);
    boost::optional<found_tracepoint> trace_find(const std::string & );
    boost::optional<found_tracepoint> trace_find_by_frame(int frame);
    boost::optional<found_tracepoint> trace_find(int number);
    boost::optional<found_tracepoint> trace_find_at(std::uint64_t addr);
    boost::optional<found_tracepoint> trace_find_inside(std::uint64_t start, std::uint64_t end);
    boost::optional<found_tracepoint> trace_find_outside(std::uint64_t start, std::uint64_t end);
    boost::optional<found_tracepoint> trace_find_line(std::size_t & line, const boost::optional<std::string> & file = boost::none);

    void trace_define_variable(const std::string & name, const boost::optional<std::string> & value = boost::none);

    traceframe_collection trace_frame_collected(
            const boost::optional<std::string> & var_pval  = boost::none,
            const boost::optional<std::string> & comp_pval = boost::none,
            const boost::optional<std::string> & regformat = boost::none,
            bool memory_contents = false);

    std::vector<trace_variable> trace_list_variables();
    void trace_save(const std::string & filename, bool remote = false);
    void trace_start();
    void trace_stop();

    struct trace_status trace_status();
    std::vector<symbol_line> symbol_list_lines(const std::string & filename);

    void file_exec_and_symbols(const std::string & file);
    void file_exec_file(const std::string & file);

    source_info file_list_exec_source_file();
    std::vector<source_info> file_list_exec_source_files();

    void file_symbol_file(const std::string & file);

    download_info target_download();

    template<typename Func>
    download_info target_download(Func && f)
    {
        boost::signals2::scoped_connection conn =
                _async_sink.connect(
                        [&](const async_output & ao)
                        {
                            if ((ao.type == async_output::status)
                             && (ao.class_ == "download"))
                                f(parse_result<download_status>(ao.results));
                        });
        return target_download();
    }

    connection_notification target_select(const std::string & type, const std::vector<std::string> & args);
    connection_notification target_select_core(const std::string & filename) { return target_select("core", {filename}); }
    connection_notification target_select_exec(const std::string & filename, const std::vector<std::string> & args = {})
    {
        std::vector<std::string> vec = { filename };
        vec.insert(vec.end(), args.begin(), args.end());
        return target_select("exec", vec);
    }
    connection_notification target_select_extended_remote(const std::string & medium)
    {
        return target_select("extended-remote", {medium});
    }
    connection_notification target_select_native(const std::string & filename, const std::vector<std::string> & args= {})
    {
        std::vector<std::string> vec = { filename };
        vec.insert(vec.end(), args.begin(), args.end());
        return target_select("native", vec);
    }
    connection_notification target_select_record()        { return target_select("record", {}); }
    connection_notification target_select_record_btrace() { return target_select("record-btrace", {}); }
    connection_notification target_select_record_core()   { return target_select("record-core", {}); }
    connection_notification target_select_record_full()   { return target_select("record-full", {}); }
    connection_notification target_select_remote(const std::string & medium) { return target_select("remote", {medium}); }
    connection_notification target_select_tfile(const std::string& file)     { return target_select("tfile", {file}); }

    void target_file_put(const std::string & hostfile,  const std::string & targetfile);
    void target_file_get(const std::string & targetfile, const std::string & hostfile);
    void target_file_delete(const std::string & targetfile);

    std::vector<info_ada_exception> info_ada_exceptions(const std::string & regexp);

    bool info_gdb_mi_command(const std::string & cmd_name);

    std::vector<std::string> list_features();
    std::vector<std::string> list_target_features();

    void gdb_exit();
    void gdb_set(const std::string & name, const std::string & value);
    std::string gdb_show(const std::string & name);
    std::string gdb_version();
    std::vector<groups> list_thread_groups(bool available = false, boost::optional<int> recurse = boost::none, std::vector<int> groups = {});

    std::vector<std::vector<std::string>> info_os(const boost::optional<std::string> & type = boost::none);
    std::string add_inferior();

    void interpreter_exec(const std::string & interpreter, const std::string & command);
    void inferior_tty_set(const std::string & terminal);
    std::string inferior_tty_show();
    void enable_timings(bool enable = true);
};



}
}
}



#endif /* METAL_GDB_MI2_SESSION_HPP_ */
