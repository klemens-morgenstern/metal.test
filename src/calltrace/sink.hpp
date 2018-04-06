/**
 * @file   /calltrace/src/sink.hpp
 * @date   08.05.2017
 * @author Klemens D. Morgenstern
 *



 */
#ifndef SINK_HPP_
#define SINK_HPP_

#include <metal/debug/frame.hpp>
#include "calltrace_clone.hpp"

struct data_sink_t
{
    virtual void enter(std::uint64_t      func_ptr, const boost::optional<metal::debug::address_info>& func,
                       std::uint64_t call_site_ptr, const boost::optional<metal::debug::address_info>& call_site,
                       const boost::optional<std::uint64_t> & ts = boost::none) = 0;
    virtual void exit (std::uint64_t      func_ptr, const boost::optional<metal::debug::address_info>& func,
                       std::uint64_t call_site_ptr, const boost::optional<metal::debug::address_info>& call_site,
                       const boost::optional<std::uint64_t> & ts = boost::none) = 0;

    virtual void set  (const calltrace_clone & cc, const boost::optional<std::uint64_t> & ts) = 0;
    virtual void reset(const calltrace_clone & cc, int error_cnt, const boost::optional<std::uint64_t> & ts) = 0;

    virtual void overflow(const calltrace_clone & cc, std::uint64_t addr, const boost::optional<metal::debug::address_info> & ai) = 0;
    virtual void mismatch(const calltrace_clone & cc, std::uint64_t addr, const boost::optional<metal::debug::address_info> & ai) = 0;

    virtual void incomplete(const calltrace_clone & cc, int missing) = 0;
    virtual void timestamp_unavailable() = 0;

    virtual ~data_sink_t() = default;


};

data_sink_t * get_hrf_sink (std::ostream & os);
data_sink_t * get_json_sink(std::ostream & os);


#endif /* SINK_HPP_ */
