/**
 * @file   metal/gdb/mi2/session.cpp
 * @date   09.12.2016
 * @author Klemens D. Morgenstern
 *



 */

#include <metal/gdb/mi2/session.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
namespace asio = boost::asio;

namespace metal
{
namespace gdb
{
namespace mi2
{




void session::communicate(boost::asio::yield_context & yield_)
{
    asio::async_read_until(_out, _out_buf, "(gdb)\n", yield_);
}


void session::communicate(const std::string & in, boost::asio::yield_context & yield_)
{
    if (!in.empty())
        asio::async_write(_in, asio::buffer(in), yield_);

    communicate(yield_);
}



void session::handle()
{
    std::string str;
    std::istream stream{&_out_buf};

    while (std::getline(stream, str))
    {
        //handle the line.
    }

}

void session::operator()(boost::asio::yield_context & yield_)
{
    communicate(yield_);
}

}
}
}
