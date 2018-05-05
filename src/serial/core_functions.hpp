/**
 * @file   serial/core_functions.hpp
 * @date   03.05.2018
 * @author Klemens D. Morgenstern
 *
 */

#ifndef METAL_TEST_CORE_FUNCTIONS_HPP
#define METAL_TEST_CORE_FUNCTIONS_HPP

#include <metal/serial/session.hpp>

void printf_impl(metal::serial::session&, const std::vector<std::string> &, const std::string & file, int line);
void exit_impl  (metal::serial::session&, const std::vector<std::string> &, const std::string & file, int line);




#endif //METAL_TEST_CORE_FUNCTIONS_HPP
