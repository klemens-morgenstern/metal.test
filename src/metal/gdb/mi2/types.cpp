#include <metal/gdb/mi2/types.hpp>
#include <iostream>
#include <boost/fusion/algorithm/iteration/for_each.hpp>

namespace metal
{
namespace gdb
{
namespace mi2
{

inline unsigned long long my_stoull(const std::string & str, std::size_t *pos = 0, int base = 10)
{
    try {
        return std::stoull(str, pos, base);
    }
    catch (std::invalid_argument & ia)
    {
        BOOST_THROW_EXCEPTION(parser_error("stoull[base:" + std::to_string(base) + "] - invalid argument '" + str + "'"));
    }
    catch (std::out_of_range & oor)
    {
        BOOST_THROW_EXCEPTION(parser_error("stoull[base:" + std::to_string(base) + "] - out of range '" + str + "'"));
    }
    return 0ull;
}

template<typename T>
struct is_vector : std::false_type {};

template<typename T>
struct is_vector<std::vector<T>> : std::true_type {};

template<typename T> struct tag {};

template<typename T> constexpr inline auto reflect(const T& );


const value& find(const std::vector<result> & input, const char * id)
{
    auto itr = std::find_if(input.begin(), input.end(),
                            [&](const result &r){return r.variable == id;});

    if (itr == input.end())
        BOOST_THROW_EXCEPTION( missing_value(id) );

    return itr->value_;
}

boost::optional<const value&> find_if(const std::vector<result> & input, const char * id)
{
    auto itr = std::find_if(input.begin(), input.end(),
                            [&](const result &r){return r.variable == id;});

    if (itr == input.end())
        return boost::none;
    else
        return itr->value_;
}

template<> error_ parse_result(const std::vector<result> &r)
{
    error_ err;
    err.msg = find(r, "msg").as_string();

    if (auto code = find_if(r, "code"))
        err.code = code->as_string();

    return err;
}

template<> breakpoint parse_result(const std::vector<result> &r)
{
    breakpoint bp;

    bp.number       = std::stoi(find(r, "number").      as_string());
    bp.type         = find(r, "type").        as_string();
    bp.disp         = find(r, "disp").        as_string();
    if (auto cp = find_if(r, "evaluated-by")) bp.evaluated_by = cp->as_string();
    {
        auto addr = find(r, "addr").as_string();
        if (addr != "<MULTIPLE>")
            bp.addr = my_stoull(addr, 0, 16);
        else
            bp.addr = 0;
    }

    bp.enabled      = find(r, "enabled").as_string() == "y";
    if (auto cp = find_if(r, "enable")) bp.enable = std::stoi(cp->as_string());
    bp.times        = std::stoi(find(r, "times"). as_string());

    if (auto cp = find_if(r, "catch-type")) bp.catch_type = cp->as_string();
    if (auto cp = find_if(r, "func")) bp.func = cp->as_string();
    if (auto cp = find_if(r, "filename")) bp.filename = cp->as_string();
    if (auto cp = find_if(r, "fullname")) bp.fullname = cp->as_string();

    if (auto cp = find_if(r, "at")) bp.at = cp->as_string();
    if (auto cp = find_if(r, "pending")) bp.pending = cp->as_string();
    if (auto cp = find_if(r, "thread")) bp.thread = cp->as_string();
    if (auto cp = find_if(r, "task")) bp.task = cp->as_string();
    if (auto cp = find_if(r, "cond")) bp.cond = cp->as_string();
    if (auto cp = find_if(r, "traceframe-usage")) bp.traceframe_usage = cp->as_string();
    if (auto cp = find_if(r, "static-tracepoint-marker-string-id")) bp.static_tracepoint_marker_string_id = cp->as_string();
    if (auto cp = find_if(r, "mask")) bp.mask = cp->as_string();
    if (auto cp = find_if(r, "original-location")) bp.original_location = cp->as_string();
    if (auto cp = find_if(r, "what")) bp.what = cp->as_string();

    if (auto cp = find_if(r, "line"))   bp.line   = std::stoi(cp->as_string());
    if (auto cp = find_if(r, "ignore")) bp.ignore = std::stoi(cp->as_string());
    if (auto cp = find_if(r, "pass"))   bp.pass   = std::stoi(cp->as_string());
    if (auto cp = find_if(r, "installed")) bp.installed = cp->as_string() == "y";

    if (auto cp = find_if(r, "thread-groups"))
    {
        const auto &l = cp->as_list().as_values();

        std::vector<std::string> th;
        th.resize(l.size());
        std::transform(l.begin(), l.end(),
                th.begin(),
                [](const value & v)
                {
                    return v.as_string();
                });

        bp.thread_groups = std::move(th);

    }

    return bp;
}


template<> watchpoint parse_result(const std::vector<result> &r)
{
    watchpoint wp;
    wp.number = std::stoi(find(r, "number").as_string());
    wp.exp = find(r, "exp").as_string();
    return wp;
}

template<> arg parse_result(const std::vector<result> & r)
{
    arg a;

    a.name  = find(r, "name").as_string();
    if (auto val = find_if(r, "value")) a.value = val->as_string();
    if (auto val = find_if(r,  "type")) a.type = val->as_string();

    return a;
}


template<> frame parse_result(const std::vector<result> &r)
{
    frame f;

    if (auto val = find_if(r, "level"))
        f.level = std::stoi(val->as_string());
    else
        f.level = 0; //< happends in case of breakpoint hit.
    if (auto val = find_if(r, "func")) f.func = val->as_string();
    if (auto val = find_if(r, "addr")) f.addr = my_stoull(find(r, "addr").as_string(), nullptr, 16);
    if (auto val = find_if(r, "file")) f.file = val->as_string();
    if (auto val = find_if(r, "line")) f.line = std::stoi(val->as_string());
    if (auto val = find_if(r, "from")) f.from = val->as_string();
    if (auto val = find_if(r, "args"))
    {
        auto & l = val->as_list();
        if (l.type() == boost::typeindex::type_id<std::vector<value>>())
        {
            auto vec = l.as_values();
            std::vector<arg> args;
            args.resize(vec.size());
            std::transform(vec.begin(), vec.end(), args.begin(),
                        [](const value & rc) -> arg
                        {
                            return {find(rc.as_tuple(), "name"). as_string(),
                                    find(rc.as_tuple(), "value").as_string()};
                        });
            f.args = std::move(args);
        }
        else //if no value is given.
        {
            auto vec = l.as_results();
            std::vector<arg> args;
            args.resize(vec.size());

            std::transform(vec.begin(), vec.end(), args.begin(),
                        [](const result & rc) -> arg
                        {
                            return {rc.value_.as_string(), {}};
                        });

            f.args = std::move(args);
        }
    }

    return f;
}

template<> thread parse_result(const std::vector<result> &r)
{
    thread t;
    t.id        = std::stoi(find(r, "id").as_string());
    t.target_id = find(r, "target-id").as_string();
    if (auto val = find_if(r, "frames"))
    {
        auto vec = val->as_list().as_values();
        std::vector<frame> res;
        for (auto & v : vec)
            res.push_back(parse_result<frame>(v.as_tuple()));
        t.frames = std::move(res);
    }
    t.state     = find(r, "state").as_string();

    return t;
}

template<> groups parse_result(const std::vector<result> &r)
{
    groups g;

    g.id = std::stoi(find(r, "id").as_string());
    g.type = find(r, "type").as_string();

    if (auto val = find_if(r, "pid"))          g.pid          = std::stoi(val->as_string());
    if (auto val = find_if(r, "exit_code"))    g.exit_code    = std::stoi(val->as_string());
    if (auto val = find_if(r, "num_children")) g.num_children = std::stoi(val->as_string());

    if (auto val = find_if(r, "threads"))
    {
        auto vec = val->as_list().as_values();
        std::vector<thread> res;
        for (auto & v : vec)
            res.push_back(parse_result<thread>(v.as_tuple()));
        g.threads = std::move(res);
    }
    if (auto val = find_if(r, "exit_code"))
    {
        auto vec = val->as_list().as_values();
        std::vector<int> res;
        for (auto & v : vec)
            res.push_back(std::atoi(v.as_string().c_str()));
        g.cores = std::move(res);
    }
    if (auto val = find_if(r, "executable")) g.executable = val->as_string();

    return g;
}


template<> thread_info parse_result(const std::vector<result> &r)
{
    thread_info ti;
    ti.id = std::stoi(find(r, "id").as_string());
    ti.target_id = find(r, "target-id").as_string();

    ti.state = (find(r, "state").as_string() == "stopped") ?
               thread_info::stopped : thread_info::running;


    if (auto val = find_if(r, "details")) ti.details = val->as_string();
    if (auto val = find_if(r, "core"))    ti.core = std::stoi(val->as_string());
    if (auto val = find_if(r, "frame"))   ti.frame = parse_result<frame>(val->as_tuple());

    return ti;
}

template<> thread_state parse_result(const std::vector<result> & ts)
{
    std::vector<thread_info> tis;
    {
      auto ths = find(ts, "threads").as_list().as_values();
      tis.resize(ths.size());
      std::transform(ths.begin(), ths.end(), tis.begin(),
              [](const value & v)
              {
                  return parse_result<thread_info>(v.as_tuple());
              });
    }
    return {std::move(tis), std::stoi(find(ts, "current-thread-id").as_string())};
}

template<> thread_id_list parse_result(const std::vector<result> & ts)
{
    std::vector<int> tis;
    {
      auto ths = find(ts, "thread-ids").as_list().as_results();
      tis.resize(ths.size());
      std::transform(ths.begin(), ths.end(), tis.begin(),
              [](const result & r)
              {
                  if (r.variable != "thread-id")
                      BOOST_THROW_EXCEPTION( missing_value("[" + r.variable + " != thread-id]") );
                  return std::stoi(r.value_.as_string());
              });
    }
    return {std::move(tis), std::stoi(find(ts, "current-thread-id").as_string()), std::stoi(find(ts, "number-of-threads").as_string())};
}

template<> thread_select parse_result(const std::vector<result> & r)
{
    thread_select ts;

    ts.new_thread_id = std::stoi(find(r, "new-thread-id").as_string());
    if (auto val = find_if(r, "frame")) ts.frame = parse_result<frame>(val->as_tuple());
    if (auto val = find_if(r, "args"))
    {
        auto vec = val->as_list().as_results();
        std::vector<arg> args;
        args.resize(vec.size());
        std::transform(vec.begin(), vec.end(), args.begin(),
                    [](const result & rc) -> arg
                    {
                        return {rc.variable, rc.value_.as_string()};
                    });
        ts.args = std::move(args);
    }
    return ts;
}

template<> ada_task_info parse_result(const std::vector<result> & r)
{
    ada_task_info ati;

    if (auto val = find_if(r, "current")) ati.current = val->as_string();
    ati.id =      std::stoi(find(r, "id").as_string());
    ati.task_id = std::stoi(find(r, "task-id").as_string());
    if (auto val = find_if(r, "thread-id")) ati.thread_id = std::stoi(val->as_string());
    if (auto val = find_if(r, "parent-id")) ati.parent_id = std::stoi(val->as_string());
    ati.priority = std::stoi(find(r, "priority").as_string());
    ati.state = find(r, "state").as_string();
    ati.name  = find(r, "name") .as_string();

    return ati;
}

template<> varobj parse_result(const std::vector<result> & r)
{
    varobj ati;
    ati.name     = find(r, "name").as_string();
    ati.numchild = std::stoi(find(r, "numchild").as_string());
    ati.value    = find(r, "value").as_string();
    ati.type     = find(r, "type").as_string();
    if (auto val = find_if(r, "thread-id")) ati.thread_id = std::stoi(val->as_string());
    if (auto val = find_if(r, "has_more"))  ati.has_more = std::stoi(val->as_string()) > 0;

    ati.dynamic = find_if(r, "dynamic").operator bool();

    if (auto val = find_if(r, "displayhint")) ati.displayhint = val->as_string();

    if (auto val = find_if(r, "exp")) ati.exp = val->as_string();
    ati.frozen = find_if(r, "dynamic").operator bool();

    return ati;
}


template<> varobj_update parse_result(const std::vector<result> & r)
{
    varobj_update vu;
    vu.name     = find(r, "name").as_string();
    vu.value    = find(r, "value").as_string();
    if (auto val = find_if(r, "in_scope"))
    {
        const auto str = val->as_string();
        if (str == "true")
            vu.in_scope = true;
        else if (str == "false")
            vu.in_scope = false;
    }
    if (auto val = find_if(r, "type_changed"))
    {
        const auto str = val->as_string();
        if (str == "true")
            vu.type_changed = true;
        else if (str == "false")
            vu.type_changed = false;
    }
    if (auto val = find_if(r, "has_more")) vu.has_more = std::stoi(val->as_string()) > 0;
    if (auto val = find_if(r, "new_type")) vu.new_type = val->as_string();
    if (auto val = find_if(r, "new_num_children"))   vu.new_num_children = std::stoi(val->as_string());

    vu.dynamic = find_if(r, "dynamic").operator bool();

    if (auto val = find_if(r, "displayhint")) vu.displayhint = val->as_string();

     vu.dynamic      = static_cast<bool>(find_if(r, "dynamic"));
     if (auto val = find_if(r, "new_children"))
     {
         auto vals = val->as_list().as_values();
         vu.new_children.clear();
         vu.new_children.resize(vals.size());
         for (auto & v : vals)
             vu.new_children.push_back(v.as_string());
     }

    return vu;
}


template<> line_asm_insn parse_result(const std::vector<result> & r)
{
    line_asm_insn lai;

    lai.address   = std::stoull(find(r, "address").as_string(), nullptr, 16);
    lai.func_name = find(r, "func-name").as_string();
    lai.offset    = std::stoi(find(r, "offset").as_string());
    lai.inst      = find(r, "inst").as_string();

    if (auto val = find_if(r, "opcodes"))
            lai.opcodes = val->as_string();

    return lai;
}

template<> src_and_asm_line parse_result(const std::vector<result> & r)
{
    src_and_asm_line lai;

    lai.line     = std::stoi(find(r, "line").as_string());
    lai.file     = find(r, "file").as_string();
    if (auto val = find_if(r, "fullname"))
        lai.fullname = val->as_string();

    if (auto val = find_if(r, "line_asm_insn"))
    {
        auto in = val->as_list().as_values();

        std::vector<line_asm_insn> vec;
        vec.reserve(in.size());

        for (auto & i : in)
            vec.push_back(parse_result<line_asm_insn>(i.as_tuple()));

        lai.line_asm_insn = std::move(vec);
    }

    return lai;
}

template<> register_value parse_result(const std::vector<result> & r)
{
    register_value lai;

    lai.number  = std::stoi(find(r, "number").as_string());
    lai.value   = find(r, "value").as_string();

    return lai;
}

template<> memory_entry parse_result(const std::vector<result> & r)
{
    memory_entry me;

    me.addr  = my_stoull(find(r, "addr").as_string(), nullptr, 16);

    auto data = find(r, "data").as_list().as_values();
    me.data.resize(data.size());

    for (auto & d : data)
        me.data.push_back(
            static_cast<std::uint8_t>(std::stoul(d.as_string(), nullptr, 16)));

    if (auto val = find_if(r, "ascii")) me.ascii = val->as_string();

    return me;
}


template<> read_memory parse_result(const std::vector<result> & r)
{
    read_memory rm;

    rm.addr        = my_stoull(find(r, "addr").as_string(), nullptr, 16);
    rm.nr_bytes    = my_stoull(find(r, "nr-bytes").as_string());
    rm.total_bytes = my_stoull(find(r, "total-bytes").as_string());
    rm.next_row    = my_stoull(find(r, "next-row").as_string(), nullptr, 16);
    rm.prev_row    = my_stoull(find(r, "prev-row").as_string(), nullptr, 16);
    rm.next_page   = my_stoull(find(r, "next-page").as_string(), nullptr, 16);
    rm.prev_page   = my_stoull(find(r, "prev-page").as_string(), nullptr, 16);


    auto mem = find(r, "memory").as_list().as_values();
    rm.memory.resize(mem.size());

    for (auto & m : mem)
        rm.memory.push_back(parse_result<memory_entry>(m.as_tuple()));

    return rm;
}

template<> read_memory_bytes parse_result(const std::vector<result> & r)
{
    read_memory_bytes rm;

    rm.begin  = my_stoull(find(r, "begin").as_string(), nullptr, 16);
    rm.offset = my_stoull(find(r, "offset").as_string());
    rm.end    = my_stoull(find(r, "end").as_string());

    auto ctn = find(r, "contents").as_string();
    rm.contents.resize(ctn.size() /2);


    auto to_int = [](char c) -> std::uint8_t
            {
                if ((c >= '0' ) && (c <= '9'))
                    return c - '0';

                if ((c >= 'A' ) && (c <= 'F'))
                    return c - 'A' + 10;

                if ((c >= 'a' ) && (c <= 'f'))
                    return c - 'a' + 10;

                return 0;
            };

    auto itr = ctn.begin();
    for (auto & c : rm.contents)
    {
        c  = to_int(*itr++) << static_cast<std::uint8_t>(4);
        c |= to_int(*itr++);
    }

    return rm;
}


template <> boost::optional<found_tracepoint> parse_result(const std::vector<result> & r)
{
    auto val = find_if(r, "found");
    if (!val)
        return boost::none;

    found_tracepoint ft;

    ft.traceframe = std::stoi(find(r, "traceframe").as_string());
    ft.tracepoint = std::stoi(find(r, "tracepoint").as_string());
    if (auto val = find_if(r, "frame")) ft.frame = parse_result<frame>(val->as_tuple());

    return ft;
}

template<> memory_region parse_result(const std::vector<result> & r)
{
    auto to_int = [](char c) -> std::uint8_t
            {
                if ((c >= '0' ) && (c <= '9'))
                    return c - '0';

                if ((c >= 'A' ) && (c <= 'F'))
                    return c - 'A' + 10;

                if ((c >= 'a' ) && (c <= 'f'))
                    return c - 'a' + 10;

                return 0;
            };

    memory_region mr;

    mr.address  = my_stoull(find(r, "address").as_string(), nullptr, 16);
    mr.length   = my_stoull(find(r, "value").as_string());
    if (auto value = find_if(r, "contents"))
    {
        auto ctn = value->as_string();
        mr.contents = std::vector<std::uint8_t>(ctn.size()/2);

        auto itr = ctn.begin();
        for (auto & c : *mr.contents)
        {
            c  = to_int(*itr++);
            c |= to_int(*itr++) << static_cast<std::uint8_t>(8);
        }
    }
    return mr;
}

template <> traceframe_collection parse_result(const std::vector<result> & r)
{
    traceframe_collection tc;

    if (auto val = find_if(r, "explicit-variables"))
    {
        auto vec = val->as_list().as_values();
        tc.explicit_variables.reserve(vec.size());
        for (auto & ev : vec)
            tc.explicit_variables.push_back(parse_result<register_value>(ev.as_tuple()));
    }

    if (auto val = find_if(r, "computed-expressions"))
    {
        auto vec = val->as_list().as_values();
        tc.computed_expressions.reserve(vec.size());
        for (auto & ev : vec)
            tc.computed_expressions.push_back(parse_result<register_value>(ev.as_tuple()));
    }

    if (auto val = find_if(r, "registers"))
    {
        auto vec = val->as_list().as_values();
        tc.registers.reserve(vec.size());
        for (auto & ev : vec)
            tc.registers.push_back(parse_result<register_value>(ev.as_tuple()));
    }

    if (auto val = find_if(r, "tvars"))
    {
        auto vec = val->as_list().as_values();
        tc.tvars.reserve(vec.size());
        for (auto & ev : vec)
            tc.tvars.push_back(parse_result<register_value>(ev.as_tuple()));
    }

    if (auto val = find_if(r, "tvars"))
    {
        auto vec = val->as_list().as_values();
        tc.memory.reserve(vec.size());
        for (auto & ev : vec)
            tc.memory.push_back(parse_result<memory_region>(ev.as_tuple()));
    }


    return tc;
}

template <> trace_variable parse_result(const std::vector<result> & r)
{
    trace_variable ft;

    ft.name = my_stoull(find(r, "name").as_string());
    ft.initial = my_stoull(find(r, "initial").as_string());
    if (auto val = find_if(r, "current")) ft.current = std::stoll(val->as_string());

    return ft;
}

template <> trace_status parse_result(const std::vector<result> & r)
{
    trace_status ft;

    ft.supported    = find(r, "supported").as_string() == "1";
    if (auto value = find_if(r, "running")) ft.running = (value->as_string() == "1");
    if (auto value = find_if(r, "stop-reason")) ft.stop_reason = value->as_string();
    if (auto value = find_if(r, "stopping-tracepoint")) ft.stopping_tracepoint = std::stoi(value->as_string());
    if (auto value = find_if(r, "frames")) ft.frames = my_stoull(value->as_string());
    if (auto value = find_if(r, "frames-created")) ft.frames_created = my_stoull(value->as_string());
    if (auto value = find_if(r, "buffer-size")) ft.buffer_size = my_stoull(value->as_string());
    if (auto value = find_if(r, "buffer-free")) ft.buffer_free = my_stoull(value->as_string());
    if (auto value = find_if(r, "circular"))     ft.circular     = (value->as_string() == "1");
    if (auto value = find_if(r, "disconnected")) ft.disconnected = (value->as_string() == "1");
    if (auto value = find_if(r, "trace-file")) ft.trace_file = value->as_string();

    return ft;
}

template <> symbol_line parse_result(const std::vector<result> & r)
{
    symbol_line sl;

    sl.pc = my_stoull(find(r, "pc").as_string());
    sl.line = find(r, "line").as_string();

    return sl;
}

template <> source_info parse_result(const std::vector<result> & r)
{
    source_info sl;

    sl.file     = find(r, "file").as_string();
    if (auto val = find_if(r, "line")) sl.line     = std::stoi(val->as_string());
    if (auto val = find_if(r, "line")) sl.fullname = val->as_string();
    if (auto val = find_if(r, "macro-info")) sl.macro_info = val->as_string();
    return sl;
}

template<> download_info parse_result(const std::vector<result> & r)
{
    download_info di;

    di.address       = my_stoull(find(r, "address").as_string(), nullptr, 16);
    di.load_size     = my_stoull(find(r, "load-size").as_string());
    di.transfer_rate = my_stoull(find(r, "transfer-rate").as_string());
    di.write_rate    = my_stoull(find(r, "write-rate").as_string());

    return di;
}

template<> download_status parse_result(const std::vector<result> & r)
{
    download_status ds;

    ds.section = find(r, "section").as_string();
    if (auto val = find_if(r, "section-sent")) ds.section_sent = my_stoull(find(r, "sections-sent").as_string());
    if (auto val = find_if(r, "total-sent"))   ds.total_sent   = my_stoull(find(r, "total-sent").as_string());
    ds.section_size  = my_stoull(find(r, "sections-size").as_string());
    ds.total_size    = my_stoull(find(r, "total-size").as_string());

    return ds;
}

template<> connection_notification parse_result(const std::vector<result> & r)
{
    connection_notification cn;

    if (auto val = find_if(r, "addr")) cn.addr = my_stoull(val->as_string(), nullptr, 16);
    if (auto val = find_if(r, "func")) cn.func = val->as_string();

    if (auto val = find_if(r, "args"))
    {
        auto v = val->as_list().as_values();

        cn.args.reserve(v.size());
        for (auto & x : v)
            cn.args.push_back(x.as_string());
    }
    return cn;
}

template<> info_ada_exception parse_result(const std::vector<result> & r)
{
    info_ada_exception ai;

    ai.name          = find(r, "name").as_string();
    ai.address       = my_stoull(find(r, "address").as_string(), nullptr, 16);

    return ai;
}

template<> thread_group_added parse_result(const std::vector<result> & r)
{
    thread_group_added tga;

    tga.id = std::stoi(find(r, "id").as_string());

    return tga;
}

template<> thread_group_removed parse_result(const std::vector<result> & r)
{
    thread_group_removed tgr;
    tgr.id = std::stoi(find(r, "id").as_string());
    return tgr;
}

template<> thread_group_started parse_result(const std::vector<result> & r)
{
    thread_group_started tgr;
    tgr.id  = std::stoi(find(r,  "id").as_string());
    tgr.pid = std::stoi(find(r, "pid").as_string());
    return tgr;
}

template<> thread_group_exited parse_result(const std::vector<result> & r)
{
    thread_group_exited tgr;
    tgr.id  = std::stoi(find(r,  "id").as_string());
    if (auto val = find_if(r, "exited")) tgr.exited = std::stoi(val->as_string());
    return tgr;
}

template<> thread_created parse_result(const std::vector<result> & r)
{
    thread_created tgr;
    tgr.id  = std::stoi(find(r,  "id").as_string());
    tgr.gid = std::stoi(find(r, "gid").as_string());
    return tgr;
}

template<> thread_exited parse_result(const std::vector<result> & r)
{
    thread_exited tgr;
    tgr.id  = std::stoi(find(r,  "id").as_string());
    tgr.gid = std::stoi(find(r, "gid").as_string());
    return tgr;
}

template<> thread_selected parse_result(const std::vector<result> & r)
{
    thread_selected tgr;
    tgr.id  = std::stoi(find(r,  "id").as_string());
    tgr.gid = std::stoi(find(r, "gid").as_string());
    return tgr;
}

template<> library_loaded parse_result(const std::vector<result> & r)
{
    library_loaded tgr;
    tgr.id          = std::stoi(find(r,  "id").as_string());
    tgr.host_name   = find(r, "host-name").as_string();
    tgr.target_name = find(r, "target-name").as_string();

    if (auto val = find_if(r, "symbols-loaded")) tgr.symbols_loaded = val->as_string();
    return tgr;
}

template<> traceframe_changed parse_result(const std::vector<result> & r)
{
    if (find_if(r, "end"))
        return traceframe_changed_end();

    traceframe_changed_t tct;
    tct.num         =  std::stoi(find(r,  "num").as_string());
    tct.tracepoint  = find(r, "tracepoint").as_string();
    return tct;
}

template<> tsv_frame parse_result(const std::vector<result> & r)
{
    tsv_frame tf;
    if (auto val = find_if(r, "name")) tf.name = val->as_string();
    if (auto val = find_if(r, "initial")) tf.initial = val->as_string();
    return tf;
}

template<> tsv_modified parse_result(const std::vector<result> & r)
{
    tsv_modified tf;
    tf.name = find(r, "name").as_string();
    tf.initial = find(r, "initial").as_string();
    if (auto val = find_if(r, "current")) tf.current = val->as_string();
    return tf;
}

template<> breakpoint_created parse_result(const std::vector<result> & r)
{
    breakpoint_created bc;
    bc.bkpt = parse_result<breakpoint>(find(r, "bkpt").as_tuple());
    return bc;
}

template<> breakpoint_modified parse_result(const std::vector<result> & r)
{
    breakpoint_modified bc;
    bc.bkpt = parse_result<breakpoint>(find(r, "bkpt").as_tuple());
    return bc;
}

template<> breakpoint_deleted parse_result(const std::vector<result> & r)
{
    breakpoint_deleted bc;
    bc.number = std::stoi(find(r, "number").as_string());
    return bc;
}

template<> record_started    parse_result(const std::vector<result> & r)
{
    record_started bc;
    bc.thread_group = std::stoi(find(r, "thread-group").as_string());
    if (auto val = find_if(r, "format")) bc.format = val->as_string();
    bc.method = find(r, "method").as_string();
    return bc;
}

template<> record_stopped    parse_result(const std::vector<result> & r)
{
    record_stopped rs;
    rs.thread_group = std::stoi(find(r, "thread-group").as_string());
    return rs;
}

template<> cmd_param_changed parse_result(const std::vector<result> & r)
{
    cmd_param_changed rs;
    rs.param = find(r, "param").as_string();
    rs.value = find(r, "value").as_string();
    return rs;
}

template<> memory_changed    parse_result(const std::vector<result> & r)
{
    memory_changed mc;
    mc.thread_group = std::stoi(find(r, "thread-group").as_string());
    mc.len = std::stoi(find(r, "len").as_string());
    mc.addr = my_stoull(find(r, "addr"). as_string(), nullptr, 16);
    if (auto val = find_if(r, "code")) mc.code = val->as_string();
    return mc;
}



}
}
}
