/**
 * @file   sink.hpp
 * @date   12.09.2016
 * @author Klemens D. Morgenstern
 *


 */
#ifndef SERIAL_SINK_HPP_
#define SERIAL_SINK_HPP_

#include <ostream>

enum class level_t
{
    assertion,
    expect,
};

struct data_sink_t
{
    virtual void enter_case(const std::string & file, int line, const std::string & id) = 0;
    virtual void exit_case (const std::string & file, int line, const std::string & id, int executed, int warnings, int errors) = 0;

    virtual void report (int free_executed, int free_warnings, int free_errors,
                         int executed, int warnings, int errors) = 0;

    virtual void log        (const std::string & file, int line, const std::string & id) = 0;
    virtual void checkpoint (const std::string & file, int line) = 0;

    virtual void message(const std::string & file, int line, bool condition, level_t lvl, const std::string & message) = 0;
    virtual void plain  (const std::string & file, int line, bool condition, level_t lvl, const std::string & message) = 0;

    virtual void equal     (const std::string & file, int line, bool condition, level_t lvl,  
                            const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) = 0;

    virtual void not_equal (const std::string & file, int line, bool condition, level_t lvl,  
                            const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) = 0;

    virtual void ge        (const std::string & file, int line, bool condition, level_t lvl, 
                            const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) = 0;

    virtual void greater   (const std::string & file, int line, bool condition, level_t lvl, 
                            const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) = 0;

    virtual void le        (const std::string & file, int line, bool condition, level_t lvl,  
                            const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) = 0;

    virtual void lesser    (const std::string & file, int line, bool condition, level_t lvl, 
                            const std::string & lhs, const std::string &rhs, const std::string & lhs_val, const std::string & rhs_val) = 0;

    virtual void no_execute   (const std::string & file, int line, level_t lvl) = 0;
};

data_sink_t * get_hrf_sink (std::ostream & os);
data_sink_t * get_json_sink(std::ostream & os);


#endif /* SINK_HPP_ */
