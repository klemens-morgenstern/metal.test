/**
 * @file   sink.cpp
 * @date   12.09.2016
 * @author Klemens D. Morgenstern
 *


 */
#include "sink.hpp"
#include <iostream>
#include <stack>

#include <boost/algorithm/string/replace.hpp>
#include <boost/optional.hpp>

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>

using namespace std;

namespace rj = rapidjson;

struct json_sink_t : data_sink_t
{
    std::ostream * os = &std::cout;

    rj::Document doc;
    rj::Value calls;
    rj::Value errors;

    void add_to_calls(rj::Value && val)
    {
        calls.PushBack(std::move(val), doc.GetAllocator());
    }

    rj::Value loc(const std::string & file, int line)
    {
        rj::Value val;
        val.SetObject();
        val.AddMember("file", boost::replace_all_copy(file, "\\\\", "\\"), doc.GetAllocator());
        val.AddMember("line", line, doc.GetAllocator());
        return val;
    }

    json_sink_t()
    {
        doc.SetObject();
        calls.SetArray();
        errors.SetArray();
    }
    virtual ~json_sink_t()
    {
        doc.AddMember("calls",  std::move(calls),  doc.GetAllocator());
        doc.AddMember("errors", std::move(errors), doc.GetAllocator());

        rj::StringBuffer buffer;
        rj::PrettyWriter<rapidjson::StringBuffer> writer(buffer);

        doc.Accept(writer);

        *os << buffer.GetString();
    }

    rj::Value address_info(const metal::debug::address_info & ai)
    {
        rj::Value val;
        val.SetObject();
        val.AddMember("file", ai.file, doc.GetAllocator());
        if (ai.full_name)
            val.AddMember("full_name",  boost::replace_all_copy(*ai.full_name, "\\\\", "\\"), doc.GetAllocator());
        val.AddMember("line", static_cast<int>(ai.line), doc.GetAllocator());
        if (ai.function)
            val.AddMember("function", *ai.function, doc.GetAllocator());
        if (ai.offset)
            val.AddMember("offset", *ai.offset, doc.GetAllocator());

        return val;
    }

    rj::Value ct_entry(const calltrace_clone_entry & cce)
    {
        rj::Value val;
        val.SetObject();
        val.AddMember("address", cce.address, doc.GetAllocator());

        if (cce.info)
            val.AddMember("info", address_info(*cce.info), doc.GetAllocator());

        return val;
    }

    void enter(std::uint64_t      func_ptr, const boost::optional<metal::debug::address_info>& func,
               std::uint64_t call_site_ptr, const boost::optional<metal::debug::address_info>& call_site,
               const boost::optional<std::uint64_t> & ts) override
    {
        rj::Value val;
        val.SetObject();
        val.AddMember("mode", "enter", doc.GetAllocator());
        val.AddMember("this_fn_ptr",    func_ptr, doc.GetAllocator());
        val.AddMember("call_site_ptr", call_site_ptr, doc.GetAllocator());

        if (func)
            val.AddMember("this_fn", address_info(*func), doc.GetAllocator());
        if (call_site)
            val.AddMember("call_site", address_info(*call_site), doc.GetAllocator());

        if (ts)
            val.AddMember("timestamp", *ts, doc.GetAllocator());

        add_to_calls(std::move(val));
    }
    void exit (std::uint64_t      func_ptr, const boost::optional<metal::debug::address_info>& func,
               std::uint64_t call_site_ptr, const boost::optional<metal::debug::address_info>& call_site,
               const boost::optional<std::uint64_t> & ts) override
    {
        rj::Value val;
        val.SetObject();
        val.AddMember("mode", "exit", doc.GetAllocator());
        val.AddMember("this_fn_ptr",    func_ptr, doc.GetAllocator());
        val.AddMember("call_site_ptr", call_site_ptr, doc.GetAllocator());
        if (func)
            val.AddMember("this_fn", address_info(*func), doc.GetAllocator());
        if (call_site)
            val.AddMember("call_site", address_info(*call_site), doc.GetAllocator());

        if (ts)
            val.AddMember("timestamp", *ts, doc.GetAllocator());

        add_to_calls(std::move(val));
    }

    void set(const calltrace_clone & cc, const boost::optional<std::uint64_t> & ts) override
    {
        rj::Value val;
        val.SetObject();
        val.AddMember("mode", "set", doc.GetAllocator());
        if (ts)
            val.AddMember("timestamp", *ts, doc.GetAllocator());


        rj::Value ct;
        ct.SetObject();

        ct.AddMember("location", cc.location(), doc.GetAllocator());
        ct.AddMember("repeat", cc.repeat(), doc.GetAllocator());
        ct.AddMember("skip",   cc.skip(), doc.GetAllocator());
        ct.AddMember("fn", ct_entry(cc.fn()), doc.GetAllocator());

        rj::Value arr;
        arr.SetArray();
        arr.Reserve(cc.content().size(), doc.GetAllocator());

        for (auto & c : cc.content())
        {
            rj::Value val;
            val.SetObject();
            val.AddMember("address", c.address, doc.GetAllocator());
            if (c.address == 0)
                arr.PushBack(rj::Value().SetNull(), doc.GetAllocator());
            else
            {
                rj::Value val;
                val.SetObject();
                val.AddMember("address", c.address, doc.GetAllocator());
                if (c.info)
                {
                    auto & i = *c.info;
                    if (i.function)
                        val.AddMember("fn", *i.function, doc.GetAllocator());
                    val.AddMember("line", static_cast<int>(i.line), doc.GetAllocator());
                    val.AddMember("file", boost::replace_all_copy(i.file, "\\\\", "\\"), doc.GetAllocator());
                    if (i.full_name)
                        val.AddMember("full_name", boost::replace_all_copy(*i.full_name, "\\\\", "\\"), doc.GetAllocator());
                }

                arr.PushBack(std::move(val), doc.GetAllocator());
            }
            arr.PushBack(std::move(val), doc.GetAllocator());
        }
        val.AddMember("calltrace", std::move(ct), doc.GetAllocator());

        errors.PushBack(std::move(val), doc.GetAllocator());
    }

    void reset(const calltrace_clone & cc, int error_cnt, const boost::optional<std::uint64_t> & ts)
    {
        rj::Value val;
        val.SetObject();
        val.AddMember("mode", "reset", doc.GetAllocator());
        if (ts)
            val.AddMember("timestamp", *ts, doc.GetAllocator());

        rj::Value ct;
        ct.SetObject();
        ct.AddMember("location", cc.location(), doc.GetAllocator());
        ct.AddMember("repeat", cc.repeat(), doc.GetAllocator());

        val.AddMember("calltrace", std::move(ct), doc.GetAllocator());

        errors.PushBack(std::move(val), doc.GetAllocator());
    }

    void overflow(const calltrace_clone & cc, std::uint64_t addr, const boost::optional<metal::debug::address_info> & ai) override
    {
        rj::Value val;
        val.SetObject();

        val.AddMember("mode", "error", doc.GetAllocator());
        val.AddMember("type", "overflow", doc.GetAllocator());
        val.AddMember("calltrace_loc", cc.location(), doc.GetAllocator());
        val.AddMember("function_ptr", addr, doc.GetAllocator());
        if (ai)
            val.AddMember("function", address_info(*ai), doc.GetAllocator());

        rj::Value ct;
        ct.SetObject();
        ct.AddMember("location", cc.location(), doc.GetAllocator());
        ct.AddMember("repeated", cc.repeated(), doc.GetAllocator());

        val.AddMember("calltrace", std::move(ct), doc.GetAllocator());
        val.AddMember("location", cc.location(), doc.GetAllocator());

        errors.PushBack(std::move(val), doc.GetAllocator());
    }
    void mismatch(const calltrace_clone & cc, std::uint64_t addr, const boost::optional<metal::debug::address_info> & ai) override
    {
        rj::Value val;
        val.SetObject();

        val.AddMember("mode", "error", doc.GetAllocator());
        val.AddMember("type", "mismatch", doc.GetAllocator());
        val.AddMember("function_ptr", addr, doc.GetAllocator());

        if (ai)
            val.AddMember("function", address_info(*ai), doc.GetAllocator());

        rj::Value ct;
        ct.SetObject();
        ct.AddMember("location", cc.location(), doc.GetAllocator());
        ct.AddMember("repeated", cc.repeated(), doc.GetAllocator());

        val.AddMember("calltrace", std::move(ct), doc.GetAllocator());
        val.AddMember("location", cc.location(), doc.GetAllocator());

        errors.PushBack(std::move(val), doc.GetAllocator());
    }

    void incomplete(const calltrace_clone & cc, int position) override
    {
        rj::Value val;
        val.SetObject();

        val.AddMember("mode", "error", doc.GetAllocator());
        val.AddMember("type", "incomplete", doc.GetAllocator());

        val.AddMember("position", position, doc.GetAllocator());
        val.AddMember("size", static_cast<uint64_t>(cc.content().size()), doc.GetAllocator());

        rj::Value ct;
        ct.SetObject();
        ct.AddMember("location", cc.location(), doc.GetAllocator());
        ct.AddMember("repeated", cc.repeated(), doc.GetAllocator());

        val.AddMember("calltrace", std::move(ct), doc.GetAllocator());
        val.AddMember("location", cc.location(), doc.GetAllocator());

        errors.PushBack(std::move(val), doc.GetAllocator());
    }


    void timestamp_unavailable() override
    {
        rj::Value val;
        val.SetObject();
        val.AddMember("mode", "error", doc.GetAllocator());
        val.AddMember("type", "missing-timestamp", doc.GetAllocator());

        errors.PushBack(std::move(val), doc.GetAllocator());
    }
};


boost::optional<json_sink_t> json_sink;

data_sink_t * get_json_sink(std::ostream & os)
{
    json_sink.emplace();
    json_sink->os = &os;
    return &*json_sink;
}
