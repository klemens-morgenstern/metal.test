/**
 * @file   serial/test_functions.hpp
 * @date   03.05.2018
 * @author Klemens D. Morgenstern
 *
 */

#ifndef METAL_SERIAL_TEST_FUNCTIONS_HPP
#define METAL_SERIAL_TEST_FUNCTIONS_HPP

#include <metal/serial/session.hpp>

void metal_serial_test_setup_entries(std::unordered_map<std::string,metal::serial::plugin_function_t> & map);
void metal_serial_test_setup_options(boost::program_options::options_description & po);


#endif //METAL_SERIAL_TEST_FUNCTIONS_HPP
