/**
 * @file   metal/debug/plugin.hpp
 * @date   10.11.2016
 * @author Klemens D. Morgenstern
 *




This header provides the function declarations needed to create plugins. It should be included
to asser the correctness of the function signatures and linkage.

 */
#ifndef METAL_GDB_PLUGIN_HPP_
#define METAL_GDB_PLUGIN_HPP_

#include <vector>
#include <memory>
#include <boost/program_options/options_description.hpp>
#include <metal/debug/break_point.hpp>

///This function is the central function needed to provide a break-point plugin.
extern "C" BOOST_SYMBOL_EXPORT void metal_dbg_setup_bps(std::vector<std::unique_ptr<metal::debug::break_point>> & bps);
///This function can be used to add program options for the plugin.
extern "C" BOOST_SYMBOL_EXPORT void metal_dbg_setup_options(boost::program_options::options_description & po);


#endif /* METAL_GDB_PLUGIN_HPP_ */
