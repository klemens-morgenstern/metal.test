/**
 * @file   sink.cpp
 * @date   06.05.2018
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

inline static const char* descr(level_t lvl)
{
    switch (lvl)
    {
        case level_t::assertion:
            return "assertion";
        case level_t::expect:
            return "expectation";
        default:
            return "";
    }
}

namespace rj = rapidjson;

struct json_sink_t : data_sink_t
{
    std::ostream * os = &std::cout;

    rj::Document doc;
    std::stack<rj::Value> vals;


    rj::Value steal()
    {
        if (vals.empty())
        {
            rj::Value val;
            val.SetObject();
            return val;
        }
        else
        {
            auto val = std::move(vals.top());
            vals.pop();
            return val;
        }
    }
    void repush(rj::Value && val)
    {
        if (vals.empty())
        {
            doc["content"].PushBack(std::move(val), doc.GetAllocator());
        }
        else
            vals.top()["content"].PushBack(std::move(val), doc.GetAllocator());
    }

    void add_to_array(rj::Value && val)
    {
        if (vals.size())
            vals.top()["content"].GetArray().PushBack(std::move(val), doc.GetAllocator());
        else
        {
            if (!doc.HasMember("content"))
            {
                rj::Value val;
                val.SetArray();
                doc.AddMember("content", std::move(val), doc.GetAllocator());
            }
            doc["content"].GetArray().PushBack(std::move(val), doc.GetAllocator());
        }
    }

    rj::Value loc(const std::string & file, int line)
    {
        rj::Value val;
        val.SetObject();
        val.AddMember("file", boost::replace_all_copy(file, "\\\\", "\\"), doc.GetAllocator());
        val.AddMember("line", line, doc.GetAllocator());
        return val;
    }

    rj::Value check(const std::string & file, int line, bool condition, level_t lvl)
    {
        auto val = loc(file, line);
        val.AddMember("condition", condition, doc.GetAllocator());
        val.AddMember("lvl",      rj::StringRef(descr(lvl)), doc.GetAllocator());

        return val;
    }

    virtual ~json_sink_t() = default;

    void enter_case(const std::string & file, int line, const std::string & id) override
    {
        auto val = loc(file, line);
        val.AddMember("type", "case", doc.GetAllocator());
        val.AddMember("id", id, doc.GetAllocator());

        rj::Value arr;
        arr.SetArray();
        val.AddMember("content", std::move(arr), doc.GetAllocator());

        vals.emplace(std::move(val));
    }
    void exit_case (const std::string & file, int line, const std::string & id, int executed, int warnings, int errors) override
    {
        auto val = steal();
        {
            rj::Value sum;
            sum.SetObject();
            sum.AddMember("executed", executed, doc.GetAllocator());
            sum.AddMember("warnings", warnings, doc.GetAllocator());
            sum.AddMember("errors",   errors  , doc.GetAllocator());
            val.AddMember("summary", std::move(sum), doc.GetAllocator());
        }
        val.AddMember("result", "exit", doc.GetAllocator());
        //   add_to_array(std::move(val));

        repush(std::move(val));
    }
    void report (int free_executed, int free_warnings, int free_errors,
                 int executed, int warnings, int errors) override
    {
        if (free_executed)
        {
            rj::Value val;
            val.SetObject();
            val.AddMember("executed", free_executed, doc.GetAllocator());
            val.AddMember("warnings", free_warnings, doc.GetAllocator());
            val.AddMember("errors",   free_errors  , doc.GetAllocator());

            doc.AddMember("free_tests", std::move(val), doc.GetAllocator());
        }

        {
            rj::Value val;
            val.SetObject();
            val.AddMember("executed", executed, doc.GetAllocator());
            val.AddMember("warnings", warnings, doc.GetAllocator());
            val.AddMember("errors",   errors  , doc.GetAllocator());

            doc.AddMember("summary", std::move(val), doc.GetAllocator());
        }
        rj::StringBuffer buffer;
        rj::PrettyWriter<rapidjson::StringBuffer> writer(buffer);

        doc.Accept(writer);

        *os << buffer.GetString();

    }

    void log        (const std::string & file, int line, const std::string & message) override
    {
        auto val = loc(file, line);
        val.AddMember("type", "message",  doc.GetAllocator());
        val.AddMember("message", message, doc.GetAllocator());
        add_to_array(std::move(val));

    }
    void checkpoint (const std::string & file, int line) override
    {
        auto val = loc(file, line);
        val.AddMember("type", "checkpoint", doc.GetAllocator());
        add_to_array(std::move(val));
    }

    void message(const std::string & file, int line, bool condition, level_t lvl, const std::string & message) override
    {
        auto val = check(file, line, condition, lvl);
        val.AddMember("type",  "message",  doc.GetAllocator());
        val.AddMember("message",  message, doc.GetAllocator());
        add_to_array(std::move(val));
    }
    void plain  (const std::string & file, int line, bool condition, level_t lvl, const std::string & message) override
    {
        auto val = check(file, line, condition, lvl);
        val.AddMember("type",    "plain",  doc.GetAllocator());
        val.AddMember("message",  message, doc.GetAllocator());
        add_to_array(std::move(val));
    }

    void equal     (const std::string & file, int line, bool condition, level_t lvl,
                    const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) override
    {
        auto val = check(file, line, condition, lvl);
        val.AddMember("type", "equal",  doc.GetAllocator());
        val.AddMember("lhs",        lhs,       doc.GetAllocator());
        val.AddMember("rhs",        rhs,       doc.GetAllocator());
        val.AddMember("lhs_val",    lhs_val,       doc.GetAllocator());
        val.AddMember("rhs_val",    rhs_val,       doc.GetAllocator());
        add_to_array(std::move(val));
    }

    void not_equal (const std::string & file, int line, bool condition, level_t lvl,
                    const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) override
    {
        auto val = check(file, line, condition, lvl);
        val.AddMember("type", "not_equal",  doc.GetAllocator());
        val.AddMember("lhs",       lhs,       doc.GetAllocator());
        val.AddMember("rhs",       rhs,       doc.GetAllocator());
        val.AddMember("lhs_val",   lhs_val,       doc.GetAllocator());
        val.AddMember("rhs_val",   rhs_val,       doc.GetAllocator());
        add_to_array(std::move(val));
    }

    void ge        (const std::string & file, int line, bool condition, level_t lvl,
                    const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) override
    {
        auto val = check(file, line, condition, lvl);
        val.AddMember("type", "ge",  doc.GetAllocator());
        val.AddMember("lhs",       lhs,       doc.GetAllocator());
        val.AddMember("rhs",       rhs,       doc.GetAllocator());
        val.AddMember("lhs_val",       lhs_val,       doc.GetAllocator());
        val.AddMember("rhs_val",       rhs_val,       doc.GetAllocator());
        add_to_array(std::move(val));
    }

    void greater   (const std::string & file, int line, bool condition, level_t lvl,
                    const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) override
    {
        auto val = check(file, line, condition, lvl);
        val.AddMember("type", "greater",  doc.GetAllocator());
        val.AddMember("lhs",       lhs,       doc.GetAllocator());
        val.AddMember("rhs",       rhs,       doc.GetAllocator());
        val.AddMember("lhs_val",       lhs_val,       doc.GetAllocator());
        val.AddMember("rhs_val",       rhs_val,       doc.GetAllocator());
        add_to_array(std::move(val));
    }

    void le        (const std::string & file, int line, bool condition, level_t lvl,
                    const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) override
    {
        auto val = check(file, line, condition, lvl);
        val.AddMember("type", "le",  doc.GetAllocator());
        val.AddMember("lhs",       lhs,       doc.GetAllocator());
        val.AddMember("rhs",       rhs,       doc.GetAllocator());
        val.AddMember("lhs_val",       lhs_val,       doc.GetAllocator());
        val.AddMember("rhs_val",       rhs_val,       doc.GetAllocator());
        add_to_array(std::move(val));
    }

    void lesser    (const std::string & file, int line, bool condition, level_t lvl,
                    const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) override
    {
        auto val = check(file, line, condition, lvl);
        val.AddMember("type", "lesser",  doc.GetAllocator());
        val.AddMember("lhs",       lhs,       doc.GetAllocator());
        val.AddMember("rhs",       rhs,       doc.GetAllocator());
        val.AddMember("lhs_val",       lhs_val,       doc.GetAllocator());
        val.AddMember("rhs_val",       rhs_val,       doc.GetAllocator());
        add_to_array(std::move(val));
    }

    void no_execute   (const std::string & file, int line, level_t lvl) override
    {
        auto val = check(file, line, false, lvl);
        val.AddMember("type", "no_execute_check", doc.GetAllocator());

        add_to_array(std::move(val));
    }
};


boost::optional<json_sink_t> json_sink;

data_sink_t * get_json_sink(std::ostream & os)
{
    json_sink.emplace();
    json_sink->os = &os;
    json_sink->doc.SetObject();
    return &*json_sink;
}