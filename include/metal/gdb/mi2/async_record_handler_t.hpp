/**
 * @file   metal/gdb/mi2/async_record_handler_t.hpp
 * @date   30.12.2016
 * @author Klemens D. Morgenstern
 *



 */
#ifndef METAL_GDB_MI2_ASYNC_RECORD_HANDLER_T_HPP_
#define METAL_GDB_MI2_ASYNC_RECORD_HANDLER_T_HPP_

#include <boost/signals2/signal.hpp>
#include <metal/gdb/mi2/output.hpp>
#include <metal/gdb/mi2/types.hpp>

namespace metal
{
namespace gdb
{
namespace mi2
{

class async_record_handler_t
{

    boost::signals2::signal<void(const async_output&)> _sig_fwd; //only forward notifications
    boost::signals2::scoped_connection _conn_fwd;

    template<typename T>
    boost::signals2::scoped_connection _make_adapter(boost::signals2::signal<void(const T &)> & sink, const char * id)
    {
        return _sig_fwd.connect(
                [&sink, id](const async_output & ao)
                {
                    if (!sink.empty())
                        if (ao.class_ == id)
                            parse_result<T>(ao.results);
                });
    }

    boost::signals2::signal<void(const thread_group_added &)> _sig_thread_group_added;
    boost::signals2::scoped_connection _conn_thread_group_added   = _make_adapter(_sig_thread_group_added, "thread-group-added");

    boost::signals2::signal<void(const thread_group_removed &)> _sig_thread_group_removed;
    boost::signals2::scoped_connection _conn_thread_group_removed = _make_adapter(_sig_thread_group_removed, "thread-group-removed");

    boost::signals2::signal<void(const thread_group_started &)> _sig_thread_group_started;
    boost::signals2::scoped_connection _conn_thread_group_started = _make_adapter(_sig_thread_group_started, "thread-group-started");

    boost::signals2::signal<void(const thread_group_exited &)> _sig_thread_group_exited;
    boost::signals2::scoped_connection _conn_thread_group_exited  = _make_adapter(_sig_thread_group_exited, "thread-group-exited");

    boost::signals2::signal<void(const thread_created &)> _sig_thread_created;
    boost::signals2::scoped_connection _conn_thread_created = _make_adapter(_sig_thread_created, "thread-created");

    boost::signals2::signal<void(const thread_exited &)> _sig_thread_exited;
    boost::signals2::scoped_connection _conn_thread_exited = _make_adapter(_sig_thread_exited, "thread-exited");

    boost::signals2::signal<void(const thread_selected &)> _sig_thread_selected;
    boost::signals2::scoped_connection _conn_thread_selected = _make_adapter(_sig_thread_selected, "thread-selected");

    boost::signals2::signal<void(const library_loaded &)> _sig_library_loaded;
    boost::signals2::scoped_connection _conn_library_loaded    = _make_adapter(_sig_library_loaded, "thread-loaded");

    boost::signals2::signal<void(const library_loaded &)> _sig_library_unloaded;
    boost::signals2::scoped_connection _conn_library_unloaded   = _make_adapter(_sig_library_unloaded, "thread-unloaded");

    boost::signals2::signal<void(const traceframe_changed &)> _sig_traceframe_changed;
    boost::signals2::scoped_connection _conn_traceframe_changed = _make_adapter(_sig_traceframe_changed, "traceframe-changed");

    boost::signals2::signal<void(const traceframe_changed_end &)> _sig_traceframe_changed_end;
    boost::signals2::scoped_connection _conn_traceframe_changed_end = _make_adapter(_sig_traceframe_changed, "traceframe-changed-end");

    boost::signals2::signal<void(const tsv_frame &)> _sig_tsv_created;
    boost::signals2::scoped_connection _conn_tsv_created  = _make_adapter(_sig_tsv_created, "tsv-created");

    boost::signals2::signal<void(const tsv_frame &)> _sig_tsv_deleted;
    boost::signals2::scoped_connection _conn_tsv_deleted  = _make_adapter(_sig_tsv_deleted, "tsv-deleted");

    boost::signals2::signal<void(const tsv_modified &)> _sig_tsv_modified;
    boost::signals2::scoped_connection _conn_tsv_modified = _make_adapter(_sig_tsv_modified, "tsv-modified");

    boost::signals2::signal<void(const breakpoint_created &)> _sig_breakpoint_created;
    boost::signals2::scoped_connection _conn_breakpoint_created  = _make_adapter(_sig_breakpoint_created, "breakpoint-created");

    boost::signals2::signal<void(const breakpoint_modified &)> _sig_breakpoint_modified;
    boost::signals2::scoped_connection _conn_breakpoint_modified = _make_adapter(_sig_breakpoint_modified, "breakpoint-modified");

    boost::signals2::signal<void(const breakpoint_deleted &)> _sig_breakpoint_deleted;
    boost::signals2::scoped_connection _conn_breakpoint_deleted  = _make_adapter(_sig_breakpoint_deleted, "breakpoint-deleted");

    boost::signals2::signal<void(const record_started &)> _sig_record_started;
    boost::signals2::scoped_connection _conn_record_started = _make_adapter(_sig_record_started, "record-started");

    boost::signals2::signal<void(const record_stopped &)> _sig_record_stopped;
    boost::signals2::scoped_connection _conn_record_stopped = _make_adapter(_sig_record_stopped, "record-stopped");

    boost::signals2::signal<void(const cmd_param_changed &)> _sig_cmd_param_changed;
    boost::signals2::scoped_connection _conn_cmd_param_changed = _make_adapter(_sig_cmd_param_changed, "cmd-param-changed");

    boost::signals2::signal<void(const memory_changed &)> _sig_memory_changed;
    boost::signals2::scoped_connection _conn_memory_changed    = _make_adapter(_sig_memory_changed, "memory-changed");

public:
    async_record_handler_t(boost::signals2::signal<void(const async_output &)> &async_sink) :
        _conn_fwd(async_sink.connect([this](const async_output & ao)
                            {
                                if (ao.type == async_output::notify)
                                    _sig_fwd(ao);
                            }))
    {

    }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_thread_group_added  (Args && ... args) {return _sig_thread_group_added.connect(std::forward<Args>(args)...); }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_thread_group_removed(Args && ... args) {return _sig_thread_group_removed.connect(std::forward<Args>(args)...); }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_thread_group_started(Args && ... args) {return _sig_thread_group_started.connect(std::forward<Args>(args)...); }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_thread_group_exited (Args && ... args) {return _sig_thread_group_exited.connect(std::forward<Args>(args)...); }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_thread_created (Args && ... args) {return _sig_thread_created.connect(std::forward<Args>(args)...); }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_thread_exited  (Args && ... args) {return _sig_thread_exited.connect(std::forward<Args>(args)...); }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_thread_selected(Args && ... args) {return _sig_thread_selected.connect(std::forward<Args>(args)...); }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_library_loaded  (Args && ... args) {return _sig_library_loaded.connect(std::forward<Args>(args)...); }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_library_unloaded(Args && ... args) {return _sig_library_unloaded.connect(std::forward<Args>(args)...); }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_traceframe_changed(Args && ... args) {return _sig_traceframe_changed.connect(std::forward<Args>(args)...); }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_traceframe_changed_end(Args && ... args) {return _sig_traceframe_changed_end.connect(std::forward<Args>(args)...); }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_tsv_created (Args && ... args) {return _sig_tsv_created.connect(std::forward<Args>(args)...); }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_tsv_deleted (Args && ... args) {return _sig_tsv_deleted.connect(std::forward<Args>(args)...); }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_tsv_modified(Args && ... args) {return _sig_tsv_modified.connect(std::forward<Args>(args)...); }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_breakpoint_created (Args && ... args) {return _sig_breakpoint_created.connect(std::forward<Args>(args)...); }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_breakpoint_modified(Args && ... args) {return _sig_breakpoint_modified.connect(std::forward<Args>(args)...); }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_breakpoint_deleted (Args && ... args) {return _sig_breakpoint_deleted.connect(std::forward<Args>(args)...); }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_record_started(Args && ... args) {return _sig_record_started.connect(std::forward<Args>(args)...); }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_record_stopped(Args && ... args) {return _sig_record_stopped.connect(std::forward<Args>(args)...); }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_cmd_param_changed(Args && ... args) {return _sig_cmd_param_changed.connect(std::forward<Args>(args)...); }

    template<typename ...Args>
    boost::signals2::scoped_connection connect_memory_changed   (Args && ... args) {return _sig_memory_changed.connect(std::forward<Args>(args)...); }

};

}
}
}



#endif /* METAL_GDB_MI2_ASYNC_RECORD_HANDLER_T_HPP_ */
